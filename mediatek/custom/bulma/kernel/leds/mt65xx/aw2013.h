#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/ctype.h>

#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/workqueue.h>
#include <linux/switch.h>
#include <linux/delay.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/time.h>
#include <linux/string.h>
#include <mach/irqs.h>

#include <mach/mt_gpio.h>
#include <mach/mt_gpt.h>
#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define HWCTL_DBG

#define HW_DEVICE_MINOR	(0)
#define HW_DEVICE_COUNT	(1)
#define HW_DEVICE_NAME	"hwdctl"

#define HW_CTL_IO_TEST			_IO('H', 0x00)
#define HW_CTL_IO_ENB_KBD		_IO('H', 0x01)
#define HW_CTL_IO_DIS_KBD		_IO('H', 0x02)
//#define HW_CTL_IO_EN_MSG_NTF	_IO('H', 0x03)
#define HW_CTL_IO_LED_COLOR	    _IO('H', 0x03)
#define HW_CTL_IO_DIS_MSG_NTF	_IO('H', 0x04)
#define HW_CTL_IO_EN_CALL_NTF	_IO('H', 0x05)
#define HW_CTL_IO_DIS_CALL_NTF	_IO('H', 0x06)
#define HW_CTL_IO_EN_BAT_NTF	_IO('H', 0x07)
#define HW_CTL_IO_DIS_BAT_NTF	_IO('H', 0x08)
#define HW_CTL_IO_CHARGING_EN_NTF	_IO('H', 0x09)
#define HW_CTL_IO_CHARGING_DIS_NTF	_IO('H', 0x0A)
#define HW_CTL_IO_LEFT_HAND_NTF		_IO('H', 0x0B)/////added by liyunpen 20130219
#define HW_CTL_IO_RIGHT_HAND_NTF	_IO('H', 0x0C)/////added by liyunpen20130219
#define HW_CTL_IO_CHARGING_FULL_EN_NTF	_IO('H', 0x0D)
#define HW_CTL_IO_CHARGING_FULL_DIS_NTF	_IO('H', 0x0E)
#define HW_CTL_IO_SET_BLINK		_IO('H', 0x0F)

#define HW_CTL_IO_FORCE_REFRESH_LEDS	_IO('H', 0xF0)

typedef enum{
	DEV_NONE_STAGE = 0x0,
	DEV_ALLOC_REGION,
	DEV_ALLOC_CDEV,	
	DEV_ADD_CDEV,
	DEV_ALLOC_CLASS,
	DEV_INIT_ALL
}HWDEV_INIT_STAGE;

typedef struct __HW_DEVICE{
	dev_t hwctl_dev_no;
	struct cdev * hw_cdev;
	struct class *hw_class;
	struct device *hw_device;
	char init_stage;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif				/* CONFIG_HAS_EARLYSUSPEND */	
}HW_DEVICE , * p_HW_DEVICE;

struct led_setting{
    unsigned int flag;
    //unsigned int blink;
    int level;    
};

#define GPIO_InitIO(dir,pin) 	 mt_set_gpio_dir(pin,dir)
#define GPIO_WriteIO(level,pin)  mt_set_gpio_out(pin,level)
#define GPIO_ReadIO(pin)	        mt_get_gpio_out(pin)


//#define AW2013_SDA_PIN				214//66                //I2C data 配置
//#define AW2013_SCK_PIN				215//67                //I2C clock 配置

#define GPIO_LED_EN 11 //liubiao
#define GPIO_LED_EN_M_GPIO 12
#define aw2013_RESET_PIN GPIO_LED_EN                //aw2013的RESET脚


#define AW2013_I2C_ADDRESS_WRITE			0x8A             //I2C 写地址
#define AW2013_I2C_ADDRESS_READ			0x8B             //I2C 读地址

#define AW2013_I2C_MAX_LOOP 		50   

#define I2C_delay 		2    //可根据平台调整,保证I2C速度不高于400k

#define AW2013_I2C_BUS_NUM 0

//以下为调节呼吸效果的参数
#define Imax          0x02   //LED最大电流配置,0x00=omA,0x01=5mA,0x02=10mA,0x03=15mA,
#define Rise_time   0x00   //LED呼吸上升时间,0x00=0.13s,0x01=0.26s,0x02=0.52s,0x03=1.04s,0x04=2.08s,0x05=4.16s,0x06=8.32s,0x07=16.64s
#define Hold_time   0x01   //LED呼吸到最亮时的保持时间0x00=0.13s,0x01=0.26s,0x02=0.52s,0x03=1.04s,0x04=2.08s,0x05=4.16s
#define Fall_time     0x00   //LED呼吸下降时间,0x00=0.13s,0x01=0.26s,0x02=0.52s,0x03=1.04s,0x04=2.08s,0x05=4.16s,0x06=8.32s,0x07=16.64s
#define Off_time      0x01   //LED呼吸到灭时的保持时间0x00=0.13s,0x01=0.26s,0x02=0.52s,0x03=1.04s,0x04=2.08s,0x05=4.16s,0x06=8.32s,0x07=16.64s
#define Delay_time   0x00   //LED呼吸启动后的延迟时间0x00=0s,0x01=0.13s,0x02=0.26s,0x03=0.52s,0x04=1.04s,0x05=2.08s,0x06=4.16s,0x07=8.32s,0x08=16.64s
#define Period_Num  0x00   //LED呼吸次数0x00=无限次,0x01=1次,0x02=2次.....0x0f=15次

#define TST_BIT(flag,bit)	(flag & (0x1 << bit))
#define CLR_BIT(flag,bit)	(flag &= (~(0x1 << bit)))
#define SET_BIT(flag,bit)	(flag |= (0x1 << bit))

#define LED_COLOR_FLAG_BIT		(0x0)
//#define MSG_FLAG_BIT		(0x0)
#define CALL_FLAG_BIT		(0x1)
#define BAT_FLAG_BIT		(0x2)
#define CHARGING_FLAG_BIT	(0x3)
#define CHARGING_FULL_FLAG_BIT	(0x4)

#define TIME_FLAG_BIT		(0x5)

#ifdef HWCTL_DBG
#define DBG_PRINT(x...)	printk(KERN_ERR x)
#else
#define DBG_PRINT(x...)
#endif

/*
static ssize_t aw2013_store_led(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t aw2013_get_reg(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t aw2013_set_reg(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);

static DEVICE_ATTR(led, S_IRUGO | S_IWUSR, NULL, aw2013_store_led);
static DEVICE_ATTR(reg, S_IRUGO | S_IWUGO,aw2013_get_reg,  aw2013_set_reg);
*/
void hwctl_set_blink(uint32_t time);
void aw2013_get_leds_status(struct led_setting *led0,struct led_setting *led1,struct led_setting *led2);
void aw2013_breath_all(struct led_setting led0,struct led_setting led1,struct led_setting led2);
//void aw2013_get_leds_status(int *led0,int *led1,int *led2);
void aw2013_all_leds_control();

void hwctl_led_off();
void hwctl_delay_1us(U16 wTime);
static BOOL hwctl_i2c_write_reg_org(unsigned char reg,unsigned char data);
BOOL hwctl_i2c_write_reg(unsigned char reg,unsigned char data);
unsigned char hwctl_i2c_read_reg(unsigned char regaddr);
int breathlight_master_send(u16 addr, char * buf ,int count);
void led_flash_aw2013_test( unsigned int id );
void led_off_aw2013_test(void);
void led_flash_aw2013( unsigned int id );
void led_flash_aw2013_power_low(void);
void led_flash_aw2013_charging_full(void);
void led_flash_aw2013_charging(void);
void led_flash_aw2013_unanswer_message_incall(void);
void led_flash_aw2013_power_on(void);
void Flush_led_data(void);
void Suspend_led(void);
static long hwctl_unlock_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int hwctl_open(struct inode *inode, struct file *file);
static int hwctl_release(struct inode *inode, struct file *file);
static ssize_t hwctl_read(struct file * fp, char __user * to, size_t read_size, loff_t * pos);
void hwctl_shut_charging_leds_and_dojobs();
static BOOL AW2013_i2c_write_reg(unsigned char reg,unsigned char data);
void AW2013_test(void);
void AW2013_OnOff(BOOL OnOff);  
void AW2013_AllOn(void);
void AW2013_out0_fade(void);
void AW2013_init_pattern(void); 
void AW2013_breath_all(int led0,int led1,int led2); 
void AW2013_Marquee(void);
void AW2013_ComingCall(void);
void AW2013_MissingCall(void);
void AW2013_ComingMsg(void);
void AW2013_MissingMsg (void);
void AW2013_GPIO_LED(void);
void AW2013_init(void);    
static int __devinit AW2013_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int __devexit AW2013_i2c_remove(struct i2c_client *client);
static int __init AW2013_Driver_Init(void); 
static void __exit AW2013_Driver_Exit(void); 

