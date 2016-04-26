/* touchscreen/ektf2k_kthread_mtk.c - ELAN EKTF2K touchscreen driver
 * for MTK65xx serial platform.
 *
 * Copyright (C) 2012 Elan Microelectronics Corporation.
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
 * 2011/12/06: The first release, version 0x0001
 * 2012/2/15:  The second release, version 0x0002 for new bootcode
 * 2012/5/8:   Release version 0x0003 for china market
 *             Integrated 2 and 5 fingers driver code together and
 *             auto-mapping resolution.
 * 2012/8/24:  MTK version
 * 2013/2/1:   Release for MTK6589/6577/6575/6573/6513 Platform
 *             For MTK6575/6573/6513, please disable both of ELAN_MTK6577 and MTK6589DMA.
 *                          It will use 8+8+2 received packet protocol
 *             For MTK6577, please enable ELAN_MTK6577 and disable MTK6589DMA.
 *                          It will use Elan standard protocol (18bytes of protocol).
 *             For MTK6589, please enable both of ELAN_MTK6577 and MTK6589DMA.
 * 2013/5/15   Fixed MTK6589_DMA issue.
 */

//#define TP_DBLCLIK_RESUM //add for Double click to resume 2014/3/5
//#define SOFTKEY_AXIS_VER
#define ELAN_TEN_FINGERS
#define MTK6589_DMA
#define ELAN_MTK6577
#define ELAN_BUTTON
#define TPD_HAVE_BUTTON

#ifdef ELAN_TEN_FINGERS
#define PACKET_SIZE		35		/* support 10 fingers packet */
#else
//#define PACKET_SIZE		8 		/* support 2 fingers packet  */
#define PACKET_SIZE		18			/* support 5 fingers packet  */
#endif

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
//#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>
#include <linux/hrtimer.h>

#include <linux/dma-mapping.h>

#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#ifndef TPD_NO_GPIO
#include "cust_gpio_usage.h"
#endif

// for linux 2.6.36.3
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <asm/ioctl.h>

//dma
#include <linux/dma-mapping.h>

#ifdef TP_DBLCLIK_RESUM
#include <asm/atomic.h>
#endif

//#include "ekth3250.h"
#include "tpd.h"
//#include "mach/eint.h"

//#include "tpd_custom_ekth3250.h"
#include "ektf3248_tpd_driver.h"

#include <cust_eint.h>



//#define ELAN_DEBUG

#if defined(ELAN_DEBUG)
#define pr_tp(format, args...) printk("<0>" format, ##args)
#define pr_k(format, args...) printk("<0>" format, ##args)
#define pr_ch(format, args...)                      \
    printk("<0>" "%s <%d>,%s(),cheehwa_print:\n\t"  \
           format,__FILE__,__LINE__,__func__, ##args)

#else
#define pr_tp(format, args...)  do {} while (0)
#define pr_ch(format, args...)  do {} while (0)
#undef pr_k(format, args...)
#define pr_k(format, args...)  do {} while (0)
#endif

#define PWR_STATE_DEEP_SLEEP	0
#define PWR_STATE_NORMAL		1
#define PWR_STATE_MASK			BIT(3)

#define CMD_S_PKT			0x52
#define CMD_R_PKT			0x53
#define CMD_W_PKT			0x54

#define HELLO_PKT			0x55
#define FIVE_FINGERS_PKT			0x5D
#define MTK_FINGERS_PKT                  0x6D    /** 2 Fingers: 5A 5 Fingers 5D, 10 Fingers: 62 **/

#define TWO_FINGERS_PKT      0x5A
#define MTK_FINGERS_PKT       0x6D
#define TEN_FINGERS_PKT	0x62

#define RESET_PKT			0x77
#define CALIB_PKT			0xA8


#define TPD_OK 0
//#define HAVE_TOUCH_KEY

#define LCT_VIRTUAL_KEY

#ifdef MTK6589_DMA
static uint8_t *gpDMABuf_va = NULL;
static uint32_t gpDMABuf_pa = NULL;
#endif

#define ELAN_ESD_CHECK
#ifdef ELAN_ESD_CHECK
    static struct workqueue_struct *esd_wq = NULL;
    static struct delayed_work esd_work;
    static atomic_t elan_cmd_response = ATOMIC_INIT(0);
    static unsigned long m_delay = 2*HZ;
#endif


#ifdef TPD_HAVE_BUTTON
#define TPD_KEY_COUNT           3
#define TPD_KEYS                { KEY_APP_SWITCH, KEY_HOMEPAGE, KEY_BACK}
//#define TPD_KEYS_DIM            {{107,1370,109,TPD_BUTTON_HEIGH},{365,1370,109,TPD_BUTTON_HEIGH},{617,1370,102,TPD_BUTTON_HEIGH}} //robin modify
#define TPD_KEYS_DIM            {{180,2100,360,100},{540,2100,360,100},{900,2100,360,100}} //robin modify

static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif

// modify
#define SYSTEM_RESET_PIN_SR 	135

//Add these Define

#define IAP_PORTION   0
#define PAGERETRY  30
#define IAPRESTART 5
#define CMD_54001234	   0


// For Firmware Update 
#define ELAN_IOCTLID	0xD0
#define IOCTL_I2C_SLAVE	_IOW(ELAN_IOCTLID,  1, int)
#define IOCTL_MAJOR_FW_VER  _IOR(ELAN_IOCTLID, 2, int)
#define IOCTL_MINOR_FW_VER  _IOR(ELAN_IOCTLID, 3, int)
#define IOCTL_RESET  _IOR(ELAN_IOCTLID, 4, int)
#define IOCTL_IAP_MODE_LOCK  _IOR(ELAN_IOCTLID, 5, int)
#define IOCTL_CHECK_RECOVERY_MODE  _IOR(ELAN_IOCTLID, 6, int)
#define IOCTL_FW_VER  _IOR(ELAN_IOCTLID, 7, int)
#define IOCTL_X_RESOLUTION  _IOR(ELAN_IOCTLID, 8, int)
#define IOCTL_Y_RESOLUTION  _IOR(ELAN_IOCTLID, 9, int)
#define IOCTL_FW_ID  _IOR(ELAN_IOCTLID, 10, int)
#define IOCTL_ROUGH_CALIBRATE  _IOR(ELAN_IOCTLID, 11, int)
#define IOCTL_IAP_MODE_UNLOCK  _IOR(ELAN_IOCTLID, 12, int)
#define IOCTL_I2C_INT  _IOR(ELAN_IOCTLID, 13, int)
#define IOCTL_RESUME  _IOR(ELAN_IOCTLID, 14, int)
#define IOCTL_POWER_LOCK  _IOR(ELAN_IOCTLID, 15, int)
#define IOCTL_POWER_UNLOCK  _IOR(ELAN_IOCTLID, 16, int)
#define IOCTL_FW_UPDATE  _IOR(ELAN_IOCTLID, 17, int)
#define IOCTL_BC_VER  _IOR(ELAN_IOCTLID, 18, int)
#define IOCTL_2WIREICE  _IOR(ELAN_IOCTLID, 19, int)
#define IOCTL_GET_UPDATE_PROGREE	_IOR(CUSTOMER_IOCTLID,  2, int)


#define CUSTOMER_IOCTLID	0xA0
#define IOCTL_CIRCUIT_CHECK  _IOR(CUSTOMER_IOCTLID, 1, int)


extern struct tpd_device *tpd;

static uint8_t RECOVERY=0x00;
static int FW_VERSION=0x00;
static int X_RESOLUTION=832; //0x00;  
static int Y_RESOLUTION=1536; //0x00;
static int FW_ID=0x00;
static int BC_VERSION = 0x00;
static int work_lock=0x00;
static int power_lock=0x00;
static int circuit_ver=0x01;
static int button_state = 0;
/*++++i2c transfer start+++++++*/
static int file_fops_addr=0x10;
/*++++i2c transfer end+++++++*/
static int tpd_down_flag=0;

static struct i2c_client *i2c_client = NULL;
static struct task_struct *thread = NULL;

#ifdef TP_DBLCLIK_RESUM
static atomic_t resume_flag;
#endif

static DECLARE_WAIT_QUEUE_HEAD(waiter);
static inline int elan_ktf2k_ts_parse_xy(uint8_t *data,
			uint16_t *x, uint16_t *y);
extern void mt_eint_unmask(unsigned int line);
extern void mt_eint_mask(unsigned int line);
extern void mt_eint_set_hw_debounce(unsigned int eintno, unsigned int ms);
extern unsigned int mt_eint_set_sens(unsigned int eintno, unsigned int sens);
extern void mt_eint_registration(unsigned int eint_num, unsigned int flag, 
              void (EINT_FUNC_PTR) (void), unsigned int is_auto_umask);


static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);


static int tpd_flag = 0;
static int boot_normal_flag = 1;

#if 0
static int key_pressed = -1;

struct osd_offset{
	int left_x;
	int right_x;
	unsigned int key_event;
};

static struct osd_offset OSD_mapping[] = { // Range need define by Case!
	{35, 290,  KEY_MENU},	//menu_left_x, menu_right_x, KEY_MENU
	{303, 467, KEY_HOME},	//home_left_x, home_right_x, KEY_HOME
	{473, 637, KEY_BACK},	//back_left_x, back_right_x, KEY_BACK
	{641, 905, KEY_SEARCH},	//search_left_x, search_right_x, KEY_SEARCH
};
#endif 

#if IAP_PORTION
uint8_t ic_status=0x00;	//0:OK 1:master fail 2:slave fail
int update_progree=0;
uint8_t I2C_DATA[3] = {0x10, 0x20, 0x21};/*I2C devices address*/  
int is_OldBootCode = 0; // 0:new 1:old



/*The newest firmware, if update must be changed here*/
static uint8_t file_fw_data[] = {
#include "Cs_fw_data.i"
};


enum
{
	PageSize		= 132,
	PageNum		    = 249,
	ACK_Fail		= 0x00,
	ACK_OK			= 0xAA,
	ACK_REWRITE		= 0x55,
};

enum
{
	E_FD			= -1,
};
#endif


//Add 0821 start
static const struct i2c_device_id tpd_id[] = 
{
	{ "ektf3k_mtk", 0 },
	{ }
};

#ifdef ELAN_MTK6577
	static struct i2c_board_info __initdata i2c_tpd={ I2C_BOARD_INFO("ektf3k_mtk", (0x20>>1))};
#else
	unsigned short force[] = {0, 0x20, I2C_CLIENT_END, I2C_CLIENT_END};
	static const unsigned short *const forces[] = { force, NULL };
	//static struct i2c_client_address_data addr_data = { .forces = forces, };
#endif

static struct i2c_driver tpd_i2c_driver =
{
    .driver = {
        .name = "ektf3k_mtk",
        .owner = THIS_MODULE,
    },
    .probe = tpd_probe,
    .remove =  tpd_remove,
    .id_table = tpd_id,
    .detect = tpd_detect,
    //.address_data = &addr_data,
};
//Add 0821 end



struct elan_ktf2k_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct workqueue_struct *elan_wq;
	struct work_struct work;
	struct early_suspend early_suspend;
	int intr_gpio;
// Firmware Information
	int fw_ver;
	int fw_id;
	int bc_ver;
	int x_resolution;
	int y_resolution;
// For Firmare Update 
	struct miscdevice firmware;
	struct hrtimer timer;
};

static struct elan_ktf2k_ts_data *private_ts;
static int __fw_packet_handler(struct i2c_client *client);
static int elan_ktf2k_ts_rough_calibrate(struct i2c_client *client);
static int tpd_resume(struct i2c_client *client);

#if IAP_PORTION
int Update_FW_One(/*struct file *filp,*/ struct i2c_client *client, int recovery);
static int __hello_packet_handler(struct i2c_client *client);
int IAPReset();
#endif


static int elan_i2c_recv_data(struct i2c_client *client, uint8_t *buf, uint8_t len)
{
    int rc = 0, i = 0;
	if(buf == NULL){
		printk("[elan] BUFFER is NULL!!!!!\n");
		return -1;
	}
	
	memset(buf, 0, len);
#ifdef MTK6589_DMA
	if(gpDMABuf_va == NULL){
		printk("[elan] gpDMABuf_va is NULL!!!!!\n");
		return -1;
	}
	client->addr = client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG | I2C_ENEXT_FLAG;
	rc = i2c_master_recv(client, gpDMABuf_pa, len);
	
	if(rc >= 0){
	    for(i = 0 ; i < len; i++){
			buf[i] = gpDMABuf_va[i];
		}
	}
#else
	rc = i2c_master_recv(client, buf, len);
#endif
    return rc;
}

static int elan_i2c_send_data(struct i2c_client *client, uint8_t *buf, uint8_t len)
{
    int rc = 0, i = 0;
	if(buf == NULL){
		printk("[elan] BUFFER is NULL!!!!!\n");
		return -1;
	}
	
#ifdef MTK6589_DMA
	if(gpDMABuf_va == NULL){
		printk("[elan] gpDMABuf_va is NULL!!!!!\n");
		return -1;
	}
	for(i = 0 ; i < len; i++){
		gpDMABuf_va[i] = buf[i];
	}
	client->addr = client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG | I2C_ENEXT_FLAG;
	rc = i2c_master_send(client, gpDMABuf_pa, len);
#else
	rc = i2c_master_send(client, buf, len);
#endif
    return rc;
}

// For Firmware Update 
static int elan_iap_open(struct inode *inode, struct file *filp){ 

	pr_tp("[ELAN]into elan_iap_open\n");
		if (private_ts == NULL)  pr_tp("private_ts is NULL~~~");
		
	return 0;
}

static int elan_iap_release(struct inode *inode, struct file *filp){    
	return 0;
}

static ssize_t elan_iap_write(struct file *filp, const char *buff, size_t count, loff_t *offp){  
    int ret;
    char *tmp;

    pr_tp("[ELAN]into elan_iap_write\n");
    if (count > 8192)
        count = 8192;

    tmp = kmalloc(count, GFP_KERNEL);
    
    if (tmp == NULL)
        return -ENOMEM;
	
    if (copy_from_user(tmp, buff, count)) {
        return -EFAULT;
    }    
    ret = elan_i2c_send_data(private_ts->client, tmp, count);
	
    kfree(tmp);
    return (ret == 1) ? count : ret;

}

static ssize_t elan_iap_read(struct file *filp, char *buff, size_t count, loff_t *offp){    
    char *tmp;
    int ret;  
    long rc;

    pr_tp("[ELAN]into elan_iap_read\n");
    if (count > 8192)
        count = 8192;

    tmp = kmalloc(count, GFP_KERNEL);

    if (tmp == NULL)
        return -ENOMEM;
	
    ret = elan_i2c_recv_data(private_ts->client, tmp, count);
    if (ret >= 0)
        rc = copy_to_user(buff, tmp, count);    
 	
    kfree(tmp);
	
    return (ret == 1) ? count : ret;
	
}

static long elan_iap_ioctl(/*struct inode *inode,*/ struct file *filp,    unsigned int cmd, unsigned long arg){

	int __user *ip = (int __user *)arg;
	pr_tp("[ELAN]into elan_iap_ioctl\n");
	pr_tp("cmd value %x\n",cmd);
	
	switch (cmd) {        
		case IOCTL_I2C_SLAVE: 
			private_ts->client->addr = (int __user)arg;
			private_ts->client->addr &= I2C_MASK_FLAG; 
			private_ts->client->addr |= I2C_ENEXT_FLAG;
			//file_fops_addr = 0x15;
			break;   
		case IOCTL_MAJOR_FW_VER:            
			break;        
		case IOCTL_MINOR_FW_VER:            
			break;        
		case IOCTL_RESET:

            mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
            mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
            mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
            mdelay(10);
		//	#if !defined(EVB)
    				mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
		//	#endif
            mdelay(10);
            mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );

			break;
		case IOCTL_IAP_MODE_LOCK:
			if(work_lock==0)
			{
				pr_tp("[elan]%s %x=IOCTL_IAP_MODE_LOCK\n", __func__,IOCTL_IAP_MODE_LOCK);
				work_lock=1;
				disable_irq(CUST_EINT_TOUCH_PANEL_NUM);
				mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
				//cancel_work_sync(&private_ts->work);
			}
			break;
		case IOCTL_IAP_MODE_UNLOCK:
			if(work_lock==1)
			{			
				work_lock=0;
				enable_irq(CUST_EINT_TOUCH_PANEL_NUM);
				mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
			}
			break;
		case IOCTL_CHECK_RECOVERY_MODE:
			return RECOVERY;
			break;
		case IOCTL_FW_VER:
			__fw_packet_handler(private_ts->client);
			return FW_VERSION;
			break;
		case IOCTL_X_RESOLUTION:
			__fw_packet_handler(private_ts->client);
			return X_RESOLUTION;
			break;
		case IOCTL_Y_RESOLUTION:
			__fw_packet_handler(private_ts->client);
			return Y_RESOLUTION;
			break;
		case IOCTL_FW_ID:
			__fw_packet_handler(private_ts->client);
			return FW_ID;
			break;
		case IOCTL_ROUGH_CALIBRATE:
			return elan_ktf2k_ts_rough_calibrate(private_ts->client);
		case IOCTL_I2C_INT:
			put_user(mt_get_gpio_in(GPIO_CTP_EINT_PIN),ip);
			pr_tp("[elan]GPIO_CTP_EINT_PIN = %d\n", mt_get_gpio_in(GPIO_CTP_EINT_PIN));

			break;	
		case IOCTL_RESUME:
			tpd_resume(private_ts->client);
			break;	
		case IOCTL_CIRCUIT_CHECK:
			return circuit_ver;
			break;
		case IOCTL_POWER_LOCK:
			power_lock=1;
			break;
		case IOCTL_POWER_UNLOCK:
			power_lock=0;
			break;
#if IAP_PORTION		
		case IOCTL_GET_UPDATE_PROGREE:
			update_progree=(int __user)arg;
			break; 

		case IOCTL_FW_UPDATE:
			//RECOVERY = IAPReset(private_ts->client);
			RECOVERY=0;
			Update_FW_One(private_ts->client, RECOVERY);
#endif
		case IOCTL_BC_VER:
			__fw_packet_handler(private_ts->client);
			return BC_VERSION;
			break;
		default:            
			break;   
	}       
	return 0;
}

static struct file_operations elan_touch_fops = {    
        .open =         elan_iap_open,    
        .write =        elan_iap_write,    
        .read = 	elan_iap_read,    
        .release =	elan_iap_release,    
	.unlocked_ioctl=elan_iap_ioctl, 
 };
#if IAP_PORTION
int EnterISPMode(struct i2c_client *client, uint8_t  *isp_cmd)
{
	char buff[4] = {0};
	int len = 0;
	
	len = elan_i2c_send_data(private_ts->client, isp_cmd,  sizeof(isp_cmd));
	if (len != sizeof(buff)) {
		pr_tp("[ELAN] ERROR: EnterISPMode fail! len=%d\r\n", len);
		return -1;
	}
	else
		pr_tp("[ELAN] IAPMode write data successfully! cmd = [%2x, %2x, %2x, %2x]\n", isp_cmd[0], isp_cmd[1], isp_cmd[2], isp_cmd[3]);
	return 0;
}

int ExtractPage(struct file *filp, uint8_t * szPage, int byte)
{
	int len = 0;

	len = filp->f_op->read(filp, szPage,byte, &filp->f_pos);
	if (len != byte) 
	{
		pr_tp("[ELAN] ExtractPage ERROR: read page error, read error. len=%d\r\n", len);
		return -1;
	}

	return 0;
}

int WritePage(uint8_t * szPage, int byte)
{
	int len = 0;

	len = elan_i2c_send_data(private_ts->client, szPage,  byte);
	if (len != byte) 
	{
		pr_tp("[ELAN] ERROR: write page error, write error. len=%d\r\n", len);
		return -1;
	}

	return 0;
}

int GetAckData(struct i2c_client *client)
{
	int len = 0;

	char buff[2] = {0};
	
	len=elan_i2c_recv_data(private_ts->client, buff, sizeof(buff));
	if (len != sizeof(buff)) {
		pr_tp("[ELAN] ERROR: read data error, write 50 times error. len=%d\r\n", len);
		return -1;
	}

	pr_tp("[ELAN] GetAckData:%x,%x\n",buff[0],buff[1]);
	if (buff[0] == 0xaa/* && buff[1] == 0xaa*/) 
		return ACK_OK;
	else if (buff[0] == 0x55 && buff[1] == 0x55)
		return ACK_REWRITE;
	else
		return ACK_Fail;

	return 0;
}

void print_progress(int page, int ic_num, int j)
{
	int i, percent,page_tatol,percent_tatol;
	char str[256];
	str[0] = '\0';
	for (i=0; i<((page)/10); i++) {
		str[i] = '#';
		str[i+1] = '\0';
	}
	
	page_tatol=page+249*(ic_num-j);
	percent = ((100*page)/(249));
	percent_tatol = ((100*page_tatol)/(249*ic_num));

	if ((page) == (249))
		percent = 100;

	if ((page_tatol) == (249*ic_num))
		percent_tatol = 100;		

	pr_tp("\rprogress %s| %d%%", str, percent);
	
	if (page == (249))
		pr_tp("\n");
}
/* 
* Restet and (Send normal_command ?)
* Get Hello Packet
*/
int  IAPReset()
{
			int res;

	   		mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
 			mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
    		mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
			mdelay(10);
		//	#if !defined(EVB)
    		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
	    //	#endif
	   		mdelay(10);
    		mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
			return 1;

#if 0
	pr_tp("[ELAN] read Hello packet data!\n"); 	  
	res= __hello_packet_handler(client);
	return res;
#endif 
}

/* Check Master & Slave is "55 aa 33 cc" */
int CheckIapMode(void)
{
	char buff[4] = {0},len = 0;
	//WaitIAPVerify(1000000);
	//len = read(fd, buff, sizeof(buff));
	len=elan_i2c_recv_data(private_ts->client, buff, sizeof(buff));
	if (len != sizeof(buff)) 
	{
		pr_tp("[ELAN] CheckIapMode ERROR: read data error,len=%d\r\n", len);
		return -1;
	}
	else
	{
		
		if (buff[0] == 0x55 && buff[1] == 0xaa && buff[2] == 0x33 && buff[3] == 0xcc)
		{
			//pr_tp("[ELAN] CheckIapMode is 55 aa 33 cc\n");
			return 0;
		}
		else// if ( j == 9 )
		{
			pr_tp("[ELAN] Mode= 0x%x 0x%x 0x%x 0x%x\r\n", buff[0], buff[1], buff[2], buff[3]);
			pr_tp("[ELAN] ERROR:  CheckIapMode error\n");
			return -1;
		}
	}
	pr_tp("\n");	
}

int Update_FW_One(struct i2c_client *client, int recovery)
{
	int res = 0,ic_num = 1;
	int iPage = 0, rewriteCnt = 0; //rewriteCnt for PAGE_REWRITE
	int i = 0;
	uint8_t data;

	int restartCnt = 0, checkCnt = 0; // For IAP_RESTART
	//uint8_t recovery_buffer[4] = {0};
	int byte_count;
	uint8_t *szBuff = NULL;
	int curIndex = 0;
#if CMD_54001234
	uint8_t isp_cmd[] = {0x54, 0x00, 0x12, 0x34};	 //54 00 12 34
#else
	uint8_t isp_cmd[] = {0x45, 0x49, 0x41, 0x50};	 //45 49 41 50
#endif
	uint8_t recovery_buffer[4] = {0};

IAP_RESTART:	

	data=I2C_DATA[0];//Master
	dev_dbg(&client->dev, "[ELAN] %s: address data=0x%x \r\n", __func__, data);

//	if(recovery != 0x80)
//	{
		pr_tp("[ELAN] Firmware upgrade normal mode !\n");

		IAPReset();
	        mdelay(40);	

		res = EnterISPMode(private_ts->client, isp_cmd);	 //enter ISP mode

	res = elan_i2c_recv_data(private_ts->client, recovery_buffer, 4);   //55 aa 33 cc 
	pr_tp("[ELAN] recovery byte data:%x,%x,%x,%x \n",recovery_buffer[0],recovery_buffer[1],recovery_buffer[2],recovery_buffer[3]);			

        mdelay(10);
#if 0
		//Check IC's status is IAP mode(55 aa 33 cc) or not
		res = CheckIapMode();	 //Step 1 enter ISP mode
		if (res == -1) //CheckIapMode fail
		{	
			checkCnt ++;
			if (checkCnt >= 5)
			{
				pr_tp("[ELAN] ERROR: CheckIapMode %d times fails!\n", IAPRESTART);
				return E_FD;
			}
			else
			{
				pr_tp("[ELAN] CheckIapMode retry %dth times! And restart IAP~~~\n\n", checkCnt);
				goto IAP_RESTART;
			}
		}
		else
			pr_tp("[ELAN]  CheckIapMode ok!\n");
#endif
//	} else
//		pr_tp("[ELAN] Firmware upgrade recovery mode !\n");
	// Send Dummy Byte	
	pr_tp("[ELAN] send one byte data:%x,%x",private_ts->client->addr,data);
	res = elan_i2c_send_data(private_ts->client, &data,  sizeof(data));
	if(res!=sizeof(data))
	{
		pr_tp("[ELAN] dummy error code = %d\n",res);
	}	
	mdelay(50);


	// Start IAP
	for( iPage = 1; iPage <= PageNum; iPage++ ) 
	{
PAGE_REWRITE:
#if 1 
		// 8byte mode
		//szBuff = fw_data + ((iPage-1) * PageSize); 
		for(byte_count=1;byte_count<=17;byte_count++)
		{
			if(byte_count!=17)
			{		
	//			pr_tp("[ELAN] byte %d\n",byte_count);	
	//			pr_tp("curIndex =%d\n",curIndex);
				szBuff = file_fw_data + curIndex;
				curIndex =  curIndex + 8;

				//ioctl(fd, IOCTL_IAP_MODE_LOCK, data);
				res = WritePage(szBuff, 8);
			}
			else
			{
	//			pr_tp("byte %d\n",byte_count);
	//			pr_tp("curIndex =%d\n",curIndex);
				szBuff = file_fw_data + curIndex;
				curIndex =  curIndex + 4;
				//ioctl(fd, IOCTL_IAP_MODE_LOCK, data);
				res = WritePage(szBuff, 4); 
			}
		} // end of for(byte_count=1;byte_count<=17;byte_count++)
#endif 
#if 0 // 132byte mode		
		szBuff = file_fw_data + curIndex;
		curIndex =  curIndex + PageSize;
		res = WritePage(szBuff, PageSize);
#endif
#if 0
		if(iPage==249 || iPage==1)
		{
			mdelay(300); 			 
		}
		else
		{
			mdelay(50); 			 
		}
#endif	
		res = GetAckData(private_ts->client);

		if (ACK_OK != res) 
		{
			mdelay(50); 
			pr_tp("[ELAN] ERROR: GetAckData fail! res=%d\r\n", res);
			if ( res == ACK_REWRITE ) 
			{
				rewriteCnt = rewriteCnt + 1;
				if (rewriteCnt == PAGERETRY)
				{
					pr_tp("[ELAN] ID 0x%02x %dth page ReWrite %d times fails!\n", data, iPage, PAGERETRY);
					return E_FD;
				}
				else
				{
					pr_tp("[ELAN] ---%d--- page ReWrite %d times!\n",  iPage, rewriteCnt);
					curIndex = curIndex - PageSize;
					goto PAGE_REWRITE;
				}
			}
			else
			{
				restartCnt = restartCnt + 1;
				if (restartCnt >= 5)
				{
					pr_tp("[ELAN] ID 0x%02x ReStart %d times fails!\n", data, IAPRESTART);
					return E_FD;
				}
				else
				{
					pr_tp("[ELAN] ===%d=== page ReStart %d times!\n",  iPage, restartCnt);
					goto IAP_RESTART;
				}
			}
		}
		else
		{       pr_tp("  data : 0x%02x ",  data);  
			rewriteCnt=0;
			print_progress(iPage,ic_num,i);
		}

		mdelay(10);
	} // end of for(iPage = 1; iPage <= PageNum; iPage++)

	//if (IAPReset() > 0)
		pr_tp("[ELAN] Update ALL Firmware successfully!\n");
	return 0;
}

#endif
// End Firmware Update


#if 0
static void elan_ktf2k_ts_early_suspend(struct early_suspend *h);
static void elan_ktf2k_ts_late_resume(struct early_suspend *h);
#endif

static ssize_t elan_ktf2k_gpio_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct elan_ktf2k_ts_data *ts = private_ts;

	//ret = gpio_get_value(ts->intr_gpio);
	ret = mt_get_gpio_in(GPIO_CTP_EINT_PIN);
	pr_tp(KERN_DEBUG "GPIO_TP_INT_N=%d\n", ts->intr_gpio);
	sprintf(buf, "GPIO_TP_INT_N=%d\n", ret);
	ret = strlen(buf) + 1;
	return ret;
}

static DEVICE_ATTR(gpio, S_IRUGO, elan_ktf2k_gpio_show, NULL);

static ssize_t elan_ktf2k_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct elan_ktf2k_ts_data *ts = private_ts;

	sprintf(buf, "%s_x%4.4x\n", "ELAN_KTF2K", ts->fw_ver);
	ret = strlen(buf) + 1;
	return ret;
}
#if 0
static DEVICE_ATTR(vendor, S_IRUGO, elan_ktf2k_vendor_show, NULL);

static struct kobject *android_touch_kobj;

static int elan_ktf2k_touch_sysfs_init(void)
{
	int ret ;

	android_touch_kobj = kobject_create_and_add("android_touch", NULL) ;
	if (android_touch_kobj == NULL) {
		pr_tp(KERN_ERR "[elan]%s: subsystem_register failed\n", __func__);
		ret = -ENOMEM;
		return ret;
	}
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_gpio.attr);
	if (ret) {
		pr_tp(KERN_ERR "[elan]%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_vendor.attr);
	if (ret) {
		pr_tp(KERN_ERR "[elan]%s: sysfs_create_group failed\n", __func__);
		return ret;
	}
	return 0 ;
}

static void elan_touch_sysfs_deinit(void)
{
	sysfs_remove_file(android_touch_kobj, &dev_attr_vendor.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_gpio.attr);
	kobject_del(android_touch_kobj);
}	
#endif


static int __elan_ktf2k_ts_poll(struct i2c_client *client)
{
	struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
	int status = 0, retry = 10;

	do {
		//status = gpio_get_value(ts->intr_gpio);
		status = mt_get_gpio_in(GPIO_CTP_EINT_PIN);
		pr_tp("[elan]: %s: status = %d\n", __func__, status);
		retry--;
		msleep(50);
	} while (status == 1 && retry > 0);

	pr_tp( "[elan]%s: poll interrupt status %s\n",
			__func__, status == 1 ? "high" : "low");
	return (status == 0 ? 0 : -ETIMEDOUT);
}

static int elan_ktf2k_ts_poll(struct i2c_client *client)
{
	return __elan_ktf2k_ts_poll(client);
}

static int elan_ktf2k_ts_get_data(struct i2c_client *client, uint8_t *cmd,
			uint8_t *buf, size_t size)
{
	int rc;

	dev_dbg(&client->dev, "[elan]%s: enter\n", __func__);

	if (buf == NULL)
	{
		
		return -EINVAL;
	}


	if ((elan_i2c_send_data(client, cmd, 4)) != 4) {
		dev_err(&client->dev,
			"[elan]%s: elan_i2c_send_data failed\n", __func__);
		
		return -EINVAL;
	}


	rc = elan_ktf2k_ts_poll(client);
	
	if (rc < 0)
		return -EINVAL;
	else {

		if (elan_i2c_recv_data(client, buf, size) != size ||
		    buf[0] != CMD_S_PKT){

			return -EINVAL;
		}
	}

	return 0;
}

static int __hello_packet_handler(struct i2c_client *client)
{
	int rc;
	uint8_t buf_recv[8] = { 0 };
	uint8_t cmd[] = {0x53, 0x00, 0x00, 0x01};

	rc = elan_ktf2k_ts_poll(client);
	if(rc != 0){
		printk("[elan] %s: Int poll 55 55 55 55 failed!\n", __func__);
	}

	rc = elan_i2c_recv_data(client, buf_recv, sizeof(buf_recv));
	if(rc != sizeof(buf_recv)){
		printk("[elan error] __hello_packet_handler recv error, retry\n");
		rc = elan_i2c_recv_data(client, buf_recv, sizeof(buf_recv));
	}
	printk("[elan] %s: hello packet %2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);

	if(buf_recv[0]==0x55 && buf_recv[1]==0x55 && buf_recv[2]==0x80 && buf_recv[3]==0x80){
		printk("[elan] %s: boot code packet %2x:%2X:%2x:%2x\n", __func__, buf_recv[4], buf_recv[5], buf_recv[6], buf_recv[7]);
		RECOVERY = 0x80;
	}
	else if(buf_recv[0]==0x55 && buf_recv[1]==0x55 && buf_recv[2]==0x55 && buf_recv[3]==0x55){
		printk("[elan] __hello_packet_handler recv ok\n");
		msleep(200);
		rc = elan_ktf2k_ts_poll(client);
		if (rc < 0) {
			printk("[elan] %s: 66 66 66 66 failed!\n", __func__);
		}
		rc = elan_i2c_recv_data(client, buf_recv, sizeof(buf_recv));
		printk("[elan] %s: RK packet %2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);
		RECOVERY = 0;
	}
	else{
		if(rc != sizeof(buf_recv)){
			rc = elan_i2c_send_data(client, cmd, sizeof(cmd));
			if(rc != sizeof(cmd)){
				msleep(5);
				rc = elan_i2c_recv_data(client, buf_recv, sizeof(buf_recv));
				if(rc != sizeof(buf_recv)){
					return -EINVAL;
				}
			}
		}
	}

	return RECOVERY;
}

static int __fw_packet_handler(struct i2c_client *client)
{
	int rc;
	int major, minor;
	uint8_t cmd[] = {CMD_R_PKT, 0x00, 0x00, 0x01};/* Get Firmware Version*/
	uint8_t cmd_x[] = {0x53, 0x60, 0x00, 0x00}; /*Get x resolution*/
	uint8_t cmd_y[] = {0x53, 0x63, 0x00, 0x00}; /*Get y resolution*/
	uint8_t cmd_id[] = {0x53, 0xf0, 0x00, 0x01}; /*Get firmware ID*/
	uint8_t cmd_bc[] = {CMD_R_PKT, 0x01, 0x00, 0x01};/* Get BootCode Version*/
	uint8_t buf_recv[4] = {0};

pr_tp( "[elan] %s: n", __func__);
// Firmware version
	rc = elan_ktf2k_ts_get_data(client, cmd, buf_recv, 4);
	if (rc < 0)
		return rc;
	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
//	ts->fw_ver = major << 8 | minor;
	FW_VERSION = major << 8 | minor;
// Firmware ID
	rc = elan_ktf2k_ts_get_data(client, cmd_id, buf_recv, 4);
	if (rc < 0)
		return rc;
	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	//ts->fw_id = major << 8 | minor;
	FW_ID = major << 8 | minor;
// X Resolution
	rc = elan_ktf2k_ts_get_data(client, cmd_x, buf_recv, 4);
	if (rc < 0)
		return rc;
	minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
	//ts->x_resolution =minor;
	//X_RESOLUTION = minor;
	X_RESOLUTION = minor;
// Y Resolution	
	rc = elan_ktf2k_ts_get_data(client, cmd_y, buf_recv, 4);
	if (rc < 0)
		return rc;
	minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
	//ts->y_resolution =minor;
	//Y_RESOLUTION = minor;
    Y_RESOLUTION = minor;
// Bootcode version
	rc = elan_ktf2k_ts_get_data(client, cmd_bc, buf_recv, 4);
	if (rc < 0)
		return rc;
	major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
	minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
	//ts->bc_ver = major << 8 | minor;
	BC_VERSION = major << 8 | minor;
	
	pr_tp( "[elan] %s: firmware version: 0x%4.4x\n",
			__func__, FW_VERSION);
	pr_tp( "[elan] %s: firmware ID: 0x%4.4x\n",
			__func__, FW_ID);
	pr_tp( "[elan] %s: x resolution: %d, y resolution: %d\n",
			__func__, X_RESOLUTION, Y_RESOLUTION);
	pr_tp( "[elan] %s: bootcode version: 0x%4.4x\n",
			__func__, BC_VERSION);
	return 0;
}

static inline int elan_ktf2k_ts_parse_xy(uint8_t *data,
			uint16_t *x, uint16_t *y)
{
	*x = *y = 0;

	*x = (data[0] & 0xf0);
	*x <<= 4;
	*x |= data[1];

	*y = (data[0] & 0x0f);
	*y <<= 8;
	*y |= data[2];

	return 0;
}

static int elan_ktf2k_ts_setup(struct i2c_client *client)
{
	int rc;
   
	rc = __hello_packet_handler(client);
	if (rc < 0){
		printk("[elan error] %s, hello_packet_handler fail, rc = %d\n", __func__, rc);
		return rc;
	}
	
	// for firmware update
	if(rc == 0x80){
		printk("[elan error] %s, fw had bening miss, rc = %d\n", __func__, rc);
		return rc;
	}
	
	__fw_packet_handler(client);
	
	return rc; /* Firmware need to be update if rc equal to 0x80(Recovery mode)   */
}

static int elan_ktf2k_ts_rough_calibrate(struct i2c_client *client){
      uint8_t cmd[] = {CMD_W_PKT, 0x29, 0x00, 0x01};

	//dev_info(&client->dev, "[elan] %s: enter\n", __func__);
	pr_tp("[elan] %s: enter\n", __func__);
	dev_info(&client->dev,
		"[elan] dump cmd: %02x, %02x, %02x, %02x\n",
		cmd[0], cmd[1], cmd[2], cmd[3]);

	if ((elan_i2c_send_data(client, cmd, sizeof(cmd))) != sizeof(cmd)) {
		dev_err(&client->dev,
			"[elan] %s: elan_i2c_send_data failed\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int elan_ktf2k_ts_set_power_state(struct i2c_client *client, int state)
{
	uint8_t cmd[] = {CMD_W_PKT, 0x50, 0x00, 0x01};

	dev_dbg(&client->dev, "[elan] %s: enter\n", __func__);

	cmd[1] |= (state << 3);

	dev_dbg(&client->dev,
		"[elan] dump cmd: %02x, %02x, %02x, %02x\n",
		cmd[0], cmd[1], cmd[2], cmd[3]);

	if ((elan_i2c_send_data(client, cmd, sizeof(cmd))) != sizeof(cmd)) {
		dev_err(&client->dev,
			"[elan] %s: elan_i2c_send_data failed\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int elan_ktf2k_ts_get_power_state(struct i2c_client *client)
{
	int rc = 0;
	uint8_t cmd[] = {CMD_R_PKT, 0x50, 0x00, 0x01};
	uint8_t buf[4], power_state;

	rc = elan_ktf2k_ts_get_data(client, cmd, buf, 4);
	if (rc)
		return rc;

	power_state = buf[1];
	dev_dbg(&client->dev, "[elan] dump repsponse: %0x\n", power_state);
	power_state = (power_state & PWR_STATE_MASK) >> 3;
	dev_dbg(&client->dev, "[elan] power state = %s\n",power_state == PWR_STATE_DEEP_SLEEP ? "Deep Sleep" : "Normal/Idle");

	return power_state;
}

static void elan_ts_handler_event(uint8_t *buf)
{
#ifdef TP_DBLCLIK_RESUM
	if(buf[0] == 0x88 && buf[1] == 0x88 && buf[2] == 0x88 && buf[3] == 0x88){
		if (atomic_read(&resume_flag) == 0) {
			printk("[elan] double click to resum tp now\n");
			atomic_set(&resume_flag, 1);
			input_report_key(tpd->dev, KEY_POWER, 1);
			input_report_key(tpd->dev, KEY_POWER, 0);
			input_sync(tpd->dev);
		}
	}
#endif
}

static int elan_ktf2k_ts_recv_data(struct i2c_client *client, uint8_t *buf)
{
	int rc, bytes_to_recv=PACKET_SIZE;
	int i = 0;
	
	rc = elan_i2c_recv_data(client, buf, bytes_to_recv);

    #ifdef ELAN_DEBUG
    	for(i = 0; i < (PACKET_SIZE+7)/8; i++){
			printk("%02x %02x %02x %02x %02x %02x %02x %02x\n", buf[i*8+0],buf[i*8+1],buf[i*8+2],buf[i*8+3],buf[i*8+4],buf[i*8+5],buf[i*8+6],buf[i*8+7]);
		}
    #endif
    
#ifdef ELAN_ESD_CHECK	
	if(buf[0] == 0x78 || buf[0] == TWO_FINGERS_PKT || buf[0] == FIVE_FINGERS_PKT || buf[0] == TEN_FINGERS_PKT){
		atomic_set(&elan_cmd_response, 1);
	}
#endif	
	
	return rc;
}

static void elan_ts_touch_down(s32 id,s32 x,s32 y,s32 w)
{
    if (RECOVERY_BOOT != get_boot_mode())
    {
        input_report_key(tpd->dev, BTN_TOUCH, 1);
    }
	input_report_abs(tpd->dev, ABS_MT_PRESSURE, w);
	//input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, w);
	//input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR, w);
	//input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, id);
	input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
	input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
	input_mt_sync(tpd->dev);

    #ifdef ELAN_DEBUG
	    printk("Touch ID:%d, X:%d, Y:%d, W:%d down\n", id, x, y, w); 
    #endif
    if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
    {   
        tpd_button(x, y, 1);
    }
    if(y > TPD_RES_Y) //virtual key debounce to avoid android ANR issue
    {
        msleep(50);
        pr_tp(TPD_DEVICE "D virtual key \n");
    }
}

static void elan_ts_touch_up(s32 id,s32 x,s32 y)
{
	input_report_key(tpd->dev, BTN_TOUCH, 0);
	//input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0);
	//input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0);
	//input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR, 0);
	input_mt_sync(tpd->dev);
	
	#ifdef ELAN_DEBUG
	    printk("Touch all release!\n");
    #endif
    if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
    {   
        tpd_button(x, y, 0); 
    }
}

static void elan_ts_report_key(uint8_t button_data)
{
	static unsigned int x = 0,y = 0;
	int key_down_flag = 0;

	switch (button_data) {
		case ELAN_KEY_MENU:
		    key_down_flag = 1;
			x = tpd_keys_dim_local[0][0];
			y = tpd_keys_dim_local[0][1];
			//if(boot_normal_flag == 1){
			elan_ts_touch_down(0, x, y, 8);
            printk("key ELAN_KEY_MENU down\n");
			//}
			//else{
			//	tpd_button(x, y, 1);
			//}
			break;
		case ELAN_KEY_HOME:
		    key_down_flag = 1;
			x = tpd_keys_dim_local[1][0];
			y = tpd_keys_dim_local[1][1];
			//if(boot_normal_flag == 1){
			elan_ts_touch_down(0, x, y, 8);
            printk("key ELAN_KEY_HOME down\n");
			//}
			//else{
			//	tpd_button(x, y, 1);
			//}
			break;
		case ELAN_KEY_BACK:
		    key_down_flag = 1;		
			x = tpd_keys_dim_local[2][0];
			y = tpd_keys_dim_local[2][1];
			//if(boot_normal_flag == 1){
			elan_ts_touch_down(0, x, y, 8);
            printk("key ELAN_KEY_BACK down\n");
			//}
			//else{
			//	tpd_button(x, y, 1);
			//}
			break;
		default:
			//if(boot_normal_flag != 1 && key_down_flag == 1){
			//    tpd_button(x, y, 0);
			//    key_down_flag = 0;
			//}
			//else{
			elan_ts_touch_up(0, x, y);
			//}
			break;
	}
}

static void elan_ktf2k_ts_report_data(uint8_t *buf)
{
    uint16_t fbits=0;
    int reported = 0;
    uint8_t idx;
    int finger_num;
    int num = 0;
    uint16_t x = 0;
    uint16_t y = 0;
    int position = 0;
    uint8_t button_byte = 0;
    
    /* for 10 fingers	*/
    if (buf[0] == TEN_FINGERS_PKT){
        finger_num = 10;
        fbits = buf[2] & 0x30;	
        fbits = (fbits << 4) | buf[1];  
        num = buf[2] &0x0f;
        idx=3;
        button_byte = buf[33];
    }
    // for 5 fingers	
    else if ((buf[0] == MTK_FINGERS_PKT) || (buf[0] == FIVE_FINGERS_PKT)){
        finger_num = 5;
        num = buf[1] & 0x07; 
        fbits = buf[1] >>3;
        idx=2;
        button_byte = buf[17];
    }
    // for 2 fingers      
    else if (buf[0] == TWO_FINGERS_PKT){
        finger_num = 2;
        num = buf[7] & 0x03; 
        fbits = buf[7] & 0x03;
        idx=1;
        button_byte = buf[7];
    }
    //other event
    else{
#ifndef ELAN_DEBUG		
        printk("[elan] other event packet:%02x %02x %02x %02x %02x %02x %02x %02x\n", buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
#endif
        elan_ts_handler_event(buf);
        return;
    }
    
    if (num == 0){
        elan_ts_report_key(button_byte);
    } 
    else{
    //printk( "[elan] %d fingers", num);
        for(position=0; (position<finger_num) && (reported < num);position++){
            if((fbits & 0x01)){
                elan_ktf2k_ts_parse_xy(&buf[idx], &x, &y);
#if 1
                if((X_RESOLUTION > 0) &&( Y_RESOLUTION > 0))
                {
                    x = ( x * LCM_X_MAX )/X_RESOLUTION;
                    y = ( y * LCM_Y_MAX )/Y_RESOLUTION;
                }
                else
                {
                    x = ( x * LCM_X_MAX )/ELAN_X_MAX;
                    y = ( y * LCM_Y_MAX )/ELAN_Y_MAX;
                }
#endif 		 
                elan_ts_touch_down(position, x, y, 8);
                reported++;
            }
            fbits = fbits >> 1;
            idx += 3;
        }
    }

    input_sync(tpd->dev);
    return;
}

static int touch_event_handler(void *unused)
{
	int rc;
	uint8_t buf[64] = { 0 };

	int touch_state = 3;
	
	unsigned long time_eclapse;
	struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
	sched_setscheduler(current, SCHED_RR, &param);
	int last_key = 0;
	int key;
	int index = 0;
	int i =0;

	do
	{
		mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
		//enable_irq(CUST_EINT_TOUCH_PANEL_NUM);
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter, tpd_flag != 0);
		tpd_flag = 0;
		set_current_state(TASK_RUNNING);
		//disable_irq(CUST_EINT_TOUCH_PANEL_NUM);
		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
		rc = elan_ktf2k_ts_recv_data(private_ts->client, buf);

		if (rc < 0)
		{
			pr_tp("[elan] rc<0\n");
	
			continue;
		}
		
		elan_ktf2k_ts_report_data(buf);

	}while(!kthread_should_stop());

	return 0;
}

static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
    strcpy(info->type, TPD_DEVICE);
    return 0;
}

static void tpd_eint_interrupt_handler(void)
{
//    pr_tp("[elan]TPD int\n");
    tpd_flag = 1;
    wake_up_interruptible(&waiter);
}

#ifdef ELAN_ESD_CHECK
bool CTP_EsdRecover(void)
{
    printk(TPD_DEVICE "CTP_EsdRecover \n");
    
    mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
    
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, 0);
    msleep(10);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, 1);
    
    msleep(200);
    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    return true;
}

static void elan_touch_esd_func(struct work_struct *work)
{	
	int res;	
	uint8_t cmd[] = {0x53, 0x00, 0x00, 0x01};	
	struct i2c_client *client = private_ts->client;	

	pr_tp("esd %s: enter.......", __FUNCTION__);
	
	if(work_lock == 1){
		goto elan_esd_check_out;
	}
	
	if(atomic_read(&elan_cmd_response) == 0){
		printk("elan response failed, power off tp!!!\n");
	}
	else{
		pr_tp("esd %s: response ok!!!", __func__);
		goto elan_esd_check_out;
	}
	//power off replace reset
	mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
	hwPowerDown(MT6323_POWER_LDO_VGP1, "TP");
	mdelay(50);
	
	hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_2800, "TP");
	mdelay(50);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	
elan_esd_check_out:	
	
	atomic_set(&elan_cmd_response, 0);
	queue_delayed_work(esd_wq, &esd_work, m_delay);
	pr_tp("[elan esd] %s: out.......", __FUNCTION__);	
	return;
}
#endif

static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;
	int fw_err = 0;
	int New_FW_ID;	
	int New_FW_VER;	
	int i;
	int retval = TPD_OK;
	int ret;
	char buf[PACKET_SIZE] = {0};
	static struct elan_ktf2k_ts_data ts;

	client->addr |= I2C_ENEXT_FLAG;

	
	pr_tp("[elan] %s:client addr is %x, TPD_DEVICE = %s\n",__func__,client->addr,TPD_DEVICE);
	pr_tp("[elan] %s:I2C_WR_FLAG=%x,I2C_MASK_FLAG=%x,I2C_ENEXT_FLAG =%x\n",__func__,I2C_WR_FLAG,I2C_MASK_FLAG,I2C_ENEXT_FLAG);
	client->timing =  400;

	pr_tp("[elan]%x=IOCTL_I2C_INT\n",IOCTL_I2C_INT);
	pr_tp("[elan]%x=IOCTL_IAP_MODE_LOCK\n",IOCTL_IAP_MODE_LOCK);
	pr_tp("[elan]%x=IOCTL_IAP_MODE_UNLOCK\n",IOCTL_IAP_MODE_UNLOCK);

#if 1
	//client->timing = 400;
	i2c_client = client;
	private_ts = &ts;
	private_ts->client = client;
	//private_ts->addr = 0x2a;
#endif

	hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_2800, "TP");
//	hwPowerOn(MT65XX_POWER_LDO_VGP6, VOL_1800, "touch"); //add by robin
	msleep(10);
#if 0
	/*LDO enable*/
	mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ZERO);
	msleep(50);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
#endif
	pr_tp("[elan] ELAN enter tpd_probe_ ,the i2c addr=0x%x", client->addr);
	pr_tp("GPIO43 =%d,GPIO_CTP_EINT_PIN =%d,GPIO_DIR_IN=%d,CUST_EINT_TOUCH_PANEL_NUM=%d",GPIO43,GPIO_CTP_EINT_PIN,GPIO_DIR_IN,CUST_EINT_TOUCH_PANEL_NUM);

// Reset Touch Pannel
    mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
    mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
    mdelay(10);
//#if !defined(EVB)
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
//#endif
    mdelay(10);
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
// End Reset Touch Pannel	

    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
    mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);
    msleep( 200 );

#ifdef MTK6589_DMA    
    gpDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &gpDMABuf_pa, GFP_KERNEL);
    if(!gpDMABuf_va){
		pr_tp(KERN_INFO "[elan] Allocate DMA I2C Buffer failed\n");
    }
#endif

	fw_err = elan_ktf2k_ts_setup(client);
	if (fw_err < 0) {
		printk("[elan] No Elan chip inside\n");
		hwPowerDown(MT6323_POWER_LDO_VGP1, "TP");
		return -1;
	}
	
#ifdef TP_DBLCLIK_RESUM
	input_set_capability(tpd->dev, EV_KEY, KEY_POWER);
#endif

#ifdef HAVE_TOUCH_KEY
int retry;
	for(retry = 0; retry <3; retry++)
	{
		input_set_capability(tpd->dev,EV_KEY,tpd_keys_local[retry]);
	}
#endif

// Setup Interrupt Pin
	mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);


	//mt_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE);
	mt_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, EINTF_TRIGGER_FALLING, tpd_eint_interrupt_handler, 1);
	//mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_POLARITY_LOW, tpd_eint_interrupt_handler, 1);
	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	msleep(100);
// End Setup Interrupt Pin	


	tpd_load_status = 1;
	if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode()){
		boot_normal_flag = 0;
	}


#if 0 /*RESET RESOLUTION: tmp use ELAN_X_MAX & ELAN_Y_MAX*/ 
	pr_tp("[elan] RESET RESOLUTION\n");
	input_set_abs_params(tpd->dev, ABS_X, 0,  ELAN_X_MAX, 0, 0);
	input_set_abs_params(tpd->dev, ABS_Y, 0,  ELAN_Y_MAX, 0, 0);
	input_set_abs_params(tpd->dev, ABS_MT_POSITION_X, 0, ELAN_X_MAX, 0, 0);
	input_set_abs_params(tpd->dev, ABS_MT_POSITION_Y, 0, ELAN_Y_MAX, 0, 0);
#endif 

	thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);

	if(IS_ERR(thread))
	{
		retval = PTR_ERR(thread);
		pr_tp(TPD_DEVICE "[elan]  failed to create kernel thread: %d\n", retval);
		return -1;
	}

	pr_tp("[elan]  ELAN Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");
// Firmware Update
	// MISC
  	ts.firmware.minor = MISC_DYNAMIC_MINOR;
  	ts.firmware.name = "elan-iap";
  	ts.firmware.fops = &elan_touch_fops;
  	ts.firmware.mode = S_IRWXUGO; 
   	
  	if (misc_register(&ts.firmware) < 0)
  		pr_tp("[elan] misc_register failed!!");
  	else
  	  pr_tp("[elan] misc_register finished!!"); 
// End Firmware Update	


#if IAP_PORTION
	if(1)
	{
		work_lock=1;
		disable_irq(CUST_EINT_TOUCH_PANEL_NUM);
		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
		power_lock = 1;
		pr_tp("[elan] start fw update");
		
/* FW ID & FW VER*/
#if 0  /* For ektf21xx and ektf20xx  */
    pr_tp("[ELAN]  [7bd0]=0x%02x,  [7bd1]=0x%02x, [7bd2]=0x%02x, [7bd3]=0x%02x\n",  file_fw_data[31696],file_fw_data[31697],file_fw_data[31698],file_fw_data[31699]);
		New_FW_ID = file_fw_data[31699]<<8  | file_fw_data[31698] ;	       
		New_FW_VER = file_fw_data[31697]<<8  | file_fw_data[31696] ;
#endif
/*		
#if 0   // for ektf31xx 2 wire ice ex: 2wireice -b xx.bin 
    pr_tp(" [7c16]=0x%02x,  [7c17]=0x%02x, [7c18]=0x%02x, [7c19]=0x%02x\n",  file_fw_data[31766],file_fw_data[31767],file_fw_data[31768],file_fw_data[31769]);
		New_FW_ID = file_fw_data[31769]<<8  | file_fw_data[31768] ;	       
		New_FW_VER = file_fw_data[31767]<<8  | file_fw_data[31766] ;
#endif	
    //for ektf31xx iap ekt file   	
    pr_tp(" [7bd8]=0x%02x,  [7bd9]=0x%02x, [7bda]=0x%02x, [7bdb]=0x%02x\n",  file_fw_data[31704],file_fw_data[31705],file_fw_data[31706],file_fw_data[31707]);
		New_FW_ID = file_fw_data[31707]<<8  | file_fw_data[31708] ;	       
		New_FW_VER = file_fw_data[31705]<<8  | file_fw_data[31704] ;
	  pr_tp(" FW_ID=0x%x,   New_FW_ID=0x%x \n",  FW_ID, New_FW_ID);   	       
		pr_tp(" FW_VERSION=0x%x,   New_FW_VER=0x%x \n",  FW_VERSION  , New_FW_VER); 
*/
		
//for firmware auto-upgrade            
	  //if (New_FW_ID   ==  FW_ID)
	  	//{		      
		   	if (New_FW_VER > (FW_VERSION)) 
		    Update_FW_One(client, RECOVERY);
		//}
	  else {                        
		    pr_tp("FW_ID is different!");		
		   } 
	  
	  if (FW_ID   == 0)
	  	{		      
		   	RECOVERY=0x80;
		    Update_FW_One(client, RECOVERY);
		}

		work_lock=0;
		enable_irq(CUST_EINT_TOUCH_PANEL_NUM);
		mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);

	}
#endif

#ifdef ELAN_ESD_CHECK
	INIT_DELAYED_WORK(&esd_work, elan_touch_esd_func);
	esd_wq = create_singlethread_workqueue("esd_wq");	
	if (!esd_wq) {
		return -ENOMEM;
	}
	queue_delayed_work(esd_wq, &esd_work, m_delay);
#endif
    
    return 0;

}

static int tpd_remove(struct i2c_client *client)

{
    pr_tp("[elan] TPD removed\n");
    
    #ifdef MTK6589_DMA    
    if(gpDMABuf_va){
        dma_free_coherent(NULL, 4096, gpDMABuf_va, gpDMABuf_pa);
        gpDMABuf_va = NULL;
        gpDMABuf_pa = NULL;
    }
    #endif    
    
    return 0;
}


static int tpd_suspend(struct i2c_client *client, pm_message_t message)
{
    int retval = TPD_OK;
    uint8_t cmd[] = {CMD_W_PKT, 0x50, 0x00, 0x01};
#ifdef ELAN_ESD_CHECK	
	cancel_delayed_work_sync(&esd_work);
#endif
    
    printk("[elan] TP enter into sleep mode\n");
    
#ifdef TP_DBLCLIK_RESUM	
   	retval = elan_i2c_send_data(private_ts->client, cmd, sizeof(cmd));
	atomic_set(&resume_flag, 0);
	printk("[elan] %s: elan_i2c_send_data rc = %d\n", __func__, retval);
#else
	mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
	hwPowerDown(MT6323_POWER_LDO_VGP1, "TP");
#endif

    return retval;
}


static int tpd_resume(struct i2c_client *client)
{
    int retval = TPD_OK;
    uint8_t cmd[] = {CMD_W_PKT, 0x58, 0x00, 0x01};
    printk("[elan] %s wake up\n", __func__);
	
#ifndef TP_DBLCLIK_RESUM
	hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_2800, "TP");
	mdelay(10);
#endif
	
#if 1	
	// Reset Touch Pannel
    mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
    mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
    mdelay(10);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    mdelay(10);
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
	mdelay(50);

#else 
    if ((elan_i2c_send_data(private_ts->client, cmd, sizeof(cmd))) != sizeof(cmd)) 
    {
		pr_tp("[elan] %s: elan_i2c_send_data failed\n", __func__);
		return -retval;
    }

    msleep(200);
#endif

    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#ifdef ELAN_ESD_CHECK
	queue_delayed_work(esd_wq, &esd_work, m_delay);	
#endif    
    return retval;
}

static int tpd_local_init(void)
{
    pr_tp("[elan]: I2C Touchscreen Driver init\n");
    if(i2c_add_driver(&tpd_i2c_driver) != 0)
    {
        pr_tp("[elan]: unable to add i2c driver.\n");
        return -1;
    }

#ifdef TPD_HAVE_BUTTON
  #ifdef LCT_VIRTUAL_KEY
    tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
  #endif
#endif
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif

    pr_tp("end %s, %d\n", __FUNCTION__, __LINE__);
    tpd_type_cap = 1;
    return 0;
}


static ssize_t show_chipinfo(struct device *dev,struct device_attribute *attr, char *buf)
{
	struct i2c_client *client =i2c_client;
	if(NULL == client)
	{
		printk("i2c client is null!!\n");
		return 0;
	}
	
	switch (FW_ID)
	{
		case 0x14F5:
			return sprintf(buf,"ID:0x%x VER:0x%x IC:ektf3248 VENDOR:truly\n",FW_ID, FW_VERSION);	
			break;
		default:
			return sprintf(buf,"ID:0x%x VER:0x%x IC:ektf3248 VENDOR:ckt\n",FW_ID, FW_VERSION);	
			break;
	}
	//return sprintf(buf, "%s\n", strbuf);        
}

static DEVICE_ATTR(chipinfo, 0444, show_chipinfo, NULL);


static const struct device_attribute * const ctp_attributes[] = {
	&dev_attr_chipinfo
};

static struct tpd_driver_t tpd_device_driver =
{
    .tpd_device_name = "ektf3k_mtk",       
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
	.attrs=
	 {
		.attr=ctp_attributes,
		.num=1
	 },
#ifdef TPD_HAVE_BUTTON
    .tpd_have_button = 1,
#else
    .tpd_have_button = 0,
#endif
};


static int __init tpd_driver_init(void)//I2C init
{
    pr_tp("[elan]: Driver Verison MTK0005 for MTK65xx serial\n");
#ifdef ELAN_MTK6577		
		pr_tp("[elan] Enable ELAN_MTK6577\n");
		i2c_register_board_info(0, &i2c_tpd, 1);
#endif		
    if(tpd_driver_add(&tpd_device_driver) < 0)
    {
        pr_tp("[elan]: %s driver failed\n", __func__);
    }

    return 0;
}


static void __exit tpd_driver_exit(void)
{
    pr_tp("[elan]: %s elan touch panel driver exit\n", __func__);
    tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);




