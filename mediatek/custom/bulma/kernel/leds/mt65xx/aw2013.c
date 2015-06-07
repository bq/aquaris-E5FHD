#include "aw2013.h"

static uint32_t on_time=0,on_level=0,blink=9;
static struct led_setting led0,led1,led2;
static DEFINE_MUTEX(aw_mutex);

struct i2c_client *aw2013_i2c_client;

static unsigned char LED_ON_FLAG = 0x0;

static HW_DEVICE g_hwctl_device={.init_stage=DEV_NONE_STAGE,};

static struct file_operations hwctl_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= hwctl_unlock_ioctl,
	.open		= hwctl_open,
	.release	= hwctl_release,
	.read		= hwctl_read,
};

static const struct i2c_device_id aw2013_i2c_id[] = {
	{ "AW2013", 0 },
	{ }
};


static struct i2c_board_info __initdata aw2013_i2c_hw={ I2C_BOARD_INFO("AW2013", (AW2013_I2C_ADDRESS_WRITE>>1))};

static struct i2c_driver aw2013_i2c_driver = {
        .driver = 
	   {
                .name   = "AW2013",
        },

        .probe          = AW2013_i2c_probe,
        .remove         =  __devexit_p(AW2013_i2c_remove),
        .id_table       = aw2013_i2c_id,
};


//*******************************AW2013ºôÎüµÆ³ÌĞò***********************************///
/*
void led_flash_aw2013( unsigned int id )   //id = 0/1/2£¬·Ö±ğ¶ÔÓ\u0160LED0 LED1 LED2
{
	unsigned char reg_data;
	unsigned int	reg_buffer[8];
	int j=0;
	//while(1)
		{
	//write_reg(0x00, 0x55);				// Reset 
	hwctl_i2c_write_reg(0x01, 0x01);		// enable LED ²»Ê¹ÓÃÖĞ¶Ï			

	hwctl_i2c_write_reg(0x31+id, 0x70|Imax);	//config mode, IMAX 	
	hwctl_i2c_write_reg(0x34+id, 0xff);	// LED0 level,
											
	hwctl_i2c_write_reg(0x37+id*3, Rise_time<<4 |Hold_time);	//led0  t_rise=0.52s  && t_hold=1.04s								
	hwctl_i2c_write_reg(0x38+id*3, Fall_time<<4 |Off_time);	//led0  t_fall=0.52s  && t_off=4.16s
	hwctl_i2c_write_reg(0x39+id*3, Delay_time<<4 |Period_Num);	//led0  t_Delay=0s && cnt=ÎŞÇî\u017dó
					
	//read_reg(0x30,1,reg_buffer);
	//reg_data = reg_buffer[0] | (0x1 << id);
	hwctl_i2c_write_reg(0x30, 1<<id);	//led on	
		for (j=0; j < 50000; j++);//ĞèÑÓÊ±5usÒÔÉÏ

		}
}
*/
void hwctl_led_off()//( unsigned int id )
{
       DBG_PRINT("==== [aw2013]hwctl_led_off ====\n");
	unsigned char reg_data;
	unsigned int	reg_buffer[8];

	hwctl_i2c_write_reg(0x30, 0);				//led off	
	hwctl_i2c_write_reg(0x01,0);

	}

void hwctl_delay_1us(U16 wTime)   //
{
	udelay(wTime);
}

// Bee-20140417
static BOOL hwctl_i2c_write_reg_org(unsigned char reg,unsigned char data)
{
	BOOL ack=0;
	unsigned char ret;
	unsigned char wrbuf[2];

	wrbuf[0] = reg;
	wrbuf[1] = data;

	ret = i2c_master_send(aw2013_i2c_client, wrbuf, 2);
	if (ret != 2) {
		dev_err(&aw2013_i2c_client->dev,
		"%s: i2c_master_recv() failed, ret=%d\n",
		__func__, ret);
		ack = 1;
	}

	return ack;
}

BOOL hwctl_i2c_write_reg(unsigned char reg,unsigned char data)
{
	BOOL ack=0;
	unsigned char i;
	for (i=0; i<AW2013_I2C_MAX_LOOP; i++)
	{
		ack = hwctl_i2c_write_reg_org(reg,data);
		if (ack == 0) // ack success
			break;
		}
	return ack;
}

unsigned char hwctl_i2c_read_reg(unsigned char regaddr) 
{
	unsigned char rdbuf[1], wrbuf[1], ret, i;

	wrbuf[0] = regaddr;

	for (i=0; i<AW2013_I2C_MAX_LOOP; i++) 
	{
		ret = i2c_master_send(aw2013_i2c_client, wrbuf, 1);
		if (ret == 1)
			break;
	}
	
	ret = i2c_master_recv(aw2013_i2c_client, rdbuf, 1);
	
	if (ret != 1)
	{
		DBG_PRINT("**********************   5555   ning hwctl_i2c_read_reg failed  %s \r\n", __func__);
		dev_err(&aw2013_i2c_client->dev,"%s: i2c_master_recv() failed, ret=%d\n",
			__func__, ret);
	}
	
    	return rdbuf[0];
		
}


	
//#endif //aw2013

#if 0//for s5 only 20130220
extern void tpd_toggle_hand_using(int left_right_hand);
#endif

#if 1


int breathlight_master_send(u16 addr, char * buf ,int count)
{
	unsigned char ret;
	
	ret = i2c_master_send(aw2013_i2c_client, buf, count);
	
	if (ret != count) 
	{

		DBG_PRINT("**********************   8888   ning breathlight_master_send failed  %s \r\n", __func__);
		dev_err(&aw2013_i2c_client->dev,"%s: i2c_master_recv() failed, ret=%d\n",
			__func__, ret);
	}
	return ret;
}

/////////////////////////////for aw2013
void led_flash_aw2013_test( unsigned int id )
{
	char buf[2];
	
	hwctl_i2c_read_reg(0x55);
	
	DBG_PRINT("hwctl led_flash_aw2013_test \n");	
	buf[0]=0x01;
	buf[1]=0x01;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x31;
	buf[1]=0x71;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x34;
	buf[1]=0xff;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x37;
	buf[1]=0x53;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x38;
	buf[1]=0x55;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x39;
	buf[1]=0x00;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x30;
	buf[1]=0x01;
	breathlight_master_send(0x45,buf,2);	
}

void led_off_aw2013_test(void)
{
	char buf[2];
	buf[0]=0x30;
	buf[1]=0x00;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x01;
	buf[1]=0x00;
	breathlight_master_send(0x45,buf,2);
}

void led_flash_aw2013( unsigned int id )
{
	char buf[2];

	
	buf[0]=0x01;
	buf[1]=0x01;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x31+id;
	buf[1]=0x02;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x34+id;
	buf[1]=0xff;
	breathlight_master_send(0x45,buf,2);
/** /
	buf[0]=0x37+id*3;
	buf[1]=0x53;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x38+id*3;
	buf[1]=0x55;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x39+id*3;
	buf[1]=0x00;
	breathlight_master_send(0x45,buf,2);
/**/
	buf[0]=0x30;
	buf[1]=1<<id;
	breathlight_master_send(0x45,buf,2);
}

void led_flash_aw2013_power_low(void)/////red led
{
    DBG_PRINT("==== [aw2013]led_flash_aw2013_power_low ====\n");    
	unsigned int id =0;/////red led
	char buf[2];

	buf[0]=0x00;
	buf[1]=0x54;/////reset led module
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x01;
	buf[1]=0x01;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x31+id;
	buf[1]=0x71;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x34+id;
	buf[1]=0xff;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x37+id*3;
	buf[1]=0x00;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x38+id*3;
	buf[1]=0x06;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x39+id*3;
	buf[1]=0x00;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x30;
	buf[1]=1<<id;
	breathlight_master_send(0x45,buf,2);
}

void led_flash_aw2013_charging_full(void)
{
    DBG_PRINT("==== [aw2013]led_flash_aw2013_charging_full ====\n");
	unsigned int id = 1;/////green led
	char buf[2];

	buf[0]=0x00;
	buf[1]=0x54;/////reset led module
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x01;
	buf[1]=0x01;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x31+id;
	buf[1]=0x02;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x34+id;
	buf[1]=0xff;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x30;
	buf[1]=1<<id;
	breathlight_master_send(0x45,buf,2);
	
}

void led_flash_aw2013_charging(void)
{
    DBG_PRINT("==== [aw2013]led_flash_aw2013_charging ====\n");
	unsigned int id = 0; //red led
    char buf[2];
	char temp;

    if(blink == 0)
    {
    	buf[0]=0x00;
    	buf[1]=0x55;/////reset led module
    	breathlight_master_send(0x45,buf,2);

    	buf[0]=0x01;
    	buf[1]=0x01;
    	breathlight_master_send(0x45,buf,2);

    	buf[0]=0x31+id;
    	buf[1]=0x02;
    	breathlight_master_send(0x45,buf,2);

    	buf[0]=0x34+id;
    	buf[1]=0xff;
    	breathlight_master_send(0x45,buf,2);

    	buf[0]=0x30;
    	buf[1]=1<<id;
    	breathlight_master_send(0x45,buf,2);
    }

}

void led_flash_aw2013_unanswer_message_incall(void)/////blue led
{      
    DBG_PRINT("==== [aw2013]led_flash_aw2013_unanswer_message_incall ====\n");
    static unsigned int led1=0,led2=0,led3=0; 
    unsigned int color,red,green,blue,id;
    color = (on_level>> 24) & 0xFF;
    if(color) {
    	red = (on_level >> 16) & 0xFF;
    	green = (on_level >> 8) & 0xFF;
    	blue = on_level & 0xFF;
    }else { // color = 0 means turn the LED off
    	red = green = blue = 0;
    }
    DBG_PRINT("==== [aw2013]incall:red=%x,green=%x,blue=%x ====\n",red,green,blue);  
    if(red) {
        led1 = 1;
        id = 0;
        DBG_PRINT("==== red:led1=%d ====\n",led1);
    }
    if(green) {
        led2 = 1;
        id = 1;
        DBG_PRINT("==== green:led2=%d ====\n",led2);
    }
    if(blue) {            
        led3 = 1;
        id = 2;
        DBG_PRINT("==== blue:led3=%d ====\n",led3);
    }

//    aw2013_breath_all(led1,led2,led3);

    /*wangli_20140709* /        
	unsigned int id = 2; //blue led
	char buf[2];
	char temp;

	buf[0]=0x00;
	buf[1]=0x54;/////reset led module
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x01;//enable led module
	buf[1]=0x01;
	breathlight_master_send(0x45,buf,2);

    for(i = 0; i < 3; i++)
    {
    	buf[0]=0x31+id[i];//pwm control mode
    	buf[1]=0x73;
    	breathlight_master_send(0x45,buf,2);

    	buf[0]=0x34+id[i];//brightness
    	buf[1]=0xff;
    	breathlight_master_send(0x45,buf,2);

    	buf[0]=0x37+id[i]*3;
    	buf[1]=0x04;
    	breathlight_master_send(0x45,buf,2);

    	buf[0]=0x38+id[i]*3;
    	buf[1]=0x04;
    	breathlight_master_send(0x45,buf,2);

    	buf[0]=0x39+id[i]*3;
    	buf[1]=0x00;
    	breathlight_master_send(0x45,buf,2);

    	buf[0]=0x30;//enable 3 channel
    	buf[1]=1<<id[i];
    	breathlight_master_send(0x45,buf,2);
    }
    /**/
}

void led_flash_aw2013_power_on(void)/////
{
	char buf[2];
	unsigned int id =0;////0 blue led ,1 red,2 green,

	buf[0]=0x01;
	buf[1]=0x01;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x31+id;
	buf[1]=0x73;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x34+id;
	buf[1]=0xff;//0xc8;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x37+id*3;
	buf[1]=0x34;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x38+id*3;
	buf[1]=0x35;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x39+id*3;
	buf[1]=0x03;
	breathlight_master_send(0x45,buf,2);

	buf[0]=0x30;
	buf[1]=1<<id;
	breathlight_master_send(0x45,buf,2);
}

void Flush_led_data(void)
{
	DBG_PRINT("==== In Flush_led_data: LED_ON_FLAG=%d ====\n",LED_ON_FLAG);
	//first if it's charging situation, we skip the other actions
	if(LED_ON_FLAG == CHARGING_FLAG_BIT) 
	{
		DBG_PRINT("==== CALL led_flash_aw2013_charging() ====\n");
        Suspend_led();
		led_flash_aw2013_charging();
		return;
	}
	if(LED_ON_FLAG == TIME_FLAG_BIT) 
	{
		DBG_PRINT("==== CALL hwctl_set_blink() ====\n");
		hwctl_set_blink(on_time);
		return;
	}
	if(LED_ON_FLAG == LED_COLOR_FLAG_BIT) 
	{
		DBG_PRINT("==== CALL aw2013_all_leds_control() ====\n");             
		aw2013_all_leds_control();
		return;
	}	

    
/** /    
	if(LED_ON_FLAG == CALL_FLAG_BIT) 
	{
		DBG_PRINT("==== CALL led_flash_aw2013_unanswer_message_incall() ====\n");             
		led_flash_aw2013_unanswer_message_incall();
		return;
	}
	//if(TST_BIT(LED_ON_FLAG,CHARGING_FULL_FLAG_BIT))
	if(LED_ON_FLAG == CHARGING_FULL_FLAG_BIT) 
	{
		DBG_PRINT("==== CALL led_flash_aw2013_charging_full ====\n");
		led_flash_aw2013_charging_full();
		return;
	}
	//second the bat infor is the priority
	//if(TST_BIT(LED_ON_FLAG,BAT_FLAG_BIT))
	if(LED_ON_FLAG == BAT_FLAG_BIT) 
	{
		DBG_PRINT("==== CALL led_flash_aw2013_power_low ====\n");
		led_flash_aw2013_power_low();
		return;
	}
	//if(TST_BIT(LED_ON_FLAG,MSG_FLAG_BIT))
	if(LED_ON_FLAG == MSG_FLAG_BIT) 
	{
		DBG_PRINT("==== CALL led_flash_aw2013_unanswer_message_incall-message ====\n");
		led_flash_aw2013_unanswer_message_incall();
		return;
	}	
/**/     
}

void Suspend_led(void)
{
	//first if it's charging situation, we skip the other actions
	hwctl_led_off();
}
#endif


static long hwctl_unlock_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	DBG_PRINT("hwctl_ioctl cmd=%x",cmd);

    long err = 0;
	void __user *ptr = (void __user*) arg;
	uint32_t level,time;

    switch(cmd)
    {
        case HW_CTL_IO_FORCE_REFRESH_LEDS:	
			DBG_PRINT("==== [HWCTL]line536:HW_CTL_IO_FORCE_REFRESH_LEDS ====\n");
			Suspend_led();
			break;
		case HW_CTL_IO_SET_BLINK:
            DBG_PRINT("==== [HWCTL]line540:HW_CTL_IO_SET_BLINK ====\n");
			LED_ON_FLAG = TIME_FLAG_BIT;
			if(copy_from_user(&time, ptr, sizeof(level)))
			{
				err = -EFAULT;
				break;
			}
			mutex_lock(&aw_mutex);
			on_time = time;
			mutex_unlock(&aw_mutex);
			DBG_PRINT("==== [HWCTL]line550:on_time = %d ====\n",on_time);
			Flush_led_data();
			break;           
		case HW_CTL_IO_LED_COLOR:
			DBG_PRINT("==== [HWCTL]line554:HW_CTL_IO_LED_COLOR ====\n");
			LED_ON_FLAG = LED_COLOR_FLAG_BIT;
			if(copy_from_user(&level, ptr, sizeof(level)))
			{
				err = -EFAULT;
				break;
			}
			mutex_lock(&aw_mutex);
			on_level = level;
			mutex_unlock(&aw_mutex);
			DBG_PRINT("==== [HWCTL]line564:on_level = %x ====\n",on_level);
			Flush_led_data();
			break;

/** /
		case HW_CTL_IO_EN_CALL_NTF:
			DBG_PRINT("==== [HWCTL]line544:HW_CTL_IO_EN_CALL_NTF ====\n");
			LED_ON_FLAG = CALL_FLAG_BIT;
			if(copy_from_user(&level, ptr, sizeof(level)))
			{
				err = -EFAULT;
				break;
			}
			mutex_lock(&aw_mutex);
			on_level = level;
			mutex_unlock(&aw_mutex);
			DBG_PRINT("==== [HWCTL]line 578:on_level = %x ====\n",on_level);
			hwctl_shut_charging_leds_and_dojobs();
			break;
		case HW_CTL_IO_CHARGING_EN_NTF:	
			DBG_PRINT("==== [HWCTL]line 556:HW_CTL_IO_CHARGING_EN_NTF ====\n");         
			LED_ON_FLAG = CHARGING_FLAG_BIT;
			if(copy_from_user(&level, ptr, sizeof(level)))
			{
				err = -EFAULT;
				break;
			}
			mutex_lock(&aw_mutex);
			on_level = level;
			mutex_unlock(&aw_mutex);
			DBG_PRINT("==== [HWCTL]line 607:on_level = %x ====\n",on_level);            
			Flush_led_data();///charging : red
			break;
        case HW_CTL_IO_TEST:
			DBG_PRINT("this is test and this module has been tested\n");
			break;
		case HW_CTL_IO_ENB_KBD:			
			DBG_PRINT("[HWCTL]Enable keyboard\n");
			break;
		case HW_CTL_IO_DIS_KBD:
			DBG_PRINT("[HWCTL]Disable keyboard\n");
			break;
/**/
/** /		case HW_CTL_IO_EN_MSG_NTF:
			//SET_BIT(LED_ON_FLAG,MSG_FLAG_BIT);
			LED_ON_FLAG = MSG_FLAG_BIT;
			if(copy_from_user(&level, ptr, sizeof(level)))
			{
				err = -EFAULT;
				break;
			}
			mutex_lock(&aw_mutex);
			on_level = level;
			mutex_unlock(&aw_mutex);
			DBG_PRINT("==== [HWCTL]line537:on_level = %x ====\n",on_level);
			hwctl_shut_charging_leds_and_dojobs();
			//Flush_led_data();///message : blue
			break;
		case HW_CTL_IO_DIS_MSG_NTF:			
			CLR_BIT(LED_ON_FLAG,MSG_FLAG_BIT);
			break;
/**/ 
/** /
	    case HW_CTL_IO_DIS_CALL_NTF:
			DBG_PRINT("==== [HWCTL]line 583:HW_CTL_IO_DIS_CALL_NTF ====\n");
			CLR_BIT(LED_ON_FLAG,CALL_FLAG_BIT);
			break;
		case HW_CTL_IO_EN_BAT_NTF:	
			DBG_PRINT("==== [HWCTL]line 587:HW_CTL_IO_EN_BAT_NTF ====\n");           
			//SET_BIT(LED_ON_FLAG,BAT_FLAG_BIT);
			LED_ON_FLAG = BAT_FLAG_BIT;
			Flush_led_data();///battert low:red
			break;
		case HW_CTL_IO_DIS_BAT_NTF:			
			CLR_BIT(LED_ON_FLAG,BAT_FLAG_BIT);
			break;
/**/

/** /
		case HW_CTL_IO_CHARGING_FULL_EN_NTF:	
            DBG_PRINT("==== [HWCTL]line 611:HW_CTL_IO_CHARGING_FULL_EN_NTF ====\n"); 
			//SET_BIT(LED_ON_FLAG,CHARGING_FULL_FLAG_BIT);
			LED_ON_FLAG = CHARGING_FULL_FLAG_BIT;
			Flush_led_data();///charging full : green
			break;	
		case HW_CTL_IO_CHARGING_FULL_DIS_NTF:	
			DBG_PRINT("==== [HWCTL]HW_CTL_IO_CHARGING_FULL_DIS_NTF ====\n"); 
			CLR_BIT(LED_ON_FLAG,CHARGING_FULL_FLAG_BIT);
			break;
		case HW_CTL_IO_CHARGING_DIS_NTF:	
			DBG_PRINT("==== [HWCTL]line 593:HW_CTL_IO_CHARGING_DIS_NTF ====\n");
			CLR_BIT(LED_ON_FLAG,CHARGING_FLAG_BIT);
			hwctl_led_off();
			break;	
		case HW_CTL_IO_LEFT_HAND_NTF:	////added by liyunpen20130219
			//tpd_toggle_hand_using(0);
			//SET_BIT(LED_ON_FLAG,CHARGING_FLAG_BIT);
			break;
		case HW_CTL_IO_RIGHT_HAND_NTF:	////added by liyunpen20130219
			//tpd_toggle_hand_using(1);
			//CLR_BIT(LED_ON_FLAG,CHARGING_FLAG_BIT);
			break;	
/**/
		default:
			break;
	}
	return 0;
}

static int hwctl_open(struct inode *inode, struct file *file)
{ 
   	return 0;
}

static int hwctl_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t hwctl_read(struct file * fp, char __user * to, size_t read_size, loff_t * pos)
{
#define VERSION_NO	"Hardware direct control driver :89.0 by david wang"

	ssize_t copy_size = strlen(VERSION_NO);

	DBG_PRINT("hwctl_read ops");

	return copy_to_user(to,VERSION_NO,copy_size);
}


#if 0//defined(CONFIG_HAS_EARLYSUSPEND)
static void hwdctl_early_suspend(struct early_suspend *h)
{
	Flush_led_data();
}

static void hwdctl_late_resume(struct early_suspend *h)
{
	Suspend_led();
}
#endif


void hwctl_shut_charging_leds_and_dojobs()
{
    DBG_PRINT("==== In hwctl_shut_charging_leds_and_dojobs ====\n");
	Suspend_led();
	CLR_BIT(LED_ON_FLAG,CHARGING_FLAG_BIT);
	CLR_BIT(LED_ON_FLAG,CHARGING_FULL_FLAG_BIT);

	Flush_led_data();
}

/*
//begin-Bee-20140417
static int aw2013_create_sysfs(struct i2c_client *client)
{
	int err;
	struct device *dev = &(client->dev);

	TS_DBG("%s", __func__);
	
	err = device_create_file(dev, &dev_attr_led);
	err = device_create_file(dev, &dev_attr_reg);
	return err;
}
*/

#if 0
void AW2013_Hw_reset(void)
{   
	mt_set_gpio_mode(AW2013_RESET_PIN, GPIO_LED_EN_M_GPIO);
	mt_set_gpio_dir(AW2013_RESET_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(AW2013_RESET_PIN, GPIO_OUT_ONE);
      msleep(5);
	mt_set_gpio_out(AW2013_RESET_PIN, GPIO_OUT_ZERO);
      msleep(9);
	mt_set_gpio_out(AW2013_RESET_PIN, GPIO_OUT_ONE);
      msleep(30);
}
#endif


static BOOL AW2013_i2c_write_reg(unsigned char reg,unsigned char data)
{
	char buf[2];

	buf[0]=reg;
	buf[1]=data;
	breathlight_master_send(0,buf,2);


}



void AW2013_test(void)
{


	 
   DBG_PRINT("AW2013_test_entry=\r\n");
   
   DBG_PRINT("AW2013_i2c_read_reg_0x00=%x\r\n",hwctl_i2c_read_reg(0x00));
   DBG_PRINT("AW2013_i2c_read_reg_0x01=%x\r\n",hwctl_i2c_read_reg(0x01)); 
   DBG_PRINT("AW2013_i2c_read_reg_0x02=%x\r\n",hwctl_i2c_read_reg(0x02));
   DBG_PRINT("AW2013_i2c_read_reg_0x03=%x\r\n",hwctl_i2c_read_reg(0x03)); 
   DBG_PRINT("AW2013_i2c_read_reg_0x04=%x\r\n",hwctl_i2c_read_reg(0x04));
   DBG_PRINT("AW2013_i2c_read_reg_0x05=%x\r\n",hwctl_i2c_read_reg(0x05)); 
   DBG_PRINT("AW2013_i2c_read_reg_0x06=%x\r\n",hwctl_i2c_read_reg(0x06));
   DBG_PRINT("AW2013_i2c_read_reg_0x07=%x\r\n",hwctl_i2c_read_reg(0x07));    
   DBG_PRINT("AW2013_i2c_read_reg_0x12=%x\r\n",hwctl_i2c_read_reg(0x12));
   DBG_PRINT("AW2013_i2c_read_reg_0x13=%x\r\n",hwctl_i2c_read_reg(0x13));   

}


void AW2013_OnOff(BOOL OnOff)  
{   
	char buf[2];
	//Paoma_cnt=0;
//	Breath_cnt=0;
	//mt_set_gpio_mode(AW2013_RESET_PIN, GPIO_LED_EN_M_GPIO);
	//mt_set_gpio_dir(AW2013_RESET_PIN, GPIO_DIR_OUT);
	
	buf[0]=0x7f;
	buf[1]=0x00;
	breathlight_master_send(0x7f,buf,2);
	//mt_set_gpio_out(AW2013_RESET_PIN, GPIO_OUT_ZERO);
      //msleep(5);
	
	//if (OnOff ==1)
	//{  
	   //mt_set_gpio_out(AW2013_RESET_PIN, GPIO_OUT_ONE);
	//}
      msleep(5);
	
}


//-------------------------------------------------------------------------------------------
//º¯ÊıÃû: AW2013_AllOn
//µ÷ÓÃ\u017dËº¯Êı£¬Ôò10Â·µÆÈ«ÁÁ£¬Ã¿Â·µÄµçÁ÷·Ö±ğÓÉÈí\u0152ş¶ÀÁ¢¿ØÖÆ¡£
//-------------------------------------------------------------------------------------------
void AW2013_AllOn(void)
{
	DBG_PRINT("-------------------------AW2013_AllOn  Entry ------------------------- \r\n");

	AW2013_OnOff(1);
	
	AW2013_i2c_write_reg(0x12,0x00);   //OUTÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x12,0x00);   //OUTÅäÖÃÎªºôÎüµÆÄ£Ê\u0153	 
	AW2013_i2c_write_reg(0x13,0x00);   //OUTÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x13,0x00);   //OUTÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x20,0x3f);//OUT0¿Úµ÷¹â£¬µ÷¹âµÈ\u0152¶Îª0-255¡£OUT0~OUT5µÄµ÷¹âÖ\u017eÁîÒÀ\u017dÎÎª0x20~0x25. Ğ\u017d0¹Ø±Õ
	AW2013_i2c_write_reg(0x21,0x3f);
	AW2013_i2c_write_reg(0x22,0x3f);
	AW2013_i2c_write_reg(0x23,0x3f);
	AW2013_i2c_write_reg(0x24,0x3f);
	AW2013_i2c_write_reg(0x25,0x3f);
}

void AW2013_out0_fade(void)
{
	AW2013_i2c_write_reg(0x12,0x00);   //OUT4~5ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153Õâ\u0178äÈç¹ûÖ®Ç°ÉèÖÃ¹ı£¬¿ÉÒÔ²»Òª
	AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153Õâ\u0178äÈç¹ûÖ®Ç°ÉèÖÃ¹ı£¬¿ÉÒÔ²»Òª
	AW2013_i2c_write_reg(0x04,0x03); 	 //OUT4-OUT5×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹Ä£?0ÎªblinkÄ£Ê\u0153£¬1ÎªFADEÄ£Ê\u0153
	AW2013_i2c_write_reg(0x05,0x0e);   //OUT0-OUT3×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹ÄÜ 0ÎªblinkÄ£Ê\u0153£¬1ÎªFADEÄ£Ê\u0153?	AW2013_i2c_write_reg(0x15,0x09);   //µ­\u0153øµ­³öÊ±\u0152äÉèÖÃÕâ\u0178äÈç¹ûÖ®Ç°ÉèÖÃ¹ı£¬¿ÉÒÔ²»Òª

	AW2013_i2c_write_reg(0x03,0x00);   //ÏÈ°Ñ03HÇå0
	AW2013_i2c_write_reg(0x03,0x01);   //\u017dÓ0Ğ\u017dµ\u01531Ôò²úÉúµ­\u0153ø¹ı³Ì
	//AW2013_i2c_write_reg(0x03,0x00);   //\u017dÓ1Ğ\u017dµ\u01530Ôò²úÉúµ­³öø¹ı³? 
}

//-------------------------------------------------------------------------------------------
//ÒÔÏÂÎªAW2013ÊµÏÖ\u0152\u017eÖÖĞ§¹ûµÄ²Î¿\u0152º¯Êı£¬ÓÃ»§Çë\u017eù\u0178İ×Ô\u0152ºµÄĞ§¹ûĞèÒªÀ\u017dµ÷Õû¡£
//-------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------
//º¯ÊıÃû: AW2013_init_pattern
//µ÷ÓÃ\u017dËº¯Êı£¬Ôò6Â·µÆÈ«ÁÁ£¬Îª×ÔÖ÷ºôÎüBLINK Ä£Ê\u0153
//-------------------------------------------------------------------------------------------
void AW2013_init_pattern(void) 
{
 //AW2013»ù±\u0178µÄĞ§¹ûÊµÏÖ¡£6Â·×ÔÖ÷ºôÎü
	DBG_PRINT("-------------------------AW2013_init_pattern  Entry ------------------------- \r\n");

	// AW2013_OnOff(1);
	 AW2013_i2c_write_reg(0x12,0x00);   //OUT4~5ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	 AW2013_i2c_write_reg(0x12,0x00);   //OUT4~5ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153	 
	 AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	 AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	 AW2013_i2c_write_reg(0x04,0x03);   //OUT4-OUT5×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹ÄÜ
	 AW2013_i2c_write_reg(0x05,0x0f);   //OUT0-OUT3×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹ÄÜ
	 
	 AW2013_i2c_write_reg(0x15,0x1b); 	 //µ­\u0153øµ­³öÊ±\u0152äÉèÖÃ
	 AW2013_i2c_write_reg(0x16,0x12);	 //È«ÁÁÈ«°µÊ±\u0152äÉèÖÃ 
	  	
	 AW2013_i2c_write_reg(0x17,0x00);	 //OUT0Æô¶¯ÑÓÊ±  
	 AW2013_i2c_write_reg(0x18,0);	//OUT1Æô¶¯ÑÓÊ± 
	 AW2013_i2c_write_reg(0x19,0);	//OUT2Æô¶¯ÑÓÊ±   
	 AW2013_i2c_write_reg(0x1a,0);  //OUT3Æô¶¯ÑÓÊ± 
	 AW2013_i2c_write_reg(0x1b,0);  //OUT4Æô¶¯ÑÓÊ± 
	 AW2013_i2c_write_reg(0x1c,0);  //OUT5Æô¶¯ÑÓÊ± 
	 
	 AW2013_i2c_write_reg(0x14,0xff);//×ÔÖ÷ºôÎüÊ¹ÄÜ 

	 AW2013_i2c_write_reg(0x11,0x82);	//¿ªÊ\u0152×ÔÖ÷ºôÎü£¬²¢ÉèÖÃ×î\u017dóµçÁ÷
	 AW2013_i2c_write_reg(0x11,0x82);	

	 //Èç¹û²»²ÉÓÃ×ÔÖ÷ºôÎüÄ£Ê\u0153£¬Ôò²ÉÓÃÏÂÁĞ·\u0153Ê\u0153µ÷¹â
	//AW2013_i2c_write_reg(0x14,0x00);//¹Ø±Õ×ÔÖ÷ºôÎüÊ¹ÄÜ
	//AW2013_i2c_write_reg(0x20,0x3f);//OUT0¿Úµ÷¹â£¬µ÷¹âµÈ\u0152¶Îª0-255¡£OUT0~OUT5µÄµ÷¹âÖ\u017eÁîÒÀ\u017dÎÎª0x20~0x25. Ğ\u017d0¹Ø±Õ
	//AW2013_i2c_write_reg(0x21,0x3f);
	//AW2013_i2c_write_reg(0x22,0x3f);
	//AW2013_i2c_write_reg(0x23,0x3f);
	//AW2013_i2c_write_reg(0x24,0x3f);
	//AW2013_i2c_write_reg(0x25,0x3f);

	 
	 AW2013_test();  

	 
}

//-------------------------------------------------------------------------------------------
//º¯ÊıÃû: AW2013_Marquee
//ÅÜÂíµÆĞ§¹û
//-------------------------------------------------------------------------------------------

#if 0   //  ning 

void AW2013_Marquee_loop(void)
{
	switch(Paoma_cnt)
	{
		case 0:			
			AW2013_i2c_write_reg(0x25,0x00); //¹Ø±ÕÉÏÒ»¿ÅµÆ
			AW2013_i2c_write_reg(0x20,0x3f); //\u017dò¿ªÒ»¿ÅµÆ
			break;
		case 1:
			AW2013_i2c_write_reg(0x20,0x00); //¹Ø±ÕÉÏÒ»¿ÅµÆ
			AW2013_i2c_write_reg(0x21,0x3f); //\u017dò¿ªÒ»¿ÅµÆ
			break;			
		case 2:
			AW2013_i2c_write_reg(0x21,0x00); 
			AW2013_i2c_write_reg(0x22,0x3f); 
			break;	
		case 3:
			AW2013_i2c_write_reg(0x22,0x00); 
			AW2013_i2c_write_reg(0x23,0x3f); 
			break;
		case 4:
			AW2013_i2c_write_reg(0x23,0x00); 
			AW2013_i2c_write_reg(0x24,0x3f); 
			break;	
		case 5:
			AW2013_i2c_write_reg(0x24,0x00); 
			AW2013_i2c_write_reg(0x25,0x3f); 
			break;	
		default:
			break;
	}
	if(Paoma_cnt<5)	
		Paoma_cnt++;
	else
		Paoma_cnt=0;
	GPTI_StartItem(AW2013_timer_ID,12,AW2013_Marquee_loop,NULL);

	


	
}

#endif

void hwctl_set_blink(uint32_t time)
{
    switch(time)
    {
        case 1499:
            blink = 0x03;
            break;
        case 1000:
            blink = 0x02;
            break;
        case 500:
            blink = 0x00;
            break;
        case 125:
            blink = 0x03;
            break;
        default:
            blink = 9;
            break;
    }
/** /    
    if(time == 1499)
    {
        blink = 0x03;    
    }
    else if(time == 1000)
    {
        blink = 0x02;
    }
    else if(time == 500)
    {
        blink = 0x00;
    }
    else
    {
        blink = 9;      
    }
/**/
    DBG_PRINT("==== [HWCTL]line939:hwctl_set_blink blink=%d ====\n",blink);
}

void aw2013_get_leds_status(struct led_setting *led0,struct led_setting *led1,struct led_setting *led2)
{
    DBG_PRINT("==== [aw2013]line943:aw2013_get_leds_status ====\n");
    unsigned int color,red,green,blue;

    led0->flag = 0;
    led1->flag = 0;
    led2->flag = 0;
    color = (on_level>> 24) & 0xFF;
    if(color) {
    	red = (on_level >> 16) & 0xFF;
    	green = (on_level >> 8) & 0xFF;
    	blue = on_level & 0xFF;
    }else {
    	red = green = blue = 0;
    }
    DBG_PRINT("==== [aw2013]aw2013_get_leds_status:red=%x,green=%x,blue=%x ====\n",red,green,blue);  
    if(red) {
        led0->flag = 1;
        led0->level = red;
        DBG_PRINT("==== red:led0=%d ====\n",led0);
    }
    if(green) {
        led1->flag = 1;
        led1->level = green;
        DBG_PRINT("==== green:led1=%d ====\n",led1);
    }
    if(blue) {            
        led2->flag = 1;
        led2->level = blue;
        DBG_PRINT("==== blue:led2=%d ====\n",led2);
    }
}

void aw2013_all_leds_control()
{
    DBG_PRINT("==== [aw2013]line971:aw2013_all_leds_control ====\n");
    
    aw2013_get_leds_status(&led0,&led1,&led2);

    DBG_PRINT("==== [aw2013]aw2013_all_leds_control:led0=%d,led1=%d,led2=%d ====\n",led0,led1,led2);
    
    aw2013_breath_all(led0,led1,led2);
}

#if 0
void AW2013_breath_all(int led0,int led1,int led2)  //led on=0x01   ledoff=0x00
{  

	//write_reg(0x00, 0x55);				// Reset 
	hwctl_i2c_write_reg(0x01, 0x01);		// enable LED ²»Ê¹ÓÃÖĞ¶Ï		

	hwctl_i2c_write_reg(0x31, Imax|0x70);	//config mode, IMAX = 5mA	
	hwctl_i2c_write_reg(0x32, Imax|0x70);	//config mode, IMAX = 5mA	
	hwctl_i2c_write_reg(0x33, Imax|0x70);	//config mode, IMAX = 5mA	

	hwctl_i2c_write_reg(0x34, 0xff);	// LED0 level,
	hwctl_i2c_write_reg(0x35, 0xff);	// LED1 level,
	hwctl_i2c_write_reg(0x36, 0xff);	// LED2 level,
											
	hwctl_i2c_write_reg(0x37, Rise_time<<4 | Hold_time);	//led0  ÉÏÉıÊ±\u0152ä£¬±£³ÖÊ±\u0152äÉè¶\u0161							
	hwctl_i2c_write_reg(0x38, Fall_time<<4 | Off_time);	       //led0 ÏÂ\u0153µÊ±\u0152ä£¬¹Ø±ÕÊ±\u0152äÉè¶\u0161
	hwctl_i2c_write_reg(0x39, Delay_time<<4| Period_Num);   //led0  ºôÎüÑÓ³ÙÊ±\u0152ä£¬ºôÎüÖÜÆÚÉè¶\u0161

	hwctl_i2c_write_reg(0x3a, Rise_time<<4 | Hold_time);	//led1ÉÏÉıÊ±\u0152ä£¬±£³ÖÊ±\u0152äÉè¶\u0161								
	hwctl_i2c_write_reg(0x3b, Fall_time<<4 | Off_time);	       //led1 ÏÂ\u0153µÊ±\u0152ä£¬¹Ø±ÕÊ±\u0152äÉè¶\u0161
	hwctl_i2c_write_reg(0x3c, Delay_time<<4| Period_Num);   //led1  ºôÎüÑÓ³ÙÊ±\u0152ä£¬ºôÎüÖÜÆÚÉè¶\u0161

	hwctl_i2c_write_reg(0x3d, Rise_time<<4 | Hold_time);	//led2  ÉÏÉıÊ±\u0152ä£¬±£³ÖÊ±\u0152äÉè¶\u0161				
	hwctl_i2c_write_reg(0x3e, Fall_time<<4 | Off_time);	       //led2 ÏÂ\u0153µÊ±\u0152ä£¬¹Ø±ÕÊ±\u0152äÉè¶\u0161
	hwctl_i2c_write_reg(0x3f, Delay_time<<4| Period_Num);    //ºôÎüÑÓ³ÙÊ±\u0152ä£¬ºôÎüÖÜÆÚÉè¶\u0161

	hwctl_i2c_write_reg(0x30, led2<<2|led1<<1|led0);	       //led on=0x01 ledoff=0x00	
	hwctl_delay_1us(8);//ĞèÑÓÊ±5usÒÔÉÏ
}
#else
void aw2013_breath_all(struct led_setting led0,struct led_setting led1,struct led_setting led2)  //led on=0x01   ledoff=0x00
{
    DBG_PRINT("==== [HWCTL]line951:AW2013_breath_all blink=%d ====\n",blink);
    
	hwctl_i2c_write_reg(0x00, 0x55);		// Reset 
	hwctl_i2c_write_reg(0x01, 0x01);		// enable LED ²»Ê¹ÓÃÖĞ¶Ï		

    if(blink == 9)
    {
    	if(led0.flag == 1)
    	{
            hwctl_i2c_write_reg(0x31, Imax|0x03);   //pwm IMAX = 5mA 
            hwctl_i2c_write_reg(0x34, 0xff);	// LED0 level,
    	}
    	if(led1.flag== 1)
    	{

            hwctl_i2c_write_reg(0x32, Imax|0x03);   //pwm IMAX = 5mA 
            if((led0.flag== 1) && (led2.flag!= 1))
            {   hwctl_i2c_write_reg(0x35, 0xa5);	// LED1 level,
            }
            else
            {
               hwctl_i2c_write_reg(0x35, 0xff); 
            }
    	}
    	if(led2.flag== 1)
    	{
            hwctl_i2c_write_reg(0x33, Imax|0x03);   //pwm IMAX = 5mA 
            hwctl_i2c_write_reg(0x36, 0xff);	// LED2 level,
    	}
    }
    else
    {
    	hwctl_i2c_write_reg(0x31, Imax|0x70);	//config mode, IMAX = 5mA	
    	hwctl_i2c_write_reg(0x32, Imax|0x70);	//config mode, IMAX = 5mA	
    	hwctl_i2c_write_reg(0x33, Imax|0x70);	//config mode, IMAX = 5mA	

    	hwctl_i2c_write_reg(0x34, 0xff);	// LED0 level,
    	hwctl_i2c_write_reg(0x35, 0xff);	// LED1 level,
    	hwctl_i2c_write_reg(0x36, 0xff);	// LED2 level,
    											
    	hwctl_i2c_write_reg(0x37, Rise_time<<4 | blink);	//led0  ÉÏÉıÊ±\u0152ä£¬±£³ÖÊ±\u0152äÉè¶\u0161							
    	hwctl_i2c_write_reg(0x38, Fall_time<<4 | blink);	       //led0 ÏÂ\u0153µÊ±\u0152ä£¬¹Ø±ÕÊ±\u0152äÉè¶\u0161
    	hwctl_i2c_write_reg(0x39, Delay_time<<4| Period_Num);   //led0  ºôÎüÑÓ³ÙÊ±\u0152ä£¬ºôÎüÖÜÆÚÉè¶\u0161

    	hwctl_i2c_write_reg(0x3a, Rise_time<<4 | blink);	//led1ÉÏÉıÊ±\u0152ä£¬±£³ÖÊ±\u0152äÉè¶\u0161								
    	hwctl_i2c_write_reg(0x3b, Fall_time<<4 | blink);	       //led1 ÏÂ\u0153µÊ±\u0152ä£¬¹Ø±ÕÊ±\u0152äÉè¶\u0161
    	hwctl_i2c_write_reg(0x3c, Delay_time<<4| Period_Num);   //led1  ºôÎüÑÓ³ÙÊ±\u0152ä£¬ºôÎüÖÜÆÚÉè¶\u0161

    	hwctl_i2c_write_reg(0x3d, Rise_time<<4 | blink);	//led2  ÉÏÉıÊ±\u0152ä£¬±£³ÖÊ±\u0152äÉè¶\u0161				
    	hwctl_i2c_write_reg(0x3e, Fall_time<<4 | blink);	       //led2 ÏÂ\u0153µÊ±\u0152ä£¬¹Ø±ÕÊ±\u0152äÉè¶\u0161
    	hwctl_i2c_write_reg(0x3f, Delay_time<<4| Period_Num);    //ºôÎüÑÓ³ÙÊ±\u0152ä£¬ºôÎüÖÜÆÚÉè¶\u0161
    }

	hwctl_i2c_write_reg(0x30, led2.flag<<2 | led1.flag<<1 | led0.flag);	       //led on=0x01 ledoff=0x00	
	hwctl_delay_1us(8);//ĞèÑÓÊ±5usÒÔÉÏ
}
#endif

void AW2013_Marquee(void)
{
	DBG_PRINT("-------------------------AW2013_Marquee  Entry ------------------------- \r\n");

	AW2013_OnOff(1);
	AW2013_i2c_write_reg(0x12,0x00);   //OUT4~5ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x12,0x00);   //OUT4~5ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153	 
	AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	//AW2013_Marquee_loop();
}


//-------------------------------------------------------------------------------------------
//º¯ÊıÃû: AW2013_ComingCall
//À\u017dµçÊ±µÄĞ§¹û¡£6Â·×ÔÖ÷ºôÎü£¬ºôÎüÆµÂÊ\u0153Ï¿ì
//-------------------------------------------------------------------------------------------
void AW2013_ComingCall(void)
{
	DBG_PRINT("-------------------------AW2013_ComingCall  Entry ------------------------- \r\n");

	AW2013_OnOff(1);
	AW2013_i2c_write_reg(0x12,0x00);   //OUT4~5ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x12,0x00);   //OUT4~5ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153	 
	AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	
	 AW2013_i2c_write_reg(0x04,0x03);   //OUT4-OUT5×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹ÄÜ
	 AW2013_i2c_write_reg(0x05,0x0f);   //OUT0-OUT3×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹ÄÜ	 
	 AW2013_i2c_write_reg(0x15,0x12); 	 //µ­\u0153øµ­³öÊ±\u0152äÉèÖÃ
	 AW2013_i2c_write_reg(0x16,0x09);	 //È«ÁÁÈ«°µÊ±\u0152äÉèÖÃ 	  	 	 
	 AW2013_i2c_write_reg(0x14,0x3f);//×ÔÖ÷ºôÎüÊ¹ÄÜ 	 
	 AW2013_i2c_write_reg(0x11,0x82);	
}
//-------------------------------------------------------------------------------------------
//º¯ÊıÃû: AW2013_MissingCall
//ÓĞÎ\u017d\u0153ÓÀ\u017dµçÊ±µÄĞ§¹û¡£Ö»ÓĞ2 Â·×ÔÖ÷ºôÎü£¬ÆäËûµÆÃğ¡£ºôÎüÆµÂÊ\u0153ÏÂı£¬²¢ÇÒÓĞÈ«°µÊ±\u0152ä
//-------------------------------------------------------------------------------------------
void AW2013_MissingCall(void)
{

	DBG_PRINT("-------------------------AW2013_MissingCall  Entry ------------------------- \r\n");

	AW2013_OnOff(1);
	AW2013_i2c_write_reg(0x12,0x00);   //OUT4~5ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x12,0x00);   //OUT4~5ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153	 
	AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	
	 AW2013_i2c_write_reg(0x04,0x03);   //OUT4-OUT5×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹ÄÜ
	 AW2013_i2c_write_reg(0x05,0x0f);   //OUT0-OUT3×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹ÄÜ	 
	 AW2013_i2c_write_reg(0x15,0x1b); 	 //µ­\u0153øµ­³öÊ±\u0152äÉèÖÃ
	 AW2013_i2c_write_reg(0x16,0x19);	 //È«ÁÁÈ«°µÊ±\u0152äÉèÖÃ 	  	 	 
	 AW2013_i2c_write_reg(0x14,0x03);//×ÔÖ÷ºôÎüÊ¹ÄÜ 	 
	 AW2013_i2c_write_reg(0x11,0x83);	
}
//-------------------------------------------------------------------------------------------
//º¯ÊıÃû: AW2013_ComingMsg
//À\u017d¶ÌĞÅÊ±µÄĞ§¹û¡£1Â·×ÔÖ÷ºôÎü£¬ºôÎüÆµÂÊ\u0153Ï¿ì
//-------------------------------------------------------------------------------------------
void AW2013_ComingMsg(void)
{

	DBG_PRINT("-------------------------AW2013_ComingMsg Entry ------------------------- \r\n");

	AW2013_OnOff(1);
	AW2013_i2c_write_reg(0x12,0x00);   //OUT4~5ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x12,0x00);   //OUT4~5ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153	 
	AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	
	 AW2013_i2c_write_reg(0x04,0x03);   //OUT4-OUT5×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹ÄÜ
	 AW2013_i2c_write_reg(0x05,0x0f);   //OUT0-OUT3×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹ÄÜ	 
	 AW2013_i2c_write_reg(0x15,0x09); 	 //µ­\u0153øµ­³öÊ±\u0152äÉèÖÃ
	 AW2013_i2c_write_reg(0x16,0x10);	 //È«ÁÁÈ«°µÊ±\u0152äÉèÖÃ 	  	 	 
	 AW2013_i2c_write_reg(0x14,0x01);    //×ÔÖ÷ºôÎüÊ¹ÄÜ 	 
	 AW2013_i2c_write_reg(0x11,0x82);	
}
//-------------------------------------------------------------------------------------------
//º¯ÊıÃû: AW2013_MissingMsg
//ÓĞÎ\u017d\u0153Ó  ¶ÌĞÅÊ±µÄĞ§¹û¡£Ö»ÓĞÒ»Â·×ÔÖ÷ºôÎü£¬ºôÎüÆµÂÊ\u0153ÏÂı£¬²¢ÇÒÓĞÈ«°µÊ±\u0152ä
//-------------------------------------------------------------------------------------------
void AW2013_MissingMsg (void)
{
	DBG_PRINT("-------------------------AW2013_MissingMsg Entry ------------------------- \r\n");
	AW2013_OnOff(1);
	AW2013_i2c_write_reg(0x12,0x00);   //OUT4~5ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x12,0x00);   //OUT4~5ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153	 
	AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	
	 AW2013_i2c_write_reg(0x04,0x03);   //OUT4-OUT5×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹ÄÜ
	 AW2013_i2c_write_reg(0x05,0x0f);   //OUT0-OUT3×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹ÄÜ	 
	 AW2013_i2c_write_reg(0x15,0x09); 	 //µ­\u0153øµ­³öÊ±\u0152äÉèÖÃ
	 AW2013_i2c_write_reg(0x16,0x20);	 //È«ÁÁÈ«°µÊ±\u0152äÉèÖÃ 	  	 	 
	 AW2013_i2c_write_reg(0x14,0x01);//×ÔÖ÷ºôÎüÊ¹ÄÜ 	 
	 AW2013_i2c_write_reg(0x11,0x82);	
}

//-------------------------------------------------------------------------------------------
//º¯ÊıÃû: AW2013_GPIO_LED
//µ±ÓÃGPIO 6789À\u017d¿ØÖÆLEDµÄÑô\u0152«£¬ÔòÏÂÃæÕâ\u017eöº¯ÊıÄÜÊµÏÖ»ù±\u0178µÄºôÎü¹\u0160ÄÜ
//-------------------------------------------------------------------------------------------
void AW2013_GPIO_LED(void)
{

	DBG_PRINT("-------------------------AW2013_GPIO_LED Entry ------------------------- \r\n");

	AW2013_OnOff(1);
	AW2013_i2c_write_reg(0x12,0xff);   //OUT4¡¢5ÅäÖÃÎªGPIOÄ£Ê\u0153
	AW2013_i2c_write_reg(0x12,0xff);   //OUT4¡¢5ÅäÖÃÎªGPIOÄ£Ê\u0153
	AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x13,0x00);   //OUT0~3ÅäÖÃÎªºôÎüµÆÄ£Ê\u0153

	//ÏÂÃæÕâÁ\u0153ÌõÖ\u017eÁîÅäÖÃOUT4~5 Êä³ö\u017eßµçÆ\u0153¡£
	//ÓÃ»§¿É\u017eù\u0178İĞèÒªÀ\u017dµ÷ÕûÄ³\u0152\u017eÂ·Ñô\u0152«Îª\u017eß£¬»òÍ\u0161¹ıtimerÀ\u017dÑ­»·ÒÔ\u017dïµ\u0153ÏëÒªµÄĞ§¹û
	AW2013_i2c_write_reg(0x11,0x10);   //OUT6 ~9 ÅäÖÃÎªÍÆÍìÄ£Ê\u0153
	AW2013_i2c_write_reg(0x02,0xff);   //OUT4~5Êä³ö\u017eßµçÆ\u0153¡£
	
	 AW2013_i2c_write_reg(0x04,0x03);	//OUT4-OUT5×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹ÄÜ
	 AW2013_i2c_write_reg(0x05,0x0f);	//OUT0-OUT3×ÔÖ÷ºôÎüBLINKÄ£Ê\u0153Ê¹ÄÜ	 
	 AW2013_i2c_write_reg(0x15,0x12);	 //µ­\u0153øµ­³öÊ±\u0152äÉèÖÃ
	 AW2013_i2c_write_reg(0x16,0x09);	 //È«ÁÁÈ«°µÊ±\u0152äÉèÖÃ 			 
	 AW2013_i2c_write_reg(0x14,0x3f);	 //×ÔÖ÷ºôÎüÊ¹ÄÜ 	 
	 AW2013_i2c_write_reg(0x11,0x92);     //¿ªÊ\u0152×ÔÖ÷ºôÎü¡£×¢Òâ	OUT6 ~9 ±ØĞëÅäÖÃÎªÍÆÍìÄ£Ê\u0153
}


//-------------------------------------------------------------------------------------------
//-----------AW2013 »ù±\u0178µÆĞ§º¯Êı¶\u0161Òå end----------------------------------------------
//-------------------------------------------------------------------------------------------



void AW2013_init(void)    
{	
	//AW2013_Hw_reset();
		
	AW2013_i2c_write_reg(0x12,0x00);   //P0¿ÚÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	AW2013_i2c_write_reg(0x13,0x00);   //P1¿ÚÅäÖÃÎªºôÎüµÆÄ£Ê\u0153
	
	AW2013_i2c_write_reg(0x20,0x3f);//OUT0¿Úµ÷¹â£¬µ÷¹âµÈ\u0152¶Îª0-255¡£OUT0~OUT5µÄµ÷¹âÖ\u017eÁîÒÀ\u017dÎÎª0x20~0x2f. Ğ\u017d0¹Ø±Õ
	AW2013_i2c_write_reg(0x21,0x3f);
	AW2013_i2c_write_reg(0x22,0x3f);
	AW2013_i2c_write_reg(0x23,0x3f);
	AW2013_i2c_write_reg(0x24,0x3f);
	AW2013_i2c_write_reg(0x25,0x3f);
	
	AW2013_test();  //¶ÁÈ¡AW2013ÄÚ²¿\u0152Ä\u017dæÆ÷µÄÖµ£¬¿\u017dÓëĞ\u017d\u0153øÈ¥µÄÖµÊÇ·ñÒ»ÖÂ£¬ÓÉ\u017dËÅĞ¶ÏI2C\u0153Ó¿ÚÊÇ·ñÍ\u0161³©
}




static int __devinit AW2013_i2c_probe(struct i2c_client *client,
				      const struct i2c_device_id *id)
{
	DBG_PRINT("AW2013_i2c_probe:OK");


	aw2013_i2c_client = client;



	DBG_PRINT("****************** ningshuigen    AW2013_i2c_probe addr %x   " , aw2013_i2c_client->addr   );


	AW2013_init();

      // AW2013_init_pattern();



      AW2013_AllOn();

//	led_flash_aw2013_test(0);///z

	
//	led_flash_aw2013_power_low();
//	led_flash_aw2013_charging_full();
//	led_flash_aw2013_charging();
//	led_flash_aw2013_unanswer_message_incall();
	//aw2013_create_sysfs(client);

	return 0;
}

static int __devexit AW2013_i2c_remove(struct i2c_client *client)
{
	aw2013_i2c_client = NULL;
	return 0;
}


static int __init AW2013_Driver_Init(void) 
{
	int ret;


	DBG_PRINT("********************    99999999999    ning AW2013_Driver_Init:start");

//	#err
	i2c_register_board_info(AW2013_I2C_BUS_NUM, &aw2013_i2c_hw, 1);
		 
	ret = i2c_add_driver(&aw2013_i2c_driver);

	
	DBG_PRINT("AW2013_Driver_Init:start \n");
	
	if(0!=alloc_chrdev_region(&g_hwctl_device.hwctl_dev_no,HW_DEVICE_MINOR,HW_DEVICE_COUNT,HW_DEVICE_NAME))
	{
		DBG_PRINT("hwctl_driver_alloc chrdev region:fail");
		DBG_PRINT("hwctl_driver_alloc chrdev region:fail \n");
		goto init_error;
		goto init_error;
		goto init_error;
		goto init_error;
	}	

	
	DBG_PRINT("hwctl_driver_alloc chrdev region:OK");
	
	g_hwctl_device.init_stage = DEV_ALLOC_REGION;
	
	DBG_PRINT("hwctl_driver_alloc chrdev region:OK1");
	g_hwctl_device.hw_cdev = cdev_alloc();
	DBG_PRINT("hwctl_driver_alloc chrdev region:OK2");
    	g_hwctl_device.hw_cdev->owner = THIS_MODULE;
    	DBG_PRINT("hwctl_driver_alloc chrdev region:OK3");
    	g_hwctl_device.hw_cdev->ops = &hwctl_fops; 
    	DBG_PRINT("hwctl_driver_alloc chrdev region:OK4");
    	ret = cdev_add(g_hwctl_device.hw_cdev, g_hwctl_device.hwctl_dev_no, 1);//
	DBG_PRINT("hwctl cdev_add_ret_is %d \n",ret);
	if(ret)
	{
		DBG_PRINT("hwctl_driver add cdev error\n");		
		goto init_error;
	}
	g_hwctl_device.init_stage = DEV_ADD_CDEV;
	g_hwctl_device.hw_class = class_create(THIS_MODULE, HW_DEVICE_NAME);
	
	g_hwctl_device.init_stage = DEV_ALLOC_CLASS;

	// if we want auto creat device node, we must call this
	g_hwctl_device.hw_device = device_create(g_hwctl_device.hw_class, NULL, g_hwctl_device.hwctl_dev_no, NULL, HW_DEVICE_NAME); 
	g_hwctl_device.init_stage = DEV_INIT_ALL;
	#if 0
	led_flash_aw2013_test(0);///z
	led_flash_aw2013_power_low();
	led_flash_aw2013_charging_full();
	led_flash_aw2013_charging();
	led_flash_aw2013_unanswer_message_incall();
	#endif

#if 0//def CONFIG_HAS_EARLYSUSPEND
	g_hwctl_device.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 20;
	g_hwctl_device.early_suspend.suspend = hwdctl_early_suspend;
	g_hwctl_device.early_suspend.resume = hwdctl_late_resume;
	register_early_suspend(&g_hwctl_device.early_suspend);
#endif
	return 0;

init_error:
	if(g_hwctl_device.init_stage == DEV_ALLOC_REGION)
		unregister_chrdev_region(g_hwctl_device.hwctl_dev_no,1);
	return (-1);
}



/* should never be called */
static void __exit AW2013_Driver_Exit(void) 
{
	if(g_hwctl_device.init_stage == DEV_INIT_ALL)
	{
		device_del(g_hwctl_device.hw_device);
		class_destroy(g_hwctl_device.hw_class);
		cdev_del(g_hwctl_device.hw_cdev);
		unregister_chrdev_region(g_hwctl_device.hwctl_dev_no,1);
	}
		i2c_del_driver(&aw2013_i2c_driver);
}

module_init(AW2013_Driver_Init);
module_exit(AW2013_Driver_Exit);
MODULE_DESCRIPTION("Linux HW direct control driver");
MODULE_AUTHOR("David.wang(softnow@live.cn)");
MODULE_LICENSE("GPL");
