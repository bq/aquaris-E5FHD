/*
 * 
 * Author: MingHsien Hsieh <minghsien.hsieh@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>
#include <linux/hwmsen_helper.h>
#include "ltr553.h"


#define POWER_NONE_MACRO MT65XX_POWER_NONE


/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/

#define LTR553_DEV_NAME   "LTR_553ALS"

/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               printk(KERN_INFO APS_TAG"%s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR  APS_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(KERN_INFO APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    printk(KERN_INFO APS_TAG fmt, ##args)                 
/******************************************************************************
 * extern functions
*******************************************************************************/
extern void mt_eint_mask(unsigned int eint_num);
extern void mt_eint_unmask(unsigned int eint_num);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flow, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
extern void mt_eint_print_status(void);

#if defined(CKT_HALL_SWITCH_SUPPORT)
extern int g_is_calling;
#endif
/*----------------------------------------------------------------------------*/

static struct i2c_client *ltr553_i2c_client = NULL;

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ltr553_i2c_id[] = {{LTR553_DEV_NAME,0},{}};
/*the adapter id & i2c address will be available in customization*/
static struct i2c_board_info __initdata i2c_ltr553={ I2C_BOARD_INFO("LTR_553ALS", 0x23)};

//static unsigned short ltr553_force[] = {0x00, 0x46, I2C_CLIENT_END, I2C_CLIENT_END};
//static const unsigned short *const ltr553_forces[] = { ltr553_force, NULL };
//static struct i2c_client_address_data ltr553_addr_data = { .forces = ltr553_forces,};
/*----------------------------------------------------------------------------*/
static int ltr553_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id); 
static int ltr553_i2c_remove(struct i2c_client *client);
static int ltr553_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
static int ltr553_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int ltr553_i2c_resume(struct i2c_client *client);
#endif
static int ltr553_ps_enable(int gainrange);
static int ltr553_init_flag =0;//sen.luo
static int ltr553_local_init(void);
static int ltr_remove(void);

static struct sensor_init_info ltr553_init_info = {
	.name = LTR553_DEV_NAME,
	.init = ltr553_local_init,
	.uninit = ltr_remove,
};

static int ps_gainrange;
static int als_gainrange;

static int final_prox_val;
static int final_lux_val;

/*----------------------------------------------------------------------------*/
static DEFINE_MUTEX(LTR553_mutex);//ckt guoyi add 2015-1-8
#define I2C_FLAG_WRITE	0
#define I2C_FLAG_READ	1

/*----------------------------------------------------------------------------*/
static int ltr553_als_read(int gainrange);
static int ltr553_ps_read(void);


/*----------------------------------------------------------------------------*/


typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;

/*----------------------------------------------------------------------------*/
struct ltr553_i2c_addr {    /*define a series of i2c slave address*/
    u8  write_addr;  
    u8  ps_thd;     /*PS INT threshold*/
};

#define MAX_FIR_LENGTH 16

struct data_filter
{
    s16 raw[MAX_FIR_LENGTH];
    int sum;
    int num;
    int idx;
};

#define Calibrate_num 10
#define LTR_LT_N_CT	30
#define LTR_HT_N_CT	60

/*----------------------------------------------------------------------------*/

struct ltr553_priv {
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct work_struct  eint_work;
    struct mutex lock;
	/*i2c address group*/
    struct ltr553_i2c_addr  addr;

     /*misc*/
    u16		    als_modulus;
    atomic_t    i2c_retry;
    atomic_t    als_debounce;   /*debounce time after enabling als*/
    atomic_t    als_deb_on;     /*indicates if the debounce is on*/
    atomic_t    als_deb_end;    /*the jiffies representing the end of debounce*/
    atomic_t    ps_mask;        /*mask ps: always return far away*/
    atomic_t    ps_debounce;    /*debounce time after enabling ps*/
    atomic_t    ps_deb_on;      /*indicates if the debounce is on*/
    atomic_t    ps_deb_end;     /*the jiffies representing the end of debounce*/
    atomic_t    ps_suspend;
    atomic_t    als_suspend;

    /*data*/
    u16         als;
    u16          ps;
    u8          _align;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];
    int         ps_cali;

    atomic_t    als_cmd_val;    /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_cmd_val;     /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_thd_val;     /*the cmd value can't be read, stored in ram*/
	atomic_t    ps_thd_val_high;     /*the cmd value can't be read, stored in ram*/
	atomic_t    ps_thd_val_low;     /*the cmd value can't be read, stored in ram*/
    ulong       enable;         /*enable mask*/
    ulong       pending_intr;   /*pending interrupt*/

    atomic_t                firlen;
    atomic_t                fir_en;
    struct data_filter      fir;

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};

 struct PS_CALI_DATA_STRUCT
{
    int close;
    int far_away;
    int valid;
} ;

static struct PS_CALI_DATA_STRUCT ps_cali={0,0,0};
static int intr_flag_value = 0;


static struct ltr553_priv *ltr553_obj = NULL;
static struct platform_driver ltr553_alsps_driver;

/*----------------------------------------------------------------------------*/
static struct i2c_driver ltr553_i2c_driver = {	
	.probe      = ltr553_i2c_probe,
	.remove     = ltr553_i2c_remove,
	.detect     = ltr553_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND) 
	.suspend    = ltr553_i2c_suspend,
	.resume     = ltr553_i2c_resume,
#endif
	.id_table   = ltr553_i2c_id,
	//.address_data = &ltr553_addr_data,
	.driver = {
		//.owner          = THIS_MODULE,
		.name           = LTR553_DEV_NAME,
	},
};


/* 
 * #########
 * ## I2C ##
 * #########
 */

int ltr553_i2c_master_operate(struct i2c_client *client, const char *buf, int count, int i2c_flag)//ckt guoyi add 2015-1-8
{
	int res = 0;
	mutex_lock(&LTR553_mutex);

	switch(i2c_flag){
		case I2C_FLAG_WRITE:
			client->addr &=I2C_MASK_FLAG;
			res = i2c_master_send(client, buf, count);
			client->addr &=I2C_MASK_FLAG;
			break;

		case I2C_FLAG_READ:
			client->addr &=I2C_MASK_FLAG;
			client->addr |=I2C_WR_FLAG;
			client->addr |=I2C_RS_FLAG;
			res = i2c_master_send(client, buf, count);
			client->addr &=I2C_MASK_FLAG;
			break;

		default:
			APS_LOG("ltr553_i2c_master_operate i2c_flag command not support!\n");
			break;
	}

	if(res < 0)
	{
		goto EXIT_ERR;
	}
	mutex_unlock(&LTR553_mutex);
	return res;

	EXIT_ERR:
	mutex_unlock(&LTR553_mutex);
	APS_ERR("ltr553_i2c_transfer fail\n");
	return res;
}

// I2C Read
static int ltr553_i2c_read_reg(u8 regnum)
{
    u8 buffer[2],reg_value;
	int res = 0;
	struct i2c_client *client = ltr553_obj->client;

	buffer[0] = regnum;
	res = ltr553_i2c_master_operate(client, buffer, 0x101, I2C_FLAG_READ);//ckt guoyi modify 2015-1-8
	if (res < 0)
	{
		return res;
	}

	reg_value = buffer[0];
	return reg_value;
}

// I2C Write
static int ltr553_i2c_write_reg(u8 regnum, u8 value)
{
	u8 databuf[2];    
	int res = 0;
	databuf[0] = regnum;
	databuf[1] = value;
	struct i2c_client *client = ltr553_obj->client;

	res = ltr553_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);//ckt guoyi modify 2015-1-8

	return res;
}


/*----------------------------------------------------------------------------*/
static ssize_t ltr553_show_als(struct device_driver *ddri, char *buf)
{
	int res;
	
	if(!ltr553_obj)
	{
		APS_ERR("ltr553_obj is null!!\n");
		return 0;
	}
	res = ltr553_als_read(als_gainrange);
    return snprintf(buf, PAGE_SIZE, "0x%04X\n", res);    
	
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr553_show_ps(struct device_driver *ddri, char *buf)
{
	int  res;
	if(!ltr553_obj)
	{
		APS_ERR("ltr553_obj is null!!\n");
		return 0;
	}
	res = ltr553_ps_read();
    return snprintf(buf, PAGE_SIZE, "0x%04X\n", res);     
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
static ssize_t ltr553_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!ltr553_obj)
	{
		APS_ERR("ltr553_obj is null!!\n");
		return 0;
	}
	
	if(ltr553_obj->hw)
	{
	
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n", 
			ltr553_obj->hw->i2c_num, ltr553_obj->hw->power_id, ltr553_obj->hw->power_vol);
		
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}


	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&ltr553_obj->als_suspend), atomic_read(&ltr553_obj->ps_suspend));

	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t ltr553_store_status(struct device_driver *ddri, const char *buf, size_t count)
{
	int status1,ret;
	if(!ltr553_obj)
	{
		APS_ERR("ltr553_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "%d ", &status1))
	{ 
	    ret=ltr553_ps_enable(ps_gainrange);
		APS_DBG("iret= %d, ps_gainrange = %d\n", ret, ps_gainrange);
	}
	else
	{
		APS_DBG("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}


/*----------------------------------------------------------------------------*/
static ssize_t ltr553_show_reg(struct device_driver *ddri, char *buf)
{
	int i,len=0;
	int reg[]={0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,
		0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x97,0x98,0x99,0x9a,0x9e};
	for(i=0;i<27;i++)
		{
		len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%04X value: 0x%04X\n", reg[i],ltr553_i2c_read_reg(reg[i]));	

	    }
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr553_store_reg(struct device_driver *ddri, const char *buf, size_t count)
{
	int ret,value;
	unsigned int reg;
	if(!ltr553_obj)
	{
		APS_ERR("ltr553_obj is null!!\n");
		return 0;
	}
	
	if(2 == sscanf(buf, "%x %x ", &reg,&value))
	{ 
		APS_DBG("before write reg: %x, reg_value = %x  write value=%x\n", reg,ltr553_i2c_read_reg(reg),value);
	    ret=ltr553_i2c_write_reg(reg,value);
		APS_DBG("after write reg: %x, reg_value = %x\n", reg,ltr553_i2c_read_reg(reg));
	}
	else
	{
		APS_DBG("invalid content: '%s', length = %d\n", buf, count);
	}
	return count;    
}

static int ltr553_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
	u8 databuf[10];    

	memset(databuf, 0, sizeof(u8)*10);

	if((NULL == buf)||(bufsize<=30))
	{
		return -1;
	}
	
	if(NULL == client)
	{
		*buf = 0;
		return -2;
	}

	sprintf(buf, "ltr553 Chip");
	return 0;
}

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = ltr553_i2c_client;
	char strbuf[256];
	if(NULL == client)
	{
		printk("i2c client is null!!\n");
		return 0;
	}
	
	ltr553_ReadChipInfo(client, strbuf, 256);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}


/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo,   S_IWUSR | S_IRUGO, show_chipinfo_value,      NULL);
static DRIVER_ATTR(als,     S_IWUSR | S_IRUGO, ltr553_show_als,   NULL);
static DRIVER_ATTR(ps,      S_IWUSR | S_IRUGO, ltr553_show_ps,    NULL);
//static DRIVER_ATTR(config,  S_IWUSR | S_IRUGO, ltr553_show_config,ltr553_store_config);
//static DRIVER_ATTR(alslv,   S_IWUSR | S_IRUGO, ltr553_show_alslv, ltr553_store_alslv);
//static DRIVER_ATTR(alsval,  S_IWUSR | S_IRUGO, ltr553_show_alsval,ltr553_store_alsval);
//static DRIVER_ATTR(trace,   S_IWUSR | S_IRUGO,ltr553_show_trace, ltr553_store_trace);
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, ltr553_show_status,  ltr553_store_status);
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, ltr553_show_reg,   ltr553_store_reg);
//static DRIVER_ATTR(i2c,     S_IWUSR | S_IRUGO, ltr553_show_i2c,   ltr553_store_i2c);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *ltr553_attr_list[] = {
    &driver_attr_chipinfo,
    &driver_attr_als,
    &driver_attr_ps,    
   // &driver_attr_trace,        /*trace log*/
   // &driver_attr_config,
   // &driver_attr_alslv,
   // &driver_attr_alsval,
    &driver_attr_status,
   //&driver_attr_i2c,
    &driver_attr_reg,
};
/*----------------------------------------------------------------------------*/
static int ltr553_create_attr(struct device_driver *driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(ltr553_attr_list)/sizeof(ltr553_attr_list[0]));

	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if(err = driver_create_file(driver, ltr553_attr_list[idx]))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", ltr553_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/
static int ltr553_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(ltr553_attr_list)/sizeof(ltr553_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, ltr553_attr_list[idx]);
	}
	
	return err;
}

/*----------------------------------------------------------------------------*/

/* 
 * ###############
 * ## PS CONFIG ##
 * ###############

 */

static int ltr553_ps_set_thres(void)
{
	int res;
	u8 databuf[2];
	struct i2c_client *client = ltr553_obj->client;
	struct ltr553_priv *obj = ltr553_obj;

	APS_FUN();
	APS_DBG("ps_cali.valid: %d\n", ps_cali.valid);
	APS_ERR("ltr553_ps_set_thres high: 0x%x, low:0x%x\n",atomic_read(&obj->ps_thd_val_high),atomic_read(&obj->ps_thd_val_low));
	if(1 == ps_cali.valid)
	{
		databuf[0] = LTR553_PS_THRES_LOW_0; 
		databuf[1] = (u8)(ps_cali.far_away & 0x00FF);
		res = ltr553_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr553_ERR_I2C;
		}
		databuf[0] = LTR553_PS_THRES_LOW_1; 
		databuf[1] = (u8)((ps_cali.far_away & 0xFF00) >> 8);
		res = ltr553_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr553_ERR_I2C;
		}
		databuf[0] = LTR553_PS_THRES_UP_0;	
		databuf[1] = (u8)(ps_cali.close & 0x00FF);
		res = ltr553_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr553_ERR_I2C;
		}
		databuf[0] = LTR553_PS_THRES_UP_1;	
		databuf[1] = (u8)((ps_cali.close & 0xFF00) >> 8);;
		res = ltr553_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr553_ERR_I2C;
		}
	}
	else
	{
		databuf[0] = LTR553_PS_THRES_LOW_0; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
		res = ltr553_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr553_ERR_I2C;
		}
		databuf[0] = LTR553_PS_THRES_LOW_1; 
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low )>> 8) & 0x00FF);
		
		res = ltr553_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr553_ERR_I2C;
		}
		databuf[0] = LTR553_PS_THRES_UP_0;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
		res = ltr553_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr553_ERR_I2C;
		}
		databuf[0] = LTR553_PS_THRES_UP_1;	
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high) >> 8) & 0x00FF);
		res = ltr553_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr553_ERR_I2C;
		}
	
	}

	res = 0;
	return res;
	
	EXIT_ERR:
	APS_ERR("set thres: %d\n", res);
	return res;

}


static int ltr553_ps_enable(int gainrange)
{
	struct i2c_client *client = ltr553_obj->client;
	struct ltr553_priv *obj = ltr553_obj;
	u8 databuf[2];	
	int res;

	int error = 0;
	int setgain;
	APS_LOG("ltr553_ps_enable() ...start!\n");

// yong.su 2015-01-08 15:49:42	switch (gainrange) {
// yong.su 2015-01-08 15:49:42	case PS_RANGE16:
// yong.su 2015-01-08 15:49:42			setgain = MODE_PS_ON_Gain16;
// yong.su 2015-01-08 15:49:42			break;
// yong.su 2015-01-08 15:49:42
// yong.su 2015-01-08 15:49:42		case PS_RANGE32:
// yong.su 2015-01-08 15:49:42			setgain = MODE_PS_ON_Gain32;
// yong.su 2015-01-08 15:49:42			break;
// yong.su 2015-01-08 15:49:42
// yong.su 2015-01-08 15:49:42		case PS_RANGE64:
// yong.su 2015-01-08 15:49:42			setgain = MODE_PS_ON_Gain64;
// yong.su 2015-01-08 15:49:42			break;
// yong.su 2015-01-08 15:49:42
// yong.su 2015-01-08 15:49:42		default:
// yong.su 2015-01-08 15:49:42			setgain = MODE_PS_ON_Gain16;
// yong.su 2015-01-08 15:49:42			break;
// yong.su 2015-01-08 15:49:42	}
	setgain = 0x3;
	APS_LOG("LTR553_PS setgain = %d!\n",setgain);

	res = ltr553_i2c_write_reg(LTR553_PS_CONTR, setgain); 
	if(res<0)
	{
		APS_LOG("ltr553_ps_enable() error1\n");
        error = ltr553_ERR_I2C;
		goto EXIT_ERR;
	}

	mdelay(WAKEUP_DELAY);
    
	/* =============== 
	 * ** IMPORTANT **
	 * ===============
	 * Other settings like timing and threshold to be set here, if required.
 	 * Not set and kept as device default for now.
 	 */
	res = ltr553_i2c_write_reg(LTR553_PS_N_PULSES, 4); 
	if(res<0)
	{
		APS_LOG("ltr553_ps_enable() error2\n");
		error = ltr553_ERR_I2C;
		goto EXIT_ERR;
	} 
	res = ltr553_i2c_write_reg(LTR553_PS_LED, 0x5c); 
	if(res<0)
	{
		APS_LOG("ltr553_ps_enable() error3...\n");
		error = ltr553_ERR_I2C;
		goto EXIT_ERR;
	}

	/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
	if(0 == obj->hw->polling_mode_ps)
	{
		ltr553_ps_set_thres();

		databuf[0] = LTR553_INTERRUPT;	
		databuf[1] = 0x01;
		res = ltr553_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if(res <= 0)
		{
			error = ltr553_ERR_I2C;
			goto EXIT_ERR;
		}

		databuf[0] = LTR553_INTERRUPT_PERSIST;
		databuf[1] = 0x30;
		res = ltr553_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if(res <= 0)
		{
			error = ltr553_ERR_I2C;
			goto EXIT_ERR;
		}
		mt_eint_unmask(CUST_EINT_ALS_NUM);

	}

#if defined(CKT_HALL_SWITCH_SUPPORT)
    g_is_calling=1;
#endif
    
 	APS_LOG("ltr553_ps_enable ...OK!\n");

	return error;

	EXIT_ERR:
	APS_ERR("ltr553_ps_enable error: %d\n", error);
	return error;
}

// Put PS into Standby mode
static int ltr553_ps_disable(void)
{
	int error = 0;
    int res = 0;
	struct ltr553_priv *obj = ltr553_obj;
// yong.su 2015-01-08 15:48:53	res = ltr553_i2c_write_reg(LTR553_ALS_CONTR, 2); 
// yong.su 2015-01-08 15:48:53	if(res<0)
// yong.su 2015-01-08 15:48:53 	    APS_LOG("ltr553_ps_disable ...ERROR\n");
// yong.su 2015-01-08 15:48:53 	else
// yong.su 2015-01-08 15:48:53        APS_LOG("ltr553_ps_disable ...OK\n");
// yong.su 2015-01-08 15:48:53	mdelay(WAKEUP_DELAY);

//	res = ltr553_i2c_write_reg(LTR553_PS_CONTR, 3); 
//	if(res<0)
// 	    APS_LOG("ltr553_ps_disable2 ...ERROR\n");
// 	else
//        APS_LOG("ltr553_ps_disable2 ...OK\n");
//	mdelay(WAKEUP_DELAY);	
	res = ltr553_i2c_write_reg(LTR553_PS_CONTR, MODE_PS_StdBy); 
	if(res<0)
	{
		APS_LOG("ltr553_ps_disable ...ERROR\n");
		error = ltr553_ERR_I2C;
        goto EXIT_ERR;
	}
	else
		APS_LOG("ltr553_ps_disable ...OK\n");

	mdelay(WAKEUP_DELAY);
	if(0 == obj->hw->polling_mode_ps)
	{
	    cancel_work_sync(&obj->eint_work);
		mt_eint_mask(CUST_EINT_ALS_NUM);
	}

	if (atomic_read(&obj->fir_en) && !atomic_read(&obj->ps_suspend))
	{
		obj->fir.num = 0;
		obj->fir.idx = 0;
		obj->fir.sum = 0;
	}

#if defined(CKT_HALL_SWITCH_SUPPORT)
    g_is_calling=0;
#endif
    
	return error;

EXIT_ERR:
APS_ERR("ltr553_ps_enable error: %d\n", error);
return error;

}


static int ltr553_ps_read(void)
{
	int psval_lo, psval_hi, psdata;

	psval_lo = ltr553_i2c_read_reg(LTR553_PS_DATA_0);
	//	APS_DBG("ps_rawdata_psval_lo = %d\n", psval_lo);
	if (psval_lo < 0){
		APS_DBG("psval_lo error\n");
		psdata = psval_lo;
		goto out;
	}
	psval_hi = ltr553_i2c_read_reg(LTR553_PS_DATA_1);
	// APS_DBG("ps_rawdata_psval_hi = %d\n", psval_hi);

	if (psval_hi < 0){
		APS_DBG("psval_hi error\n");
		psdata = psval_hi;
		goto out;
	}

	psdata = ((psval_hi & 7)* 256) + psval_lo;
    //psdata = ((psval_hi&0x7)<<8) + psval_lo;

	if (atomic_read(&ltr553_obj->fir_en) && !atomic_read(&ltr553_obj->ps_suspend))
	{
		int idx, firlen = atomic_read(&ltr553_obj->firlen);   
		if (ltr553_obj->fir.num < firlen)
		{
			ltr553_obj->fir.raw[ltr553_obj->fir.num] = psdata;
			ltr553_obj->fir.sum += psdata;
			ltr553_obj->fir.num++;
			ltr553_obj->fir.idx++;
		}
		else
		{
			idx = ltr553_obj->fir.idx % firlen;
			ltr553_obj->fir.sum -= ltr553_obj->fir.raw[idx];
			ltr553_obj->fir.raw[idx] = psdata;
			ltr553_obj->fir.sum += psdata;
			ltr553_obj->fir.idx++;
			psdata = ltr553_obj->fir.sum/firlen;
		}
		//APS_DBG("fir.idx = %d\n", ltr553_obj->fir.idx);
	}

	if (psdata > 980)//ckt guoyi add for factory mode display 3FFh in als/ps 2015-2-3
	{
		psdata = 1023;
	}
    //APS_DBG("ps_rawdata = %d\n", psdata);
    
	out:
	final_prox_val = psdata;
	
	return psdata;
}

/* 
 * ################
 * ## ALS CONFIG ##
 * ################
 */

static int ltr553_als_enable(int gainrange)
{
	int error = 0;
    int res;
	APS_LOG("als_gainrange = %d\n",gainrange);
	switch (gainrange)
	{
		case ALS_RANGE_64K:
			res = ltr553_i2c_write_reg(LTR553_ALS_CONTR, MODE_ALS_ON_Range1);
			break;

		case ALS_RANGE_32K:
			res = ltr553_i2c_write_reg(LTR553_ALS_CONTR, MODE_ALS_ON_Range2);
			break;

		case ALS_RANGE_16K:
			res = ltr553_i2c_write_reg(LTR553_ALS_CONTR, MODE_ALS_ON_Range3);
			break;
			
		case ALS_RANGE_8K:
			res = ltr553_i2c_write_reg(LTR553_ALS_CONTR, MODE_ALS_ON_Range4);
			break;
			
		case ALS_RANGE_1300:
			res = ltr553_i2c_write_reg(LTR553_ALS_CONTR, MODE_ALS_ON_Range5);
			break;

		case ALS_RANGE_600:
			res = ltr553_i2c_write_reg(LTR553_ALS_CONTR, MODE_ALS_ON_Range6);
			break;
			
		default:
			res = ltr553_i2c_write_reg(LTR553_ALS_CONTR, MODE_ALS_ON_Range1);
			APS_ERR("als sensor als_gainrange %d!\n", gainrange);
			break;
	}

	mdelay(WAKEUP_DELAY);

	/* =============== 
	 * ** IMPORTANT **
	 * ===============
	 * Other settings like timing and threshold to be set here, if required.
 	 * Not set and kept as device default for now.
 	 */
	if(res<0)
	{
		error = ltr553_ERR_I2C;
		APS_LOG("ltr553_als_enable ...ERROR\n");
	}
	else
		APS_LOG("ltr553_als_enable ...OK\n");
        
	return error;
}


// Put ALS into Standby mode
static int ltr553_als_disable(void)
{
	int error = 0;
    int res;
	//res = ltr553_i2c_write_reg(LTR553_ALS_CONTR, 2); 
	//mdelay(WAKEUP_DELAY);
//	res = ltr553_i2c_write_reg(LTR553_ALS_CONTR, 1);
//	if(res<0)
// 	    APS_LOG("ltr553_als_disable ...ERROR\n");
// 	else
//        APS_LOG("ltr553_als_disable ...OK\n");
//	mdelay(WAKEUP_DELAY);
	res = ltr553_i2c_write_reg(LTR553_ALS_CONTR, MODE_ALS_StdBy); 
	if(res<0)
	{
		error = ltr553_ERR_I2C;
		APS_LOG("ltr553_als_enable ...ERROR\n");
	}
	else
		APS_LOG("ltr553_als_disable ...OK\n");

	mdelay(WAKEUP_DELAY);

	return error;
}

static int ltr553_als_read(int gainrange)
{
	int alsval_ch0_lo, alsval_ch0_hi, alsval_ch0;
	int alsval_ch1_lo, alsval_ch1_hi, alsval_ch1;
	int  luxdata_int;
	int ratio;

	
	alsval_ch1_lo = ltr553_i2c_read_reg(LTR553_ALS_DATA_CH1_0);
	alsval_ch1_hi = ltr553_i2c_read_reg(LTR553_ALS_DATA_CH1_1);
	alsval_ch1 = (alsval_ch1_hi * 256) + alsval_ch1_lo;
	APS_DBG("alsval_ch1_lo = %d,alsval_ch1_hi=%d,alsval_ch1=%d\n",alsval_ch1_lo,alsval_ch1_hi,alsval_ch1);
       alsval_ch0_lo = ltr553_i2c_read_reg(LTR553_ALS_DATA_CH0_0);
	alsval_ch0_hi = ltr553_i2c_read_reg(LTR553_ALS_DATA_CH0_1);
	alsval_ch0 = (alsval_ch0_hi * 256) + alsval_ch0_lo;
	APS_DBG("alsval_ch0_lo = %d,alsval_ch0_hi=%d,alsval_ch0=%d\n",alsval_ch0_lo,alsval_ch0_hi,alsval_ch0);
    if((alsval_ch1==0)||(alsval_ch0==0))
    {
        luxdata_int = 0;
        goto err;
    }
#if 0
	ratio = (alsval_ch1*100) /(alsval_ch0+alsval_ch1);
	APS_DBG("ratio = %d  als_gainrange = %d\n",ratio,gainrange);
	if (ratio < 45){
		luxdata_int = (((17743 * alsval_ch0)+(11059 * alsval_ch1))/gainrange)/10000;
	}
	else if ((ratio < 64) && (ratio >= 45)){
		luxdata_int = (((42785 * alsval_ch0)-(19548 * alsval_ch1))/gainrange)/10000;
	}
	else if ((ratio < 85) && (ratio >= 64)) {
		luxdata_int = (((5926 * alsval_ch0)+(1185 * alsval_ch1))/gainrange)/10000;
	}
	else {
		luxdata_int = 0;
		}
#endif
	luxdata_int = alsval_ch0;
	APS_DBG("als_value_lux = %d\n", luxdata_int);
	return luxdata_int;

	
err:
	final_lux_val = luxdata_int;
	APS_DBG("err als_value_lux = 0x%x\n", luxdata_int);
	return luxdata_int;
}

/*----------------------------------------------------------------------------*/
int ltr553_get_addr(struct alsps_hw *hw, struct ltr553_i2c_addr *addr)
{
	/***
	if(!hw || !addr)
	{
		return -EFAULT;
	}
	addr->write_addr= hw->i2c_addr[0];
	***/
	return 0;
}


/*-----------------------------------------------------------------------------*/
void ltr553_eint_func(void)
{
	struct ltr553_priv *obj = ltr553_obj;
	if(!obj)
	{
		return;
	}
	APS_FUN();
	
	schedule_work(&obj->eint_work);
	//schedule_delayed_work(&obj->eint_work);
}


/*----------------------------------------------------------------------------*/
/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
int ltr553_setup_eint(struct i2c_client *client)
{
	struct ltr553_priv *obj = (struct ltr553_priv *)i2c_get_clientdata(client);        
	APS_FUN();

	ltr553_obj = obj;
	mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
	mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, TRUE);
	mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);

	//mt65xx_eint_set_sens(CUST_EINT_ALS_NUM, CUST_EINT_ALS_SENSITIVE);
	//mt65xx_eint_set_polarity(CUST_EINT_ALS_NUM, CUST_EINT_ALS_POLARITY);
	//mt65xx_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	//mt65xx_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_EN, CUST_EINT_ALS_POLARITY, ltr553_eint_func, 0);
	//mt65xx_eint_unmask(CUST_EINT_ALS_NUM);  

	mt_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_TYPE, ltr553_eint_func, 0);

	mt_eint_unmask(CUST_EINT_ALS_NUM);
    return 0;
}


/*----------------------------------------------------------------------------*/
static void ltr553_power(struct alsps_hw *hw, unsigned int on) 
{
	static unsigned int power_on = 0;

	//APS_LOG("power %s\n", on ? "on" : "off");

	if(hw->power_id != POWER_NONE_MACRO)
	{
		if(power_on == on)
		{
			APS_LOG("ignore power control: %d\n", on);
		}
		else if(on)
		{
			if(!hwPowerOn(hw->power_id, hw->power_vol, "LTR553")) 
			{
				APS_ERR("power on fails!!\n");
			}
		}
		else
		{
			if(!hwPowerDown(hw->power_id, "LTR553")) 
			{
				APS_ERR("power off fail!!\n");   
			}
		}
	}
	power_on = on;
}

/*----------------------------------------------------------------------------*/
/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
static int ltr553_check_and_clear_intr(struct i2c_client *client) 
{
//***
	int res,intp,intl;
	u8 buffer[2];	
	u8 temp;
		//if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/	
		//	  return 0;
	APS_FUN();

    res = buffer[0] = ltr553_i2c_read_reg(LTR553_ALS_PS_STATUS);
//    APS_LOG("ltr553_check_and_clear_intr() res=%d !\n", res);
	if(res < 0)
	{
		goto EXIT_ERR;
	}

	temp = buffer[0];
	res = 1;
	intp = 0;
	intl = 0;
	if(0 != (buffer[0] & 0x02))
	{
		res = 0;
		intp = 1;
	}
	if(0 != (buffer[0] & 0x08))
	{
		res = 0;
		intl = 1;		
	}

	if(0 == res)
	{
		if((1 == intp) && (0 == intl))
		{
			buffer[1] = buffer[0] & 0xfD;
		}
		else if((0 == intp) && (1 == intl))
		{
			buffer[1] = buffer[0] & 0xf7;
		}
		else
		{
			buffer[1] = buffer[0] & 0xf5;
		}
		buffer[0] = LTR553_ALS_PS_STATUS;
//		res = i2c_master_send(client, buffer, 0x2);
		if(res <= 0)
		{
			goto EXIT_ERR;
		}
		else
		{
			res = 0;
		}
	}

	return res;

EXIT_ERR:
	APS_ERR("ltr553_check_and_clear_intr fail\n");
	return res;

}
/*----------------------------------------------------------------------------*/


static int ltr553_check_intr(struct i2c_client *client) 
{
	int res,intp,intl;
	u8 buffer[2];

	APS_FUN();

	//if (mt_get_gpio_in(GPIO_ALS_EINT_PIN) == 1) /*skip if no interrupt*/  
	//    return 0;

	buffer[0] = LTR553_ALS_PS_STATUS;

	res = buffer[0] = ltr553_i2c_read_reg(LTR553_ALS_PS_STATUS);
//	APS_LOG("ltr553_check_intr() res=%d !\n", res);
	if(res < 0)
	{
		goto EXIT_ERR;
	}
    
	res = 1;
	intp = 0;
	intl = 0;
	if(0 != (buffer[0] & 0x02))
	{
		res = 0;
		intp = 1;
	}
	if(0 != (buffer[0] & 0x08))
	{
		res = 0;
		intl = 1;		
	}

	return res;

EXIT_ERR:
	APS_ERR("ltr553_check_intr fail\n");
	return res;
}

static int ltr553_clear_intr(struct i2c_client *client) 
{
	int res;
	u8 buffer[2];

	APS_FUN();
	
	buffer[0] = LTR553_ALS_PS_STATUS;
	res = buffer[0] = ltr553_i2c_read_reg(LTR553_ALS_PS_STATUS);
//    APS_LOG("ltr553_clear_intr() res=%d !\n", res);
	if(res < 0)
	{
		goto EXIT_ERR;
	}

//	APS_DBG("buffer[0] = %d \n",buffer[0]);
	buffer[1] = buffer[0] & 0x01;
	buffer[0] = LTR553_ALS_PS_STATUS	;

//	res = i2c_master_send(client, buffer, 0x2);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	else
	{
		res = 0;
	}

	return res;

EXIT_ERR:
	APS_ERR("ltr553_clear_intr fail\n");
	return res;
}


static int ltr553_dev_init(struct i2c_client *client)
{
	int res;
	int init_ps_gain;
	int init_als_gain;
	u8 databuf[2];	
	int id1 = 0;
	int id2 = 0;

//	struct i2c_client *client = ltr553_obj->client;
	struct ltr553_priv *obj = ltr553_obj;   
	ltr553_i2c_client = client;
	
	mdelay(PON_DELAY);

	id1 = ltr553_i2c_read_reg(LTR553_PART_ID);
	id2 = ltr553_i2c_read_reg(LTR553_MANUFACTURER_ID);
	APS_ERR("id1 = %x, id2 = %x,\n",id1, id2);
    if (id1 < 0 || id2 < 0)
    {
        res = ltr553_ERR_I2C;
        goto EXIT_ERR;
    }

	// Enable PS to Gain4 at startup
	init_ps_gain = PS_RANGE16;
	ps_gainrange = init_ps_gain;

//	res = ltr553_ps_enable(init_ps_gain);
//	if (res < 0)
//		goto EXIT_ERR;


	// Enable ALS to Full Range at startup
	init_als_gain = ALS_RANGE_1300;//ALS_RANGE_8K;
	als_gainrange = init_als_gain;

//	res = ltr553_als_enable(init_als_gain);
//	if (res < 0)
//		goto EXIT_ERR;

	/*for interrup work mode support */
	if(0 == obj->hw->polling_mode_ps)
	{	
		APS_LOG("eint enable");
		ltr553_ps_set_thres();
		
		databuf[0] = LTR553_INTERRUPT;	
		databuf[1] = 0x01;
		res = ltr553_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr553_ERR_I2C;
		}

		databuf[0] = LTR553_INTERRUPT_PERSIST;	
		databuf[1] = 0x30;
		res = ltr553_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if(res <= 0)
		{
			goto EXIT_ERR;
			return ltr553_ERR_I2C;
		}

	}

	if((res = ltr553_setup_eint(client))!=0)
	{
		APS_ERR("setup eint: %d\n", res);
		return res;
	}
	
	if((res = ltr553_check_and_clear_intr(client)))
	{
		APS_ERR("check/clear intr: %d\n", res);
		//    return res;
	}

	res = 0;

	EXIT_ERR:
	APS_ERR("init dev: %d\n", res);
	return res;

}
/*----------------------------------------------------------------------------*/


static int ltr553_get_als_value(struct ltr553_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;
	APS_DBG("als  = %d\n",als); 
	for(idx = 0; idx < obj->als_level_num; idx++)
	{
		if(als < obj->hw->als_level[idx])
		{
			break;
		}
	}
	
	if(idx >= obj->als_value_num)
	{
		APS_ERR("exceed range\n"); 
		idx = obj->als_value_num - 1;
	}
	
	if(1 == atomic_read(&obj->als_deb_on))
	{
		unsigned long endt = atomic_read(&obj->als_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->als_deb_on, 0);
		}
		
		if(1 == atomic_read(&obj->als_deb_on))
		{
			invalid = 1;
		}
	}

	if(!invalid)
	{
//#if defined(MTK_AAL_SUPPORT)
		int level_high = obj->hw->als_level[idx];
		int level_low = (idx > 0) ? obj->hw->als_level[idx-1] : 0;
		int level_diff = level_high - level_low;
		int value_high = obj->hw->als_value[idx];
		int value_low = (idx > 0) ? obj->hw->als_value[idx-1] : 0;
		int value_diff = value_high - value_low;
		int value = 0;

		if ((level_low >= level_high) || (value_low >= value_high))
			value = value_low;
		else
			value = (level_diff * value_low + (als - level_low) * value_diff + ((level_diff + 1) >> 1)) / level_diff;

		APS_DBG("ALS: %d [%d, %d] => %d [%d, %d] \n", als, level_low, level_high, value, value_low, value_high);
		return value;
//#endif
    
		APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);	
		return obj->hw->als_value[idx];
	}
	else
	{
		APS_ERR("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);    
		return -1;
	}
}
/*----------------------------------------------------------------------------*/
static int ltr553_get_ps_value(struct ltr553_priv *obj, u16 ps)
{
	int val,  mask = atomic_read(&obj->ps_mask);
	int invalid = 0;

	static int val_temp = 1;
	if((ps > atomic_read(&obj->ps_thd_val_high)))
	{
		val = 0;  /*close*/
		val_temp = 0;
		intr_flag_value = 1;
	}
			//else if((ps < atomic_read(&obj->ps_thd_val_low))&&(temp_ps[0]  < atomic_read(&obj->ps_thd_val_low)))
	else if((ps < atomic_read(&obj->ps_thd_val_low)))
	{
		val = 1;  /*far away*/
		val_temp = 1;
		intr_flag_value = 0;
	}
	else
		val = val_temp;	
			
	
	if(atomic_read(&obj->ps_suspend))
	{
		invalid = 1;
	}
	else if(1 == atomic_read(&obj->ps_deb_on))
	{
		unsigned long endt = atomic_read(&obj->ps_deb_end);
		if(time_after(jiffies, endt))
		{
			atomic_set(&obj->ps_deb_on, 0);
		}
		
		if (1 == atomic_read(&obj->ps_deb_on))
		{
			invalid = 1;
		}
	}
	else if (obj->als > 50000)
	{
		//invalid = 1;
		APS_DBG("ligh too high will result to failt proximiy\n");
		return 1;  /*far away*/
	}

	if(!invalid)
	{
		APS_DBG("PS:  %05d => %05d\n", ps, val);
		return val;
	}	
	else
	{
		return -1;
	}	
}

/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*for interrup work mode support */
static void ltr553_eint_work(struct work_struct *work)
{
	struct ltr553_priv *obj = (struct ltr553_priv *)container_of(work, struct ltr553_priv, eint_work);
	int err;
	hwm_sensor_data sensor_data;
//	u8 buffer[1];
//	u8 reg_value[1];
	u8 databuf[2];
	int res = 0;
	APS_FUN();
	err = ltr553_check_intr(obj->client);
	if(err < 0)
	{
		APS_ERR("ltr553_eint_work check intrs: %d\n", err);
	}
	else
	{
		//get raw data
		obj->ps = ltr553_ps_read();
    	if(obj->ps < 0)
    	{
    		err = -1;
    		return;
    	}
				
		APS_DBG("ltr553_eint_work rawdata ps=%d als_ch0=%d!\n",obj->ps,obj->als);
		sensor_data.values[0] = ltr553_get_ps_value(obj, obj->ps);
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;			
/*singal interrupt function add*/
		APS_DBG("intr_flag_value=%d\n",intr_flag_value);
		if(intr_flag_value){
				APS_DBG(" interrupt value ps will < ps_threshold_low");

				databuf[0] = LTR553_PS_THRES_LOW_0;	
				databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
				res = ltr553_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
				if(res <= 0)
				{
					return;
				}
				databuf[0] = LTR553_PS_THRES_LOW_1;	
				databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_low)) & 0xFF00) >> 8);
				res = ltr553_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
				if(res <= 0)
				{
					return;
				}
				databuf[0] = LTR553_PS_THRES_UP_0;	
				databuf[1] = (u8)(0x00FF);
				res = ltr553_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
				if(res <= 0)
				{
					return;
				}
				databuf[0] = LTR553_PS_THRES_UP_1; 
				databuf[1] = (u8)((0xFF00) >> 8);;
				res = ltr553_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
				if(res <= 0)
				{
					return;
				}
		}
		else{	
				APS_DBG(" interrupt value ps will > ps_threshold_high");
				databuf[0] = LTR553_PS_THRES_LOW_0;	
				databuf[1] = (u8)(0 & 0x00FF);
				res = ltr553_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
				if(res <= 0)
				{
					return;
				}
				databuf[0] = LTR553_PS_THRES_LOW_1;	
				databuf[1] = (u8)((0 & 0xFF00) >> 8);
				res = ltr553_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
				if(res <= 0)
				{
					return;
				}
				databuf[0] = LTR553_PS_THRES_UP_0;	
				databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
				res = ltr553_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
				if(res <= 0)
				{
					return;
				}
				databuf[0] = LTR553_PS_THRES_UP_1; 
				databuf[1] = (u8)(((atomic_read(&obj->ps_thd_val_high)) & 0xFF00) >> 8);;
				res = ltr553_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
				if(res <= 0)
				{
					return;
				}
		}
		//let up layer to know
		if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
		{
		  APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
		}
	}
	ltr553_clear_intr(obj->client);
	mt_eint_unmask(CUST_EINT_ALS_NUM);      
}



/****************************************************************************** 
 * Function Configuration
******************************************************************************/
static int ltr553_open(struct inode *inode, struct file *file)
{
	file->private_data = ltr553_i2c_client;

	if (!file->private_data)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int ltr553_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/


static long ltr553_unlocked_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)       
{
	struct i2c_client *client = (struct i2c_client*)file->private_data;
	struct ltr553_priv *obj = i2c_get_clientdata(client);  
	long err = 0;
	void __user *ptr = (void __user*) arg;
	int dat, dat_tmp;
	uint32_t enable;
	int ps_cali;
	int threshold[2];
    unsigned int i = 0,sum = 0,data = 0;
    
	APS_DBG("cmd= %d\n", cmd); 
	switch (cmd)
	{
		case ALSPS_SET_PS_MODE:
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(enable)
			{
			    err = ltr553_ps_enable(ps_gainrange);
				if(err < 0)
				{
					APS_ERR("enable ps fail: %ld\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_PS, &obj->enable);
			}
			else
			{
			    err = ltr553_ps_disable();
				if(err < 0)
				{
					APS_ERR("disable ps fail: %ld\n", err); 
					goto err_out;
				}
				
				clear_bit(CMC_BIT_PS, &obj->enable);
			}
			break;

		case ALSPS_GET_PS_MODE:
			enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_PS_DATA:
			APS_DBG("ALSPS_GET_PS_DATA\n"); 
		    obj->ps = ltr553_ps_read();
			if(obj->ps < 0)
			{
				err = -EFAULT;
				goto err_out;
			}
			
			dat = ltr553_get_ps_value(obj, obj->ps);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_GET_PS_RAW_DATA:
			obj->ps = ltr553_ps_read();
			if(obj->ps < 0)
			{
				err = -EFAULT;
				goto err_out;
			}
			dat = obj->ps;
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}  
			break;

		case ALSPS_SET_ALS_MODE:
			if(copy_from_user(&enable, ptr, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			if(enable)
			{
			    err = ltr553_als_enable(als_gainrange);
				if(err < 0)
				{
					APS_ERR("enable als fail: %ld\n", err); 
					goto err_out;
				}
				set_bit(CMC_BIT_ALS, &obj->enable);
			}
			else
			{
			    err = ltr553_als_disable();
				if(err < 0)
				{
					APS_ERR("disable als fail: %ld\n", err); 
					goto err_out;
				}
				clear_bit(CMC_BIT_ALS, &obj->enable);
			}
			break;

		case ALSPS_GET_ALS_MODE:
			enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
			if(copy_to_user(ptr, &enable, sizeof(enable)))
			{
				err = -EFAULT;
				goto err_out;
			}
			break;

		case ALSPS_GET_ALS_DATA: 
		    obj->als = ltr553_als_read(als_gainrange);
			if(obj->als < 0)
			{
				goto err_out;
			}

			dat = ltr553_get_als_value(obj, obj->als);
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

		case ALSPS_GET_ALS_RAW_DATA:    
			obj->als = ltr553_als_read(als_gainrange);
			if(obj->als < 0)
			{
				goto err_out;
			}

			dat = obj->als;
			if(copy_to_user(ptr, &dat, sizeof(dat)))
			{
				err = -EFAULT;
				goto err_out;
			}              
			break;

        case ALSPS_IOCTL_CLR_CALI:
            if(copy_from_user(&dat, ptr, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }
            if(dat == 0)
                obj->ps_cali = 0;
            APS_ERR("%s clear ps_cali: %d , dat=%d \n", __func__, obj->ps_cali, dat);
            break;
        
        case ALSPS_IOCTL_GET_CALI:
            ps_cali = obj->ps_cali ;
            if(copy_to_user(ptr, &ps_cali, sizeof(ps_cali)))
            {
                err = -EFAULT;
                goto err_out;
            }
            APS_ERR("%s get ps_cali: %d \n", __func__, ps_cali);
            break;
        
        case ALSPS_IOCTL_SET_CALI:
            if(copy_from_user(&ps_cali, ptr, sizeof(ps_cali)))
            {
                err = -EFAULT;
                goto err_out;
            }
        
            obj->ps_cali = ps_cali;
            APS_ERR("%s set ps_cali: %d \n", __func__, ps_cali);
            break;

        case ALSPS_SET_PS_THRESHOLD:
            if(copy_from_user(threshold, ptr, sizeof(threshold)))
            {
                err = -EFAULT;
                goto err_out;
            }
            APS_ERR("%s set threshold high: 0x%x, low: 0x%x\n", __func__, threshold[0],threshold[1]); 
//            atomic_set(&obj->ps_thd_val_high,  (threshold[0]+obj->ps_cali));
//            atomic_set(&obj->ps_thd_val_low,  (threshold[1]+obj->ps_cali));//need to confirm
            break;

        case ALSPS_GET_PS_THRESHOLD_HIGH:
            threshold[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
            APS_ERR("%s get threshold high: 0x%x\n", __func__, threshold[0]); 
            if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;
            
        case ALSPS_GET_PS_THRESHOLD_LOW:
            threshold[0] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
            APS_ERR("%s get threshold low: 0x%x\n", __func__, threshold[0]); 
            if(copy_to_user(ptr, &threshold[0], sizeof(threshold[0])))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;

        case ALSPS_GET_PS_CALI:
            for(i=0; i<Calibrate_num; i++)
            {
                dat = ltr553_ps_read();
                if(dat < 0)
                {
                    err = -EFAULT;
                    goto err_out;
                }
                sum += dat;
                //   APS_LOG("ioctl ps raw CALI value = %d,sum=%d \n", dat,sum);
                data = 0;
                msleep(70);
            }
            data = (unsigned int)(sum / Calibrate_num) + LTR_HT_N_CT;
            APS_ERR("LTR553 ALSPS_GET_PS_CALI data=%d,sum=%d\n",data,sum);
            if(copy_to_user(ptr, &data, sizeof(data)))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;
        
        case ALSPS_SET_PS_CALI:
            if(copy_from_user(&dat, ptr, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }
            dat_tmp = dat;
            if(dat <= (LTR_HT_N_CT - LTR_LT_N_CT))
                dat = LTR_HT_N_CT + 20;
            if(dat >= (LTR_HT_N_CT + LTR_LT_N_CT))
                dat = (LTR_HT_N_CT + LTR_LT_N_CT);
            atomic_set(&obj->ps_thd_val_high, dat);
            atomic_set(&obj->ps_thd_val_low, (dat - (LTR_HT_N_CT - LTR_LT_N_CT)));
            obj->hw->ps_threshold_high = dat;
            obj->hw->ps_threshold_low= dat - (LTR_HT_N_CT - LTR_LT_N_CT);
//            ltr553_ps_set_thres();
            APS_ERR("LTR553 ALSPS_SET_PS_CALI dat = %d, dat_tmp =%d\n",dat,dat_tmp);
            break;

        case ALSPS_GET_PS_TEST_RESULT:
            obj->ps = ltr553_ps_read();
            if(obj->ps < 0)
            {
                err = -EFAULT;
                goto err_out;
            }

            dat = ltr553_get_ps_value(obj, obj->ps);
            if(copy_to_user(ptr, &dat, sizeof(dat)))
            {
                err = -EFAULT;
                goto err_out;
            }
            break;

		default:
			APS_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
			err = -ENOIOCTLCMD;
			break;
	}

	err_out:
	return err;    
}

/*----------------------------------------------------------------------------*/
static struct file_operations ltr553_fops = {
	//.owner = THIS_MODULE,
	.open = ltr553_open,
	.release = ltr553_release,
	.unlocked_ioctl = ltr553_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice ltr553_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &ltr553_fops,
};

#ifndef CONFIG_HAS_EARLYSUSPEND
static int ltr553_i2c_suspend(struct i2c_client *client, pm_message_t msg) 
{
	struct ltr553_priv *obj = i2c_get_clientdata(client);    
	int err;
	APS_FUN();    

	if(msg.event == PM_EVENT_SUSPEND)
	{   
		if(!obj)
		{
			APS_ERR("null pointer!!\n");
			return -EINVAL;
		}
		
		atomic_set(&obj->als_suspend, 1);
		err = ltr553_als_disable();
		if(err < 0)
		{
			APS_ERR("disable als: %d\n", err);
			return err;
		}

		atomic_set(&obj->ps_suspend, 1);
		err = ltr553_ps_disable();
		if(err < 0)
		{
			APS_ERR("disable ps:  %d\n", err);
			return err;
		}
		
		ltr553_power(obj->hw, 0);
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr553_i2c_resume(struct i2c_client *client)
{
	struct ltr553_priv *obj = i2c_get_clientdata(client);        
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	ltr553_power(obj->hw, 1);
/*	err = ltr553_devinit();
	if(err < 0)
	{
		APS_ERR("initialize client fail!!\n");
		return err;        
	}*/
	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
	    err = ltr553_als_enable(als_gainrange);
	    if (err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        
		}
	}
	atomic_set(&obj->ps_suspend, 0);
	if(test_bit(CMC_BIT_PS,  &obj->enable))
	{
		err = ltr553_ps_enable(ps_gainrange);
	    if (err < 0)
		{
			APS_ERR("enable ps fail: %d\n", err);                
		}
	}

	return 0;
}
#else
static void ltr553_early_suspend(struct early_suspend *h) 
{   /*early_suspend is only applied for ALS*/
	struct ltr553_priv *obj = container_of(h, struct ltr553_priv, early_drv);   
	int err;
	APS_FUN();    

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}
	
	atomic_set(&obj->als_suspend, 1); 
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
		err = ltr553_als_disable();
		if(err < 0)
		{
			APS_ERR("disable als fail: %d\n", err); 
		}
	}
}

static void ltr553_late_resume(struct early_suspend *h)
{   /*early_suspend is only applied for ALS*/
	struct ltr553_priv *obj = container_of(h, struct ltr553_priv, early_drv);         
	int err;
	APS_FUN();

	if(!obj)
	{
		APS_ERR("null pointer!!\n");
		return;
	}

	atomic_set(&obj->als_suspend, 0);
	if(test_bit(CMC_BIT_ALS, &obj->enable))
	{
	    err = ltr553_als_enable(als_gainrange);
		if(err < 0)
		{
			APS_ERR("enable als fail: %d\n", err);        

		}
	}
}
#endif

int ltr553_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct ltr553_priv *obj = (struct ltr553_priv *)self;

    APS_FUN();
    APS_DBG("command= %d\n", command);
	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{				
				value = *(int *)buff_in;
				if(value)
				{
				    err = ltr553_ps_enable(ps_gainrange);
					if(err < 0)
					{
						APS_ERR("enable ps fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_PS, &obj->enable);
				}
				else
				{
				    err = ltr553_ps_disable();
					if(err < 0)
					{
						APS_ERR("disable ps fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_PS, &obj->enable);
				}
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				APS_ERR("get sensor ps data !\n");
				sensor_data = (hwm_sensor_data *)buff_out;
				obj->ps = ltr553_ps_read();
    			if(obj->ps < 0)
    			{
    				err = -1;
    				break;
    			}
				sensor_data->values[0] = ltr553_get_ps_value(obj, obj->ps);
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;			
			}
			break;
		default:
			APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}

int ltr553_als_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err = 0;
	int value;
	hwm_sensor_data* sensor_data;
	struct ltr553_priv *obj = (struct ltr553_priv *)self;

	switch (command)
	{
		case SENSOR_DELAY:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Set delay parameter error!\n");
				err = -EINVAL;
			}
			// Do nothing
			break;

		case SENSOR_ENABLE:
			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				APS_ERR("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				value = *(int *)buff_in;				
				if(value)
				{
				    err = ltr553_als_enable(als_gainrange);
					if(err < 0)
					{
						APS_ERR("enable als fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_ALS, &obj->enable);
				}
				else
				{
				    err = ltr553_als_disable();
					if(err < 0)
					{
						APS_ERR("disable als fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_ALS, &obj->enable);
				}
				
			}
			break;

		case SENSOR_GET_DATA:
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				APS_ERR("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				APS_ERR("get sensor als data !\n");
				sensor_data = (hwm_sensor_data *)buff_out;
				obj->als = ltr553_als_read(als_gainrange);
                #if defined(MTK_AAL_SUPPORT)
				sensor_data->values[0] = obj->als;
				#else
				sensor_data->values[0] = ltr553_get_als_value(obj, obj->als);
				#endif
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
			}
			break;
		default:
			APS_ERR("light sensor operate function no this parameter %d!\n", command);
			err = -1;
			break;
	}
	
	return err;
}


/*----------------------------------------------------------------------------*/
static int ltr553_i2c_detect(struct i2c_client *client, struct i2c_board_info *info) 
{    
	strcpy(info->type, LTR553_DEV_NAME);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr553_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ltr553_priv *obj;
	struct hwmsen_object obj_ps, obj_als;
	int err = 0;

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	ltr553_obj = obj;

	obj->hw = ltr553_get_cust_alsps_hw();
	ltr553_get_addr(obj->hw, &obj->addr);

	INIT_WORK(&obj->eint_work, ltr553_eint_work);
	obj->client = client;
	obj->client->timing = 400;

	i2c_set_clientdata(client, obj);	
	atomic_set(&obj->als_debounce, 300);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 300);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->ps_suspend, 0);
	atomic_set(&obj->ps_thd_val_high,  obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low,  obj->hw->ps_threshold_low);
	//atomic_set(&obj->als_cmd_val, 0xDF);
	//atomic_set(&obj->ps_cmd_val,  0xC1);
	atomic_set(&obj->ps_thd_val,  obj->hw->ps_threshold);
	obj->enable = 0;
	obj->pending_intr = 0;
	obj->ps_cali = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);   
	obj->als_modulus = (400*100)/(16*150);//(1/Gain)*(400/Tine), this value is fix after init ATIME and CONTROL register value
										//(400)/16*2.72 here is amplify *100
	atomic_set(&obj->firlen, MAX_FIR_LENGTH);
	if (atomic_read(&obj->firlen) > 0)
	{
		atomic_set(&obj->fir_en, 0);
		obj->fir.num = 0;
		obj->fir.idx = 0;
		obj->fir.sum = 0;
	}

	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	set_bit(CMC_BIT_ALS, &obj->enable);
	set_bit(CMC_BIT_PS, &obj->enable);

	APS_LOG("ltr553_dev_init() start...!\n");
	if(err = ltr553_dev_init(client))
	{
		goto exit_init_failed;
	}
	APS_LOG("ltr553_dev_init() ...OK!\n");

	//printk("@@@@@@ manufacturer value:%x\n",ltr553_i2c_read_reg(0x87));

	if(err = misc_register(&ltr553_device))
	{
		APS_ERR("ltr553_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	
	/* Register sysfs attribute */
#if 0
	if(err = ltr553_create_attr(&ltr553_alsps_driver.driver))
	{
		printk(KERN_ERR "create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
#endif
	if(err = ltr553_create_attr(&(ltr553_init_info.platform_diver_addr->driver)))
	{
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
	obj_ps.self = ltr553_obj;
	/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
	if(1 == obj->hw->polling_mode_ps)
	{
		obj_ps.polling = 1;
	}
	else
	{
		obj_ps.polling = 0;
	}
	obj_ps.sensor_operate = ltr553_ps_operate;
	if(err = hwmsen_attach(ID_PROXIMITY, &obj_ps))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	
	obj_als.self = ltr553_obj;
	obj_als.polling = 1;
	obj_als.sensor_operate = ltr553_als_operate;
	if(err = hwmsen_attach(ID_LIGHT, &obj_als))
	{
		APS_ERR("attach fail = %d\n", err);
		goto exit_create_attr_failed;
	}

#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = ltr553_early_suspend,
	obj->early_drv.resume   = ltr553_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif
	ltr553_init_flag =0;//sen.luo

	APS_LOG("%s: OK\n", __func__);
	return 0;

	exit_create_attr_failed:
	misc_deregister(&ltr553_device);
	exit_misc_device_register_failed:
	exit_init_failed:
	//i2c_detach_client(client);
	exit_kfree:
	kfree(obj);
	exit:
	ltr553_i2c_client = NULL;           
//	MT6516_EINTIRQMask(CUST_EINT_ALS_NUM);  /*mask interrupt if fail*/
	ltr553_init_flag =-1;//sen.luo
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}

/*----------------------------------------------------------------------------*/

static int ltr553_i2c_remove(struct i2c_client *client)
{
	int err;	
#if 0
	if(err = ltr553_delete_attr(&ltr553_i2c_driver.driver))
	{
		APS_ERR("ltr553_delete_attr fail: %d\n", err);
	} 
#endif
	if(err = ltr553_delete_attr(&(ltr553_init_info.platform_diver_addr->driver)))
	{
		APS_ERR("ltr553_delete_attr fail: %d\n", err);
	} 
	
	if(err = misc_deregister(&ltr553_device))
	{
		APS_ERR("misc_deregister fail: %d\n", err);    
	}
	
	ltr553_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr553_probe(struct platform_device *pdev) 
{
	struct alsps_hw *hw = ltr553_get_cust_alsps_hw();

	ltr553_power(hw, 1);
	//ltr553_force[0] = hw->i2c_num;
	//ltr553_force[1] = hw->i2c_addr[0];
	//APS_DBG("I2C = %d, addr =0x%x\n",ltr553_force[0],ltr553_force[1]);
	if(i2c_add_driver(&ltr553_i2c_driver))
	{
		APS_ERR("add driver error\n");
		return -1;
	} 
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ltr553_remove(struct platform_device *pdev)
{
	struct alsps_hw *hw = ltr553_get_cust_alsps_hw();
	APS_FUN();    
	ltr553_power(hw, 0);    
	i2c_del_driver(&ltr553_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver ltr553_alsps_driver = {
	.probe      = ltr553_probe,
	.remove     = ltr553_remove,    
	.driver     = {
		.name  = "als_ps",
		//.owner = THIS_MODULE,
	}
};
static int  ltr553_local_init(void)
{
		struct alsps_hw *hw = ltr553_get_cust_alsps_hw();
		ltr553_power(hw, 1);
		if(i2c_add_driver(&ltr553_i2c_driver))
		{
			APS_ERR("add driver error\n");
			return -1;
		}
		if(-1 == ltr553_init_flag)  
		{
			return -1;
		}
		return 0;
}
static int ltr_remove(void)
{
		struct alsps_hw *hw = ltr553_get_cust_alsps_hw();
		APS_FUN();
		ltr553_power(hw, 0);
		i2c_del_driver(&ltr553_i2c_driver);
		return 0;
}
/*----------------------------------------------------------------------------*/
static int __init ltr553_init(void)
{
	struct alsps_hw *hw = ltr553_get_cust_alsps_hw();
	APS_FUN();
	APS_ERR("%s: i2c_number=%d\n", __func__,hw->i2c_num); 
	i2c_register_board_info(hw->i2c_num, &i2c_ltr553, 1);
#if 0
	if(platform_driver_register(&ltr558_alsps_driver))
	{
		APS_ERR("failed to register driver");
		return -ENODEV;
	}
#else
	hwmsen_alsps_sensor_add(&ltr553_init_info);
#endif
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit ltr553_exit(void)
{
	APS_FUN();
	//platform_driver_unregister(&ltr553_alsps_driver);
}
/*----------------------------------------------------------------------------*/
module_init(ltr553_init);
module_exit(ltr553_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("XX Xx");
MODULE_DESCRIPTION("LTR-553ALS Driver");
MODULE_LICENSE("GPL");

