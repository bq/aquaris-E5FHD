/* Himax Android Driver Sample Code MTK Ver 2.4-M0903
*
* Copyright (C) 2012 Himax Corporation.
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

//=============================================================================================================
// Segment list :
// Include Header file 
// Himax Define Options
// Himax Define Variable
// Himax Include Header file / Data Structure
// Himax Variable/Pre Declation Function
// Himax Normal Function
// Himax SYS Debug Function
// Himax Touch Work Function
// Himax Linux Driver Probe Function - MTK
// Other Function
//============================================================================================================= 

//=============================================================================================================
//
//	Segment : Include Header file 
//
//=============================================================================================================
#include "tpd.h"
#include <linux/interrupt.h>
#include <cust_eint.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#include "cust_gpio_usage.h"
#include <linux/hwmsen_helper.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
// for custom
#include "tpd_custom_hx8527.h"
// for proximity
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <cust_vibrator.h>

//=============================================================================================================
//
//	Segment : Himax Define Options 
//
//=============================================================================================================

extern struct tpd_device *tpd;

//ckt-chunhui.lin
#define FTS_PRESSURE
#define PRESS_MAX	0xFF

#ifdef Himax_Gesture
#define GESTURE_DOUBLE_TAP 	0xFC
#define GESTURE_NONE 		0xFB
#define GESTURE_Q 		0xFA
#define GESTURE_E 		0xF9
#define GESTURE_Z 		0xF8

unsigned char GestureEnable = 1;//wangli_20140729
#endif

extern void custom_vibration_enable(int);

//gionee songll 20131128 porting from mt6577 begin
#ifdef GN_MTK_BSP_DEVICECHECK
#include <linux/gn_device_check.h>
extern int gn_set_device_info(struct gn_device_info gn_dev_info);
#endif
//gionee songll 20131128 porting from mt6577 end

//=============================================================================================================
//
//	Segment : Himax Define Variable 
//
//=============================================================================================================

//------------------------------------------
// Flash dump file
#define FLASH_DUMP_FILE "/sdcard/Flash_Dump.bin"

//------------------------------------------
// Diag Coordinate dump file
#define DIAG_COORDINATE_FILE "/data/Coordinate_Dump.csv"
#define HX_85XX_A_SERIES_PWON		1
#define HX_85XX_B_SERIES_PWON		2
#define HX_85XX_C_SERIES_PWON		3
#define HX_85XX_D_SERIES_PWON		4

#define HX_TP_BIN_CHECKSUM_SW		1
#define HX_TP_BIN_CHECKSUM_HW		2
#define HX_TP_BIN_CHECKSUM_CRC	    3

/********* Start:register miscdevice for user space to upgrade FW *********/
//#define CONFIG_SUPPORT_CTP_UPG	//wangli_20140619

#ifdef CONFIG_SUPPORT_CTP_UPG

#include <linux/device.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

#define TOUCH_IOC_MAGIC 'H'
#define TPD_UPGRADE_CKT _IO(TOUCH_IOC_MAGIC,2)

static unsigned char fw_upgrade_ioctl(unsigned char *i_buf,unsigned int i_length);
static DEFINE_MUTEX(fwupgrade_mutex);
atomic_t upgrading;

static int tpd_misc_open(struct inode *inode,struct file *file)
{
	return nonseekable_open(inode,file);
}

static int tpd_misc_release(struct inode *inode,struct file *file)
{
	return 0;
}

static long tpd_unlocked_ioctl(struct file *file,unsigned int cmd,unsigned long arg)
{
	void __user *data;

	long err = 0;
	int size = 0;
	char * ctpdata = NULL;

	if(_IOC_DIR(cmd) & _IOC_READ)
	{
		err = !access_ok(VERIFY_WRITE,(void __user *)arg,_IOC_SIZE(cmd));
	}
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
	{
		err = !access_ok(VERIFY_READ,(void __user *)arg,_IOC_SIZE(cmd));
	}

	if(err)
	{
		printk("tpd: access error: %08x,(%2d,%2d)\n",cmd,_IOC_DIR(cmd),_IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch(cmd)
	{
		case TPD_UPGRADE_CKT:
			data = (void __user *)arg;
			if(NULL == data)
			{
				err = -EINVAL;
				break;
			}
			if(copy_from_user(&size,data,sizeof(int)))
			{
				err = -EFAULT;
				break;
			}
			ctpdata = kmalloc(size,GFP_KERNEL);
			if( NULL == ctpdata)
			{
				err = -EFAULT;
				break;
			}

			if(copy_from_user(ctpdata,data+sizeof(int),size))
			{
				kfree(ctpdata);
				err = -EFAULT;
				break;
			}
			err = fw_upgrade_ioctl(ctpdata,size);
			kfree(ctpdata);
			break;
		default:
			printk("tpd: unknown IOCTL: %0x08x\n",cmd);
			err = -ENOIOCTLCMD;
			break;		
	}

	return err;
}

static struct file_operations tpd_fops = {
	.open = tpd_misc_open,
	.release = tpd_misc_release,
	.unlocked_ioctl = tpd_unlocked_ioctl,
};

static struct miscdevice tpd_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "hx8527",
	.fops = &tpd_fops,
};
#endif
/********* end: =============== wangli_20140616 ==================*********/

//=============================================================================================================
//
//	Segment : Himax Include Header file / Data Structure
//
//=============================================================================================================

struct touch_info 
{
	int y[10];	// Y coordinate of touch point
	int x[10];	// X coordinate of touch point
	int p[10];	// event flag of touch point
	int id[10];	// touch id of touch point
	int count;	// touch counter
};


#ifdef BUTTON_CHECK
struct t_pos_queue {
    int pos;
    unsigned int timestamp;
};
#endif


//=============================================================================================================
//
//	Segment : Himax Variable/Pre Declation Function
//
//=============================================================================================================
static struct i2c_client *i2c_client    = NULL;
static struct task_struct *touch_thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(waiter);
static DEFINE_MUTEX(i2c_access);

struct workqueue_struct	*himax_wq;
static int tpd_flag  = 0;
static int tpd_halt  = 0;
static int point_num = 0;

static struct kobject *android_touch_kobj = NULL;

uint8_t	IC_STATUS_CHECK	  = 0xAA; // for Hand shaking to check IC status
unsigned char IC_CHECKSUM = 0;
static unsigned char IC_TYPE     = 0;
unsigned char power_ret   = 0;

static int HX_TOUCH_INFO_POINT_CNT = 0;
static int HX_RX_NUM               = 0;
static int HX_TX_NUM               = 0;
static int HX_BT_NUM               = 0;
static int HX_X_RES                = 0;
static int HX_Y_RES                = 0;
static int HX_MAX_PT               = 0;
static bool HX_INT_IS_EDGE         = false;
static int HX_XY_REV               =0;
static unsigned int HX_Gesture     =0;
static bool HX_KEY_HIT_FLAG=false;
static bool point_key_flag=false;
static bool last_point_key_flag=false;

#ifdef Himax_Gesture
//static bool t_t_flag = false;//avoid TP report  twice while double tap
#endif

unsigned char FW_VER_MAJ_FLASH_ADDR;
unsigned char FW_VER_MAJ_FLASH_LENG;
unsigned char FW_VER_MIN_FLASH_ADDR;
unsigned char FW_VER_MIN_FLASH_LENG;
unsigned char CFG_VER_MAJ_FLASH_ADDR;
unsigned char CFG_VER_MAJ_FLASH_LENG;
unsigned char CFG_VER_MIN_FLASH_ADDR;
unsigned char CFG_VER_MIN_FLASH_LENG;
//songll 20131217 begin
//unsigned char CFG_VER_MIN_FLASH_buff[12]={0};//wangli_20140506
//songll 20131217 end

extern kal_bool upmu_is_chr_det(void);
static int himax_charge_switch(s32 dir_update);
static u16 FW_VER_MAJ_buff[1]; // for Firmware Version
static u16 FW_VER_MIN_buff[1];
static u16 CFG_VER_MAJ_buff[12];
static u16 CFG_VER_MIN_buff[12];

static int hx_point_num	= 0; // for himax_ts_work_func use
static int p_point_num	= 0xFFFF;
static int tpd_key      = 0;
static int tpd_key_old  = 0xFF;

#ifdef HX_MTK_DMA
	static uint8_t *gpDMABuf_va = NULL;
	static uint32_t gpDMABuf_pa = NULL;
#endif

//-------------------------------------virtual key
static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;



//------------------------------------ proximity
	#ifdef TPD_PROXIMITY
	static u8 tpd_proximity_flag = 0;
	static u8 tpd_proximity_detect = 1;//0-->close ; 1--> far away
	static int point_proximity_position;
	#define TPD_PROXIMITY_DMESG(a,arg...) printk("TPD_himax8526" ": " a,##arg)
	#define TPD_PROXIMITY_DEBUG(a,arg...) printk("TPD_himax8526" ": " a,##arg)
	#endif


//----[HX_LOADIN_CONFIG]--------------------------------------------------------------------------------start
	#ifdef HX_LOADIN_CONFIG
	unsigned char c1[] 	 =	{ 0x37, 0xFF, 0x08, 0xFF, 0x08};
	unsigned char c2[] 	 =	{ 0x3F, 0x00};
	unsigned char c3[] 	 =	{ 0x62, 0x01, 0x00, 0x01, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c4[] 	 =	{ 0x63, 0x10, 0x00, 0x10, 0x30, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c5[] 	 =	{ 0x64, 0x01, 0x00, 0x01, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c6[] 	 =	{ 0x65, 0x10, 0x00, 0x10, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c7[] 	 =	{ 0x66, 0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c8[] 	 =	{ 0x67, 0x10, 0x00, 0x10, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c9[] 	 =	{ 0x68, 0x01, 0x00, 0x01, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c10[]	 =	{ 0x69, 0x10, 0x00, 0x10, 0x30, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c11[]	 =	{ 0x6A, 0x01, 0x00, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c12[]	 =	{ 0x6B, 0x10, 0x00, 0x10, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c13[]	 =	{ 0x6C, 0x01, 0x00, 0x01, 0x30, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c14[]	 =	{ 0x6D, 0x10, 0x00, 0x10, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c15[]	 =	{ 0x6E, 0x01, 0x00, 0x01, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c16[]	 =	{ 0x6F, 0x10, 0x00, 0x10, 0x20, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c17[]	 =	{ 0x70, 0x01, 0x00, 0x01, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c18[]	 =	{ 0x7B, 0x03};
	unsigned char c19[]	 =	{ 0x7C, 0x00, 0xD8, 0x8C};
	unsigned char c20[]	 =	{ 0x7F, 0x00, 0x04, 0x0A, 0x0A, 0x04, 0x00, 0x00, 0x00};
	unsigned char c21[]	 =	{ 0xA4, 0x94, 0x62, 0x94, 0x86};
	unsigned char c22[]	 =	{ 0xB4, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x0F, 0x04, 0x07, 0x04, 0x07, 0x04, 0x07, 0x00};
	unsigned char c23[]	 =	{ 0xB9, 0x01, 0x36};
	unsigned char c24[]	 =	{ 0xBA, 0x00};
	unsigned char c25[]	 =	{ 0xBB, 0x00};
	unsigned char c26[]	 =	{ 0xBC, 0x00, 0x00, 0x00, 0x00};
	unsigned char c27[]	 =	{ 0xBD, 0x04, 0x0C};
	unsigned char c28[]	 =	{ 0xC2, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c29[]	 =	{ 0xC5, 0x0A, 0x1D, 0x00, 0x10, 0x1A, 0x1E, 0x0B, 0x1D, 0x08, 0x16};
	unsigned char c30[]	 =	{ 0xC6, 0x1A, 0x10, 0x1F};
	unsigned char c31[]	 =	{ 0xC9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x17, 0x17, 0x19, 0x19, 0x1F, 0x1F, 0x1B, 0x1B, 0x1D, 0x1D, 0x21, 0x21, 0x23, 0x23, 
														0x25, 0x25, 0x27, 0x27, 0x29, 0x29, 0x2B, 0x2B, 0x2D, 0x2D, 0x2F, 0x2F, 0x16, 0x16, 0x18, 0x18, 0x1A, 0x1A, 0x20, 0x20, 0x1C, 0x1C, 
														0x1E, 0x1E, 0x22, 0x22, 0x24, 0x24, 0x26, 0x26, 0x28, 0x28, 0x2A, 0x2A, 0x2C, 0x2C, 0x2E, 0x2E, 0x30, 0x30, 0x00, 0x00, 0x00};
	unsigned char c32[] 	= { 0xCB, 0x01, 0xF5, 0xFF, 0xFF, 0x01, 0x00, 0x05, 0x00, 0x9F, 0x00, 0x00, 0x00};
	unsigned char c33[] 	= { 0xD0, 0x06, 0x01};
	unsigned char c34[] 	= { 0xD3, 0x06, 0x01};
	unsigned char c35[] 	= { 0xD5, 0xA5, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	
	unsigned char c36[] 	= { 0x40,0x01, 0x5A
													, 0x77, 0x02, 0xF0, 0x13, 0x00, 0x00
													, 0x56, 0x10, 0x14, 0x18, 0x06, 0x10, 0x0C, 0x0F, 0x0F, 0x0F, 0x52, 0x34, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};//start:0x00 ,size 31
	
	unsigned char c37[] 	= { 0x40, 0xA5, 0x00, 0x80, 0x82, 0x85, 0x00
													, 0x35, 0x25, 0x0F, 0x0F, 0x83, 0x3C, 0x00, 0x00
													, 0x11, 0x11, 0x00, 0x00
													, 0x01, 0x01, 0x00, 0x0A, 0x00, 0x00
													, 0x10, 0x02, 0x10, 0x64, 0x00, 0x00}; // start 0x1E :size 31
	
	unsigned char c38[] 	= {	0x40, 0x40, 0x38, 0x38, 0x02, 0x14, 0x00, 0x00, 0x00
													, 0x04, 0x03, 0x12, 0x06, 0x06, 0x00, 0x00, 0x00}; // start:0x3C ,size 17
	                    	
	unsigned char c39[] 	= {	0x40, 0x18, 0x18, 0x05, 0x00, 0x00, 0xD8, 0x8C, 0x00, 0x00, 0x42, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
													, 0x10, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x0C}; //start 0x4C,size 25
	                    	
	unsigned char c40[] 	= {	0x40, 0x10, 0x12, 0x20, 0x32, 0x01, 0x04, 0x07, 0x09
													, 0xB4, 0x6E, 0x32, 0x00
													, 0x0F, 0x1C, 0xA0, 0x16
													, 0x00, 0x00, 0x04, 0x38, 0x07, 0x80}; //start 0x64,size 23
	
	unsigned char c41[]	 	= {	0x40, 0x03, 0x2F, 0x08, 0x5B, 0x56, 0x2D, 0x05, 0x00, 0x69, 0x02, 0x15, 0x4B, 0x6C, 0x05
													, 0x03, 0xCE, 0x09, 0xFD, 0x58, 0xCC, 0x00, 0x00, 0x7F, 0x02, 0x85, 0x4C, 0xC7, 0x00};//start 0x7A,size 29
	
	unsigned char c42[] 	= {	0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //start 0x96,size 9
	
	unsigned char c43_1[]	= {	0x40, 0x00, 0xFF, 0x15, 0x28, 0x01, 0xFF, 0x16, 0x29, 0x02, 0xFF, 0x1B, 0x2A, 0x03, 0xFF, 0x1C, 0xFF, 0x04, 0xFF, 0x1D, 0xFF, 0x05, 0x0F, 0x1E, 0xFF, 0x06, 0x10, 0x1F, 0xFF, 0x07, 0x11, 0x20}; //start 0x9E,size 32
	unsigned char c43_2[] = {	0x40, 0xFF, 0x08, 0x12, 0x21, 0xFF, 0x09, 0x13, 0x22, 0xFF, 0x0A, 0x14, 0x23, 0xFF, 0x0B, 0x17, 0x24, 0xFF, 0x0C, 0x18, 0x25, 0xFF, 0x0D, 0x19, 0x26, 0xFF, 0x0E, 0x1A, 0x27, 0xFF}; //start 0xBD,size 29
	
	unsigned char c44_1[] = {	0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //start 0xDA,size 32
	unsigned char c44_2[] = {	0x40, 0x00, 0x00, 0x00, 0x00, 0x00}; //0xF9 size 6
	unsigned char c45[] 	= {	0x40, 0x1D, 0x00}; //start 0xFE,size 3
	#endif
//----[HX_LOADIN_CONFIG]----------------------------------------------------------------------------------end

extern void mt_eint_unmask(unsigned int line);
extern void mt_eint_mask(unsigned int line);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);
//extern void mt65_eint_registration(unsigned int eint_num, unsigned int is_deb_en, unsigned int pol, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);

static void tpd_eint_interrupt_handler(void);
static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int __devexit tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);

static int himax_ts_poweron(void);
static int himax_hang_shaking(void);
static bool himax_ic_package_check(void);
static int himax_touch_sysfs_init(void);
static void himax_touch_sysfs_deinit(void);
int himax_ManualMode(int enter);
int himax_FlashMode(int enter);
static int himax_lock_flash(void);
static int himax_unlock_flash(void);
static uint8_t himax_calculateChecksum(char *ImageBuffer, int fullLength);
static int himax_read_flash(unsigned char *buf, unsigned int addr_start, unsigned int length);
void himax_touch_information(void);

bool himax_debug_flag=false;

#ifdef HX_RST_PIN_FUNC
	void himax_HW_reset(void);
#endif

//----[HX_TP_SYS_DEBUG_LEVEL]---------------------------------------------------------------------------start
#ifdef HX_TP_SYS_DEBUG_LEVEL
	static uint8_t debug_log_level       = 0;
	static bool fw_update_complete       = false;
	static bool irq_enable               = false;
	static int handshaking_result        = 0;
	static unsigned char debug_level_cmd = 0;
	static unsigned char upgrade_fw[32*1024];
	
	static uint8_t getDebugLevel(void);
	int fts_ctpm_fw_upgrade_with_sys_fs(unsigned char *fw, int len);
#endif
//----[HX_TP_SYS_DEBUG_LEVEL]-----------------------------------------------------------------------------end

//----[HX_TP_SYS_DIAG]----------------------------------------------------------------------------------start
#ifdef HX_TP_SYS_DIAG
	static uint8_t x_channel    = 15;//wangli_20140507
	static uint8_t y_channel    = 29;//wangli_20140507
	static uint8_t *diag_mutual = NULL;
	static uint8_t diag_command = 0;
	static uint8_t diag_coor[128];// = {0xFF};
	static uint8_t diag_self[100] = {0};
	
	static uint8_t *getMutualBuffer(void);
	static uint8_t *getSelfBuffer(void);
	static uint8_t 	getDiagCommand(void);
	static uint8_t 	getXChannel(void);
	static uint8_t 	getYChannel(void);
	
	static void setMutualBuffer(void);
	static void setXChannel(uint8_t x);
	static void setYChannel(uint8_t y);
	
	static uint8_t coordinate_dump_enable = 0;
	struct file *coordinate_fn;
#endif
//----[HX_TP_SYS_DIAG]------------------------------------------------------------------------------------end

//----[HX_TP_SYS_REGISTER]------------------------------------------------------------------------------start
#ifdef HX_TP_SYS_REGISTER
	static uint8_t register_command       = 0;
	static uint8_t multi_register_command = 0;
	static uint8_t multi_register[8]      = {0x00};
	static uint8_t multi_cfg_bank[8]      = {0x00};
	static uint8_t multi_value[1024]      = {0x00};
	static bool config_bank_reg           = false;
#endif
//----[HX_TP_SYS_REGISTER]--------------------------------------------------------------------------------end

//----[HX_TP_SYS_FLASH_DUMP]--------------------------------------------------------------------------start
#ifdef HX_TP_SYS_FLASH_DUMP
	struct workqueue_struct *flash_wq;
	struct work_struct flash_work;

	static uint8_t *flash_buffer       = NULL;
	static uint8_t flash_command       = 0;
	static uint8_t flash_read_step     = 0;
	static uint8_t flash_progress      = 0;
	static uint8_t flash_dump_complete = 0;
	static uint8_t flash_dump_fail     = 0;
	static uint8_t sys_operation       = 0;
	static uint8_t flash_dump_sector   = 0;
	static uint8_t flash_dump_page     = 0;
	static bool flash_dump_going       = false;

	static uint8_t getFlashCommand(void);
	static uint8_t getFlashDumpComplete(void);
	static uint8_t getFlashDumpFail(void);
	static uint8_t getFlashDumpProgress(void);
	static uint8_t getFlashReadStep(void);
	static uint8_t getSysOperation(void);
	static uint8_t getFlashDumpSector(void);
	static uint8_t getFlashDumpPage(void);
	static bool getFlashDumpGoing(void);

	static void setFlashBuffer(void);
	static void setFlashCommand(uint8_t command);
	static void setFlashReadStep(uint8_t step);
	static void setFlashDumpComplete(uint8_t complete);
	static void setFlashDumpFail(uint8_t fail);
	static void setFlashDumpProgress(uint8_t progress);
	static void setSysOperation(uint8_t operation);
	static void setFlashDumpSector(uint8_t sector);
	static void setFlashDumpPage(uint8_t page);
	static void setFlashDumpGoing(bool going);
#endif
//----[HX_TP_SYS_FLASH_DUMP]----------------------------------------------------------------------------end

//----[HX_TP_SYS_SELF_TEST]-----------------------------------------------------------------------------start
#ifdef HX_TP_SYS_SELF_TEST 
	static ssize_t himax_chip_self_test_function(struct device *dev, struct device_attribute *attr, char *buf);
	static int himax_chip_self_test(void);
#endif
//----[HX_TP_SYS_SELF_TEST]-------------------------------------------------------------------------------end

//----[HX_FW_UPDATE_BY_I_FILE]--------------------------------------------------------------------------start
#ifdef HX_FW_UPDATE_BY_I_FILE

   
	static bool i_Needupdate = true;
	static unsigned char i_isTP_Updated = 0;
	static int fw_size=0;
	static unsigned char i_CTPM_FW[]=
	{
		#include "FW_CKT-BQ6_Truly_CT3S1403_173D_C13_2014-08-19_E.i" //Paul Check //wangli_20140819
	};
	
	
#endif
//----[HX_FW_UPDATE_BY_I_FILE]----------------------------------------------------------------------------end

//----[HX_ESD_WORKAROUND]-------------------------------------------------------------------------------start
#ifdef HX_ESD_WORKAROUND
	static u8 ESD_RESET_ACTIVATE = 1;
	static u8 ESD_COUNTER = 0;
	static int ESD_COUNTER_SETTING = 3;
	unsigned char TOUCH_UP_COUNTER = 0;
	void ESD_HW_REST(void);
#endif 
//----[HX_ESD_WORKAROUND]---------------------------------------------------------------------------------end

//=============================================================================================================
//
//	Segment : Himax Normal Function
//
//=============================================================================================================	
#ifdef PT_NUM_LOG
#define PT_ARRAY_SZ (200)
static int point_cnt_array[PT_ARRAY_SZ];
static int curr_ptr = 0;
#endif


#ifdef BUTTON_CHECK
static int bt_cnt = 0;
static int bt_confirm_cnt=40; /* successive n frame in button, it report a button event*/
static int obs_intvl = 200;	//1000;  //unit : sec


#define POS_QUEUE_LEN   (8)
#define INVALID_POS        (0xffff)
#define LEAVE_POS      (0xfffe)
//static int pos_queue[POS_QUEUE_LEN]={INVALID_POS};
static struct t_pos_queue pos_queue[POS_QUEUE_LEN];
static int p_latest=1;
static int p_prev = 0;
static bool is_himax = true;


int pointFromAA(void)
{
    int i;
    int point_cnt = 0;
    int pt = p_prev;
    
    for(i=0;i<POS_QUEUE_LEN;i++)
    {        
        pt = (p_prev+POS_QUEUE_LEN-i)%POS_QUEUE_LEN;

        if (unlikely(himax_debug_flag))
        {
            printk(KERN_INFO "[himax] pos_queue check,p_prev = [%d], pt = [%d], pos =[%d] , timestamp=[%u]\n ", 
                p_prev,pt,pos_queue[pt].pos,pos_queue[pt].timestamp);
        }       
        
        if(pos_queue[pt].pos == INVALID_POS)   //invalid entry
            continue;

        if(jiffies - pos_queue[pt].timestamp >= (obs_intvl*HZ)/1000) //old entry , dont check
            continue;

        if(pos_queue[pt].pos > 1000 && pos_queue[pt].pos < 1280)
            point_cnt +=1;         
    }

    if (unlikely(himax_debug_flag))
    {
        printk(KERN_INFO "[himax] pointFromAA, point_cnt = [%d]\n ", 
            point_cnt);
    }   

    if(point_cnt > 0)
        return 1;
    else
        return 0;
}

#endif

	
static int i2c_himax_write(struct i2c_client *client, uint8_t command,uint8_t length, uint8_t *data)
{
	int retry, loop_i;
	int toRetry=1;
	uint8_t *buf = kzalloc(sizeof(uint8_t)*(length+1), GFP_KERNEL);

	struct i2c_msg msg[] = 
	{
		{
			.addr = client->addr,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};

	buf[0] = command;
	for (loop_i = 0; loop_i < length; loop_i++)
	{
		buf[loop_i + 1] = data[loop_i];
	}
	for (retry = 0; retry < toRetry; retry++) 
	{
		if (i2c_transfer(client->adapter, msg, 1) == 1)
		{
			break;
		}
		msleep(10);
	}

	if (retry == toRetry) 
	{
		printk(KERN_ERR "[TP] %s: i2c_write_block retry over %d\n", __func__, toRetry);
		kfree(buf);
		return -EIO;
	}
	kfree(buf);
	return 0;
}


static int i2c_himax_read(struct i2c_client *client, uint8_t command, uint8_t length,uint8_t *data)
{
	int retry;
	int toRetry=1;
	struct i2c_msg msg[] = 
	{
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &command,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		}
	};

	for (retry = 0; retry < toRetry; retry++) 
	{
		if (i2c_transfer(client->adapter, msg, 2) == 2)
		{
			break;
		}
		msleep(10);
	}
	if (retry == toRetry) 
	{
		printk(KERN_INFO "[TP] %s: i2c_read_block retry over %d\n", __func__, toRetry);
		return -EIO;
	}
	return 0;
}


static int himax_i2c_read_data(struct i2c_client *client, uint8_t command,uint8_t len,uint8_t *buf)
{
#ifdef HX_MTK_DMA
	int rc, i;
	unsigned short addr = 0;
	uint8_t *pReadData = 0;
	pReadData = gpDMABuf_va;
	addr = client->addr ;

	client->addr = client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG | I2C_ENEXT_FLAG;
	client->timing = 400;

	if(!pReadData)
	{
		printk("[Himax] dma_alloc_coherent failed!\n");
		return -1;
	}

	gpDMABuf_va[0] = command;
	rc = i2c_master_send(client, gpDMABuf_pa, 1);
	if (rc < 0) 
	{
		printk("[Himax] hx8526_i2c_dma_recv_data sendcomand failed!\n");
	}
	rc = i2c_master_recv(client, gpDMABuf_pa, len);
	client->addr = addr;

	for(i=0;i<len;i++)
	{
		buf[i] = gpDMABuf_va[i];
	}
	return rc;
#else
	return i2c_smbus_read_i2c_block_data(i2c_client, command, len, buf);
	 //return i2c_himax_read(i2c_client,command,len,buf);
#endif
}

static int himax_i2c_write_data(struct i2c_client *client, uint8_t command,uint8_t len,uint8_t *buf)
{
#ifdef HX_MTK_DMA
	int rc,i;
	unsigned short addr = 0;
	uint8_t *pWriteData = gpDMABuf_va;
	addr = client->addr ;

	client->addr = client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG | I2C_ENEXT_FLAG;
	client->timing = 400;

	//printk("[Himax] hx8526_i2c_dma_send_data len = %d\n",len);
	
	if(!pWriteData)
	{
		printk("[Himax] dma_alloc_coherent failed!\n");
		return -1;
	}

	gpDMABuf_va[0] = command;

	for(i=0;i<len;i++)
	{
		gpDMABuf_va[i+1] = buf[i];
	}

	rc = i2c_master_send(client, gpDMABuf_pa, len+1);
	client->addr = addr;
	//printk("[Himax] hx8526_i2c_dma_send_data rc=%d!\n",rc);
	return rc;
#else
	return i2c_smbus_write_i2c_block_data(i2c_client, command, len, buf);
	  //return i2c_himax_write(i2c_client,command,len,buf);
#endif
}

int himax_ManualMode(int enter)
{
	uint8_t cmd[2];
	cmd[0] = enter;

	if( himax_i2c_write_data(i2c_client, 0x42, 1, &(cmd[0])) < 0 )
	{
		printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		return -1;
	}
	return 1;
}

int himax_FlashMode(int enter)
{
	uint8_t cmd[2];
	cmd[0] = enter;

	if( himax_i2c_write_data(i2c_client, 0x43, 1, &(cmd[0])) < 0 )
	{
		printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		return -1;
	}
	return 1;
}

static int himax_lock_flash(void)
{
	uint8_t cmd[5];

	/* lock sequence start */
	cmd[0] = 0x01;cmd[1] = 0x00;cmd[2] = 0x06;
	if( himax_i2c_write_data(i2c_client, 0x43, 3, &(cmd[0])) < 0 )
	{
		printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		return -1;
	}

	cmd[0] = 0x03;cmd[1] = 0x00;cmd[2] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
	{
		printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		return -1;
	}

	cmd[0] = 0x00;cmd[1] = 0x00;cmd[2] = 0x7D;cmd[3] = 0x03;
	if( himax_i2c_write_data(i2c_client, 0x45, 4, &(cmd[0])) < 0 )
	{
		printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		return -1;
	}

	if( himax_i2c_write_data(i2c_client, 0x4A, 0, &(cmd[0])) < 0 )
	{
		printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		return -1;
	}
	mdelay(50);
	return 1;
	/* lock sequence stop */
}

static int himax_unlock_flash(void)
{
	uint8_t cmd[5];

	/* unlock sequence start */
	cmd[0] = 0x01;cmd[1] = 0x00;cmd[2] = 0x06;
	if( himax_i2c_write_data(i2c_client, 0x43, 3, &(cmd[0])) < 0 )
	{
		printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		return -1;
	}

	cmd[0] = 0x03;cmd[1] = 0x00;cmd[2] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
	{
		printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		return -1;
	}

	cmd[0] = 0x00;cmd[1] = 0x00;cmd[2] = 0x3D;cmd[3] = 0x03;
	if( himax_i2c_write_data(i2c_client, 0x45, 4, &(cmd[0])) < 0 )
	{
		printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		return -1;
	}

	if( himax_i2c_write_data(i2c_client, 0x4A, 0, &(cmd[0])) < 0 )
	{
		printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		return -1;
	}
	mdelay(50);
	
	return 1;
	/* unlock sequence stop */
}

static bool himax_ic_package_check(void)
{
	uint8_t cmd[3];
	uint8_t data[3];

	himax_i2c_read_data(i2c_client, 0xD1, 3, &(cmd[0]));
	himax_i2c_read_data(i2c_client, 0x31, 3, &(data[0]));
	
	printk("\nREAD 0x31 data is %x %x",data[0],data[1]);//wangli
	printk("\nREAD 0xD1 cmd is %x %x %x\n",cmd[0],cmd[1],cmd[2]);//wangli
	if((data[0] == 0x85 && data[1] == 0x28) || (cmd[0] == 0x04 && cmd[1] == 0x85 && (cmd[2] == 0x26 || cmd[2] == 0x27 || cmd[2] == 0x28)))
	{
		tpd_load_status = 1;
		IC_TYPE	= HX_85XX_D_SERIES_PWON;
		IC_CHECKSUM = HX_TP_BIN_CHECKSUM_CRC;
		//Himax: Set FW and CFG Flash Address
		FW_VER_MAJ_FLASH_ADDR = 133; //0x0085
		FW_VER_MAJ_FLASH_LENG = 1;
		FW_VER_MIN_FLASH_ADDR = 134; //0x0086
		FW_VER_MIN_FLASH_LENG = 1;
		CFG_VER_MAJ_FLASH_ADDR = 160; //0x00A0
		CFG_VER_MAJ_FLASH_LENG = 12;
		CFG_VER_MIN_FLASH_ADDR = 172; //0x00AC
		CFG_VER_MIN_FLASH_LENG = 12;

		printk("[Himax] IC package 8528 D\n");
	}
	else if((data[0] == 0x85 && data[1] == 0x23) || (cmd[0] == 0x03 && cmd[1] == 0x85 && (cmd[2] == 0x26 || cmd[2] == 0x27 || cmd[2] == 0x28 || cmd[2] == 0x29)))
	{
		tpd_load_status = 1;
		IC_TYPE = HX_85XX_C_SERIES_PWON;  
		IC_CHECKSUM = HX_TP_BIN_CHECKSUM_SW;
		//Himax: Set FW and CFG Flash Address
		FW_VER_MAJ_FLASH_ADDR = 133; //0x0085
		FW_VER_MAJ_FLASH_LENG = 1;
		FW_VER_MIN_FLASH_ADDR = 134; //0x0086
		FW_VER_MIN_FLASH_LENG = 1;
		CFG_VER_MAJ_FLASH_ADDR = 135; //0x0087
		CFG_VER_MAJ_FLASH_LENG = 12;
		CFG_VER_MIN_FLASH_ADDR = 147; //0x0093
		CFG_VER_MIN_FLASH_LENG = 12;

		printk("[Himax] IC package 8523 C\n");
	}
	else if ((data[0] == 0x85 && data[1] == 0x26) || (cmd[0] == 0x02 && cmd[1] == 0x85 && (cmd[2] == 0x19 || cmd[2] == 0x25 || cmd[2] == 0x26)))
	{
		tpd_load_status = 1;
		IC_TYPE = HX_85XX_B_SERIES_PWON;
		IC_CHECKSUM = HX_TP_BIN_CHECKSUM_SW;
		//Himax: Set FW and CFG Flash Address
		FW_VER_MAJ_FLASH_ADDR = 133; //0x0085
		FW_VER_MAJ_FLASH_LENG = 1;
		FW_VER_MIN_FLASH_ADDR = 728; //0x02D8
		FW_VER_MIN_FLASH_LENG = 1;
		CFG_VER_MAJ_FLASH_ADDR = 692; //0x02B4
		CFG_VER_MAJ_FLASH_LENG = 3;
		CFG_VER_MIN_FLASH_ADDR = 704; //0x02C0
		CFG_VER_MIN_FLASH_LENG = 3;
		
		printk("[Himax] IC package 8526 B\n");
	}
	else if ((data[0] == 0x85 && data[1] == 0x20) || (cmd[0] == 0x01 && cmd[1] == 0x85 && cmd[2] == 0x19))
	{	
		tpd_load_status = 1;
		IC_TYPE = HX_85XX_A_SERIES_PWON;
		IC_CHECKSUM = HX_TP_BIN_CHECKSUM_SW;
		printk("[Himax] IC package 8520 A\n");
	}
	else
	{
		tpd_load_status = 0;
		printk("[Himax] IC package incorrect!!\n");
		printk("\nHERE ARE THE ERROR!!!");//wangli
		return false;
	}



	return true;
}

u8 himax_read_FW_ver(void)
{
	u16 fw_ver_maj_start_addr;
	u16 fw_ver_maj_end_addr;
	u16 fw_ver_maj_addr;
	u16 fw_ver_maj_length;

	u16 fw_ver_min_start_addr;
	u16 fw_ver_min_end_addr;
	u16 fw_ver_min_addr;
	u16 fw_ver_min_length;

	u16 cfg_ver_maj_start_addr;
	u16 cfg_ver_maj_end_addr;
	u16 cfg_ver_maj_addr;
	u16 cfg_ver_maj_length;

	u16 cfg_ver_min_start_addr;
	u16 cfg_ver_min_end_addr;
	u16 cfg_ver_min_addr;
	u16 cfg_ver_min_length;

	uint8_t cmd[3];
	u16 i = 0;
	u16 j = 0;
	u16 k = 0;

	fw_ver_maj_start_addr 	= FW_VER_MAJ_FLASH_ADDR / 4;                            // start addr = 133 / 4 = 33 
	fw_ver_maj_length       = FW_VER_MAJ_FLASH_LENG;                                // length = 1
	fw_ver_maj_end_addr     = (FW_VER_MAJ_FLASH_ADDR + fw_ver_maj_length ) / 4 + 1;	// end addr = 134 / 4 = 33
	fw_ver_maj_addr         = FW_VER_MAJ_FLASH_ADDR % 4;                            // 133 mod 4 = 1

	fw_ver_min_start_addr   = FW_VER_MIN_FLASH_ADDR / 4;                            // start addr = 134 / 4 = 33
	fw_ver_min_length       = FW_VER_MIN_FLASH_LENG;                                // length = 1
	fw_ver_min_end_addr     = (FW_VER_MIN_FLASH_ADDR + fw_ver_min_length ) / 4 + 1;	// end addr = 135 / 4 = 33
	fw_ver_min_addr         = FW_VER_MIN_FLASH_ADDR % 4;                            // 134 mod 4 = 2

	cfg_ver_maj_start_addr  = CFG_VER_MAJ_FLASH_ADDR / 4;                             // start addr = 160 / 4 = 40
	cfg_ver_maj_length      = CFG_VER_MAJ_FLASH_LENG;                                 // length = 12
	cfg_ver_maj_end_addr    = (CFG_VER_MAJ_FLASH_ADDR + cfg_ver_maj_length ) / 4 + 1; // end addr = (160 + 12) / 4 = 43
	cfg_ver_maj_addr        = CFG_VER_MAJ_FLASH_ADDR % 4;                             // 160 mod 4 = 0

	cfg_ver_min_start_addr  = CFG_VER_MIN_FLASH_ADDR / 4;                             // start addr = 172 / 4 = 43
	cfg_ver_min_length      = CFG_VER_MIN_FLASH_LENG;                                 // length = 12
	cfg_ver_min_end_addr    = (CFG_VER_MIN_FLASH_ADDR + cfg_ver_min_length ) / 4 + 1; // end addr = (172 + 12) / 4 = 46
	cfg_ver_min_addr        = CFG_VER_MIN_FLASH_ADDR % 4;                             // 172 mod 4 = 0

	mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);

#ifdef HX_RST_PIN_FUNC
	himax_HW_reset();
#endif

	//Sleep out

	if( himax_i2c_write_data(i2c_client, 0x81, 0, &(cmd[0])) < 0)
	{
		printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		return -1;
	}
	mdelay(120);

	//Enter flash mode
	himax_FlashMode(1);

	//Read Flash Start
	//FW Version MAJ
	i = fw_ver_maj_start_addr;
	do
	{
		cmd[0] = i & 0x1F;         //column = 33 mod 32 = 1
		cmd[1] = (i >> 5) & 0x1F;  //page = 33 / 32 = 1
		cmd[2] = (i >> 10) & 0x1F; //sector = 33 / 1024 = 0

		if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		if(i == fw_ver_maj_start_addr) //first page
		{
			j = 0;
			for( k = fw_ver_maj_addr; k < 4 && j < fw_ver_maj_length; k++)
			{
				FW_VER_MAJ_buff[j++] = cmd[k];
			}
		}
		else //other page
		{
			for( k = 0; k < 4 && j < fw_ver_maj_length; k++)
			{
				FW_VER_MAJ_buff[j++] = cmd[k];
			}
		}
		i++;
	}
	while(i < fw_ver_maj_end_addr);

	//FW Version MIN
	i = fw_ver_min_start_addr;
	do
	{
		cmd[0] = i & 0x1F;         //column = 33 mod 32 = 1
		cmd[1] = (i >> 5) & 0x1F;  //page = 33 / 32 = 1
		cmd[2] = (i >> 10) & 0x1F; //sector = 33 / 1024	= 0

		if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}


		if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		if(i == fw_ver_min_start_addr) //first page
		{
			j = 0;
			for(k = fw_ver_min_addr; k < 4 && j < fw_ver_min_length; k++)
			{
				FW_VER_MIN_buff[j++] = cmd[k];
			}
		}
		else //other page
		{
			for(k = 0; k < 4 && j < fw_ver_min_length; k++)
			{
				FW_VER_MIN_buff[j++] = cmd[k];
			}
		}
		i++;
	}while(i < fw_ver_min_end_addr);


	//CFG Version MAJ
	i = cfg_ver_maj_start_addr;
	do
	{
		cmd[0] = i & 0x1F;         //column = 40 mod 32 = 8
		cmd[1] = (i >> 5) & 0x1F;  //page = 40 / 32 = 1
		cmd[2] = (i >> 10) & 0x1F; //sector = 40 / 1024 = 0

		if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		if(i == cfg_ver_maj_start_addr) //first page
		{
			j = 0;
			for( k = cfg_ver_maj_addr; k < 4 && j < cfg_ver_maj_length; k++)
			{
				CFG_VER_MAJ_buff[j++] = cmd[k];
			}
		}
		else //other page
		{
			for(k = 0; k < 4 && j < cfg_ver_maj_length; k++)
			{
				CFG_VER_MAJ_buff[j++] = cmd[k];
			}
		}
		i++;
	}
	while(i < cfg_ver_maj_end_addr);

	//CFG Version MIN
	i = cfg_ver_min_start_addr;
	do
	{
		cmd[0] = i & 0x1F;         //column = 43 mod 32 = 11
		cmd[1] = (i >> 5) & 0x1F;  //page = 43 / 32 = 1
		cmd[2] = (i >> 10) & 0x1F; //sector = 43 / 1024	= 0

		if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

	
		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}
		if(i == cfg_ver_min_start_addr) //first page
		{
			j = 0;
			for(k = cfg_ver_min_addr; k < 4 && j < cfg_ver_min_length; k++)
			{
				CFG_VER_MIN_buff[j++] = cmd[k];
			}
		}
		else //other page
		{
			for(k = 0; k < 4 && j < cfg_ver_min_length; k++)
			{
				CFG_VER_MIN_buff[j++] = cmd[k];
			}
		}
		i++;
	}
	while(i < cfg_ver_min_end_addr);

	//Exit flash mode
	himax_FlashMode(0);

	/**************************************************
	Check FW Version , TBD
	FW Major version : FW_VER_MAJ_buff
	FW Minor version : FW_VER_MIN_buff
	CFG Major version : CFG_VER_MAJ_buff
	CFG Minor version : CFG_VER_MIN_buff

	return 0 :
	return 1 :
	return 2 :

	**************************************************/

	printk("FW_VER_MAJ_buff : %d \n",FW_VER_MAJ_buff[0]);
	printk("FW_VER_MIN_buff : %d \n",FW_VER_MIN_buff[0]);

	printk("CFG_VER_MAJ_buff : ");
	for(i=0; i<12; i++)
	{
		printk(" %d ,",CFG_VER_MAJ_buff[i]);
	}
	printk("\n");

	printk("CFG_VER_MIN_buff : ");
	for(i=0; i<12; i++)
	{
		printk(" %d ,",CFG_VER_MIN_buff[i]);
	}
	printk("\n");

	himax_ts_poweron();//wangli_20140522

	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	return 0;
}

//----[ HX_LOADIN_CONFIG ]----------------------------------------------------------------------------start
#ifdef HX_LOADIN_CONFIG
static int himax_config_flow()
{
	char data[4];
	uint8_t buf0[4];
	int i2c_CheckSum = 0;
	unsigned long i = 0;
			
	data[0] = 0xE3;
	data[1] = 0x00;	//reload disable

	if( himax_i2c_write_data(i2c_client, &data[0], 1, &(data[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c1[0], sizeof(c1)-1, &(c1[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c2[0], sizeof(c2)-1, &(c2[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c3[0], sizeof(c3)-1, &(c3[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c4[0], sizeof(c4)-1, &(c4[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c5[0], sizeof(c5)-1, &(c5[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c6[0], sizeof(c6)-1, &(c6[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c7[0], sizeof(c7)-1, &(c7[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c8[0], sizeof(c8)-1, &(c8[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c9[0], sizeof(c9)-1, &(c9[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c10[0], sizeof(c10)-1, &(c10[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c11[0], sizeof(c11)-1, &(c11[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c12[0], sizeof(c12)-1, &(c12[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c13[0], sizeof(c13)-1, &(c13[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c14[0], sizeof(c14)-1, &(c14[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c15[0], sizeof(c15)-1, &(c15[1])) < 0 )
	{
		goto HimaxErr;
	}


	if( himax_i2c_write_data(i2c_client, c16[0], sizeof(c16)-1, &(c16[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c17[0], sizeof(c17)-1, &(c17[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c18[0], sizeof(c18)-1, &(c18[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c19[0], sizeof(c19)-1, &(c19[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c20[0], sizeof(c20)-1, &(c20[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c21[0], sizeof(c21)-1, &(c21[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c22[0], sizeof(c22)-1, &(c22[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c23[0], sizeof(c23)-1, &(c23[1])) < 0 )
	{	
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c24[0], sizeof(c24)-1, &(c24[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c25[0], sizeof(c25)-1, &(c25[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c26[0], sizeof(c26)-1, &(c26[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c27[0], sizeof(c27)-1, &(c27[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c28[0], sizeof(c28)-1, &(c28[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c29[0], sizeof(c29)-1, &(c29[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c30[0], sizeof(c30)-1, &(c30[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c31[0], sizeof(c31)-1, &(c31[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c32[0], sizeof(c32)-1, &(c32[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c33[0], sizeof(c33)-1, &(c33[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c34[0], sizeof(c34)-1, &(c34[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c35[0], sizeof(c35)-1, &(c35[1])) < 0 )

	{
		goto HimaxErr;
	}


	//i2c check sum start.
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xAB, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	data[0] = 0x01;
	if( himax_i2c_write_data(i2c_client, 0xAB, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	//------------------------------------------------------------------config bank PART c36 START
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
			
	data[0] = 0x00;
	data[1] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}			
			
	if( himax_i2c_write_data(i2c_client, c36[0], sizeof(c36)-1, &(c36[1])) < 0 )
	{	
		goto HimaxErr;
	}
	
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	for( i=0 ; i<sizeof(c36) ; i++ )
	{
		i2c_CheckSum += c36[i];
	}
	printk("Himax i2c_checksum_36_size = %d \n",sizeof(c36));
	printk("Himax i2c_checksum_36 = %d \n",i2c_CheckSum);
	
	i2c_CheckSum += 0x2AF;
	
	printk("Himax i2c_checksum_36 = %d \n",i2c_CheckSum);
	//------------------------------------------------------------------config bank PART c36 END
	
	
	//------------------------------------------------------------------config bank PART c37 START
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
			
	data[0] = 0x00;
	data[1] = 0x1E;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}			
			
	if( himax_i2c_write_data(i2c_client, c37[0], sizeof(c37)-1, &(c37[1])) < 0 )
	{	
		goto HimaxErr;
	}
	
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	for( i=0 ; i<sizeof(c37) ; i++ )
	{
		i2c_CheckSum += c37[i];
	}
	printk("Himax i2c_checksum_37_size = %d \n",sizeof(c37));
	printk("Himax i2c_checksum_37 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x2CD;
	printk("Himax i2c_checksum_37 = %d \n",i2c_CheckSum);
	//------------------------------------------------------------------config bank PART c37 END
	
	//------------------------------------------------------------------config bank PART c38 START
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
			
	data[0] = 0x00;
	data[1] = 0x3C;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}			
			
	if( himax_i2c_write_data(i2c_client, c38[0], sizeof(c38)-1, &(c38[1])) < 0 )
	{	
		goto HimaxErr;
	}
	
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	for( i=0 ; i<sizeof(c38) ; i++ )
	{
		i2c_CheckSum += c38[i];
	}
	printk("Himax i2c_checksum_38_size = %d \n",sizeof(c38));
	printk("Himax i2c_checksum_38 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x2EB;
	printk("Himax i2c_checksum_38 = %d \n",i2c_CheckSum);
	//------------------------------------------------------------------config bank PART c38 END
	
	//------------------------------------------------------------------config bank PART c39 START
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
			
	data[0] = 0x00;
	data[1] = 0x4C;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}			
			
	if( himax_i2c_write_data(i2c_client, c39[0], sizeof(c39)-1, &(c39[1])) < 0 )
	{	
		goto HimaxErr;
	}
	
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	for( i=0 ; i<sizeof(c39) ; i++ )
	{
		i2c_CheckSum += c39[i];
	}
	printk("Himax i2c_checksum_39_size = %d \n",sizeof(c39));
	printk("Himax i2c_checksum_39 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x2FB;	
	printk("Himax i2c_checksum_39 = %d \n",i2c_CheckSum);
	//------------------------------------------------------------------config bank PART c39 END
	
	//------------------------------------------------------------------config bank PART c40 START
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
			
	data[0] = 0x00;
	data[1] = 0x64;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}			
			
	if( himax_i2c_write_data(i2c_client, c40[0], sizeof(c40)-1, &(c40[1])) < 0 )
	{	
		goto HimaxErr;
	}
	
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	for( i=0 ; i<sizeof(c40) ; i++ )
	{
		i2c_CheckSum += c40[i];
	}
	printk("Himax i2c_checksum_40_size = %d \n",sizeof(c40));
	printk("Himax i2c_checksum_40 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x313;
	printk("Himax i2c_checksum_40 = %d \n",i2c_CheckSum);
	//------------------------------------------------------------------config bank PART c40 END
	
	//------------------------------------------------------------------config bank PART c41 START
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
			
	data[0] = 0x00;
	data[1] = 0x7A;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}			
			
	if( himax_i2c_write_data(i2c_client, c41[0], sizeof(c41)-1, &(c41[1])) < 0 )
	{	
		goto HimaxErr;
	}
	
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	for( i=0 ; i<sizeof(c41) ; i++ )
	{
		i2c_CheckSum += c41[i];
	}
	printk("Himax i2c_checksum_41_size = %d \n",sizeof(c41));
	printk("Himax i2c_checksum_41 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x329;
	printk("Himax i2c_checksum_41 = %d \n",i2c_CheckSum);
	//------------------------------------------------------------------config bank PART c41 END	
	
	//------------------------------------------------------------------config bank PART c42 START
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
			
	data[0] = 0x00;
	data[1] = 0x96;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}			
			
	if( himax_i2c_write_data(i2c_client, c42[0], sizeof(c42)-1, &(c42[1])) < 0 )
	{	
		goto HimaxErr;
	}
	
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	for( i=0 ; i<sizeof(c42) ; i++ )
	{
		i2c_CheckSum += c42[i];
	}
	printk("Himax i2c_checksum_42_size = %d \n",sizeof(c42));
	printk("Himax i2c_checksum_42 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x345;
	printk("Himax i2c_checksum_42 = %d \n",i2c_CheckSum);
	//------------------------------------------------------------------config bank PART c42 END
	
	//------------------------------------------------------------------config bank PART c43_1 START
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
			
	data[0] = 0x00;
	data[1] = 0x9E;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}			
			
	if( himax_i2c_write_data(i2c_client, c43_1[0], sizeof(c43_1)-1, &(c43_1[1])) < 0 )
	{	
		goto HimaxErr;
	}
	
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	for( i=0 ; i<sizeof(c43_1) ; i++ )
	{
		i2c_CheckSum += c43_1[i];
	}
	printk("Himax i2c_checksum_43_1_size = %d \n",sizeof(c43_1));
	printk("Himax i2c_checksum_43_1 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x34D;
	printk("Himax i2c_checksum_43_1 = %d \n",i2c_CheckSum);
	//------------------------------------------------------------------config bank PART c43_1 END
	
	//------------------------------------------------------------------config bank PART c43_2 START
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
			
	data[0] = 0x00;
	data[1] = 0xBD;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}			
			
	if( himax_i2c_write_data(i2c_client, c43_2[0], sizeof(c43_2)-1, &(c43_2[0])) < 0 )
	{	
		goto HimaxErr;
	}
	
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	for( i=0 ; i<sizeof(c43_2) ; i++ )
	{
		i2c_CheckSum += c43_2[i];
	}
	printk("Himax i2c_checksum_43_2_size = %d \n",sizeof(c43_2));
	printk("Himax i2c_checksum_43_2 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x36C;
	printk("Himax i2c_checksum_43_2 = %d \n",i2c_CheckSum);
	//------------------------------------------------------------------config bank PART c43_2 END
	
	//------------------------------------------------------------------config bank PART c44_1 START
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
			
	data[0] = 0x00;
	data[1] = 0xDA;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}			
			
	//if((i2c_himax_master_write(touch_i2c, &c44_1[0],sizeof(c44_1),3))<0)
	if( himax_i2c_write_data(i2c_client, c44_1[0], sizeof(c44_1)-1, &(c44_1[0])) < 0 )
	{	
		goto HimaxErr;
	}
	
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	for( i=0 ; i<sizeof(c44_1) ; i++ )
	{
		i2c_CheckSum += c44_1[i];
	}
	printk("Himax i2c_checksum_44_1_size = %d \n",sizeof(c44_1));
	printk("Himax i2c_checksum_44_1 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x389;
	printk("Himax i2c_checksum_44_1 = %d \n",i2c_CheckSum);
	//------------------------------------------------------------------config bank PART c44_1 END
	
	//------------------------------------------------------------------config bank PART c44_2 START
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
			
	data[0] = 0x00;
	data[1] = 0xF9;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}			

	if( himax_i2c_write_data(i2c_client, c44_2[0], sizeof(c44_2)-1, &(c44_2[1])) < 0 )
	{	
		goto HimaxErr;
	}
	
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	for( i=0 ; i<sizeof(c44_2) ; i++ )
	{
		i2c_CheckSum += c44_2[i];
	}
	printk("Himax i2c_checksum_44_2_size = %d \n",sizeof(c44_2));
	printk("Himax i2c_checksum_44_2 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x3A8;
	printk("Himax i2c_checksum_44_2 = %d \n",i2c_CheckSum);
	//------------------------------------------------------------------config bank PART c44_2 END
	
	//------------------------------------------------------------------config bank PART c45 START
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
			
	data[0] = 0x00;
	data[1] = 0xFE;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}			
			
	if( himax_i2c_write_data(i2c_client, c45[0], sizeof(c45)-1, &(c45[1])) < 0 )
	{	
		goto HimaxErr;
	}
	
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	for( i=0 ; i<sizeof(c45) ; i++ )
	{
		i2c_CheckSum += c45[i];
	}
	printk("Himax i2c_checksum_45_size = %d \n",sizeof(c45));
	printk("Himax i2c_checksum_45 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x3AD;
	printk("Himax i2c_checksum_45 = %d \n",i2c_CheckSum);
	//------------------------------------------------------------------config bank PART c45 END
	
	data[0] = 0x10;
	if( himax_i2c_write_data(i2c_client, 0xAB, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	i2c_CheckSum += 0xAB;
	i2c_CheckSum += 0x10;
	
	printk("Himax i2c_checksum_Final = %d \n",i2c_CheckSum);
	
	data[0] = i2c_CheckSum & 0xFF;
	data[1] = (i2c_CheckSum >> 8) & 0xFF;
	if( himax_i2c_write_data(i2c_client, 0xAC, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}
	
	printk("Himax i2c_checksum_AC = %d , %d \n",data[1],data[2]);
	
	int ret = himax_i2c_read_data(i2c_client, 0xAB, 2, &(buf0[0]));
	if(ret < 0)
	{
		printk(KERN_ERR "[Himax]:i2c_himax_read 0xDA failed line: %d \n",__LINE__);
		goto HimaxErr;
	}
	
	if(buf0[0] == 0x18)
	{
		return -1;
	}
	
	if(buf0[0] == 0x10)
	{
		return 1;
	}
			
	return 1;
	HimaxErr:
	return -1;		
}
#endif
//----[ HX_LOADIN_CONFIG ]------------------------------------------------------------------------------end

//----[HX_FW_UPDATE_BY_I_FILE]--------------------------------------------------------------------------start
#ifdef HX_FW_UPDATE_BY_I_FILE
int fts_ctpm_fw_upgrade_with_i_file(void)
{
	printk("======== func:%s line:%d ========\n",__func__,__LINE__);//wangli_20140505
	unsigned char* ImageBuffer = i_CTPM_FW;
	int fullFileLength = sizeof(i_CTPM_FW); //Paul Check

	int i, j;
	uint8_t cmd[5], last_byte, prePage;
	int FileLength;
	uint8_t checksumResult = 0;

	//Try 3 Times
	for (j = 0; j < 3; j++) 
	{
		if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_CRC)
		{
			FileLength = fullFileLength;
		}
		else
		{
			FileLength = fullFileLength - 2;
		}

		#ifdef HX_RST_PIN_FUNC
		himax_HW_reset();
		#endif

		if( himax_i2c_write_data(i2c_client, 0x81, 0, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		mdelay(120);

		himax_unlock_flash(); //ok

		cmd[0] = 0x05;
		cmd[1] = 0x00;
		cmd[2] = 0x02;
		if( himax_i2c_write_data(i2c_client, 0x43, 3, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_write_data(i2c_client, 0x4F, 0, &(cmd[0])) < 0 )
		{

			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}
		mdelay(50);

		himax_ManualMode(1);
		himax_FlashMode(1);

		FileLength = (FileLength + 3) / 4;
		for (i = 0, prePage = 0; i < FileLength; i++) 
		{
			last_byte = 0;

			cmd[0] = i & 0x1F;
			if (cmd[0] == 0x1F || i == FileLength - 1)
			{
				last_byte = 1;
			}
			cmd[1] = (i >> 5) & 0x1F;
			cmd[2] = (i >> 10) & 0x1F;





			if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			if (prePage != cmd[1] || i == 0) 
			{
				prePage = cmd[1];

				cmd[0] = 0x01;
				cmd[1] = 0x09;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x0D;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x09;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}
			}

			memcpy(&cmd[0], &ImageBuffer[4*i], 4);//Paul
			if( himax_i2c_write_data(i2c_client, 0x45, 4, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			cmd[0] = 0x01;
			cmd[1] = 0x0D;//cmd[2] = 0x02;
			if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			cmd[0] = 0x01;
			cmd[1] = 0x09;//cmd[2] = 0x02;
			if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			if (last_byte == 1) 
			{
				cmd[0] = 0x01;
				cmd[1] = 0x01;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{

					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x05;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x01;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x00;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}
				
				mdelay(10);
				if (i == (FileLength - 1))
				{
					//update time too long and need kick dog
					//mtk_wdt_restart(WK_WDT_EXT_TYPE); //kick external WDT
					//mtk_wdt_restart(WK_WDT_LOC_TYPE); //local WDT CPU0/CPU1 kick

					himax_FlashMode(0);
					himax_ManualMode(0);
					checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);

					printk("======== func:%s line:%d checksumResult:%d ========\n",__func__,__LINE__,checksumResult);//wangli_20140505
					himax_lock_flash();

					if (checksumResult) //Success
					{
						printk("======== func:%s SUCCESS line:%d ========\n",__func__,__LINE__);//wangli_20140505
						return 1;
					} 
					else //Fail
					{
						printk("======== func:%s FAIL line:%d ========\n",__func__,__LINE__);//wangli_20140505
						return 0;
					} 
				}
			}
		}
	}
	//return 0;
}




static int hx8526_read_flash(unsigned char *buf, unsigned int addr_start, unsigned int length) //OK
{
	u16 i;
	unsigned int j = 0;
	unsigned char add_buf[4];
	printk("======== func:%s,line:%d addr_start:%d ========\n",__func__,__LINE__,addr_start);//wangli_20140505
	for (i = addr_start; i < addr_start+length; i++)
	{
		
		add_buf[0] = i & 0x1F;
		add_buf[1] = (i >> 5) & 0x1F;
		add_buf[2] = (i >> 10) & 0x1F;
					
		if((i2c_smbus_write_i2c_block_data(i2c_client, 0x44, 3, &add_buf[0]))< 0){
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}
		udelay(10);
					
		if((i2c_smbus_write_i2c_block_data(i2c_client, 0x46, 0, &add_buf[0]))< 0){
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}
		udelay(10);
		if((i2c_smbus_read_i2c_block_data(i2c_client, 0x59, 4, &buf[0+j]))< 0){
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}	

		udelay(10);
		j=j+4;
	}

	return 1;
}


static int Check_FW_Version(void)
{
	int tp_ver, i_file_ver;
	unsigned char FW_VER_MAJ_FLASH_buff[FW_VER_MAJ_FLASH_LENG];
	unsigned char FW_VER_MIN_FLASH_buff[FW_VER_MIN_FLASH_LENG];
	unsigned char CFG_VER_MAJ_FLASH_buff[CFG_VER_MAJ_FLASH_LENG];
	unsigned char CFG_VER_MIN_FLASH_buff[CFG_VER_MIN_FLASH_LENG];//wangli_20140506
	unsigned char fw_maj_addr; 
	unsigned char fw_min_addr; 
	unsigned char cfg_maj_addr;
	unsigned char cfg_min_addr; 
	unsigned int i;
	unsigned char cmd[5];
	fw_maj_addr=FW_VER_MAJ_FLASH_ADDR/4;
	fw_min_addr=FW_VER_MIN_FLASH_ADDR/4;
	cfg_maj_addr=CFG_VER_MAJ_FLASH_ADDR/4;
	cfg_min_addr=CFG_VER_MIN_FLASH_ADDR/4;
	cmd[0] =0x02;
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x42, 1, &cmd[0]))< 0)
	{
		return -1;
	}
	mdelay(1);
			
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x81, 0, &cmd[0]))< 0)
	{
		printk(KERN_ERR "FW_Version", __LINE__);
		return -1;
	}
	mdelay(120);

	//Himax: Flash Read Enable
	cmd[0] = 0x01;	cmd[1] = 0x00;	cmd[2] = 0x02;	
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)	goto err;	
	//udelay(10);//wangli_20140505
	udelay(30);	

	//Himax: Read FW Major Version//wangli_20140504
	//printk("======== func:%s,line:%d cfg_min_addr:%d ========\n",__func__,__LINE__,cfg_min_addr);//wangli_20140505
	if (hx8526_read_flash(FW_VER_MAJ_FLASH_buff, fw_maj_addr, FW_VER_MAJ_FLASH_LENG) < 0)	goto err;
	if (hx8526_read_flash(FW_VER_MIN_FLASH_buff, fw_min_addr, FW_VER_MIN_FLASH_LENG) < 0)	goto err;
	if (hx8526_read_flash(CFG_VER_MAJ_FLASH_buff, cfg_maj_addr, CFG_VER_MAJ_FLASH_LENG) < 0)	goto err; 
	if (hx8526_read_flash(CFG_VER_MIN_FLASH_buff, cfg_min_addr, CFG_VER_MIN_FLASH_LENG) < 0)goto err;
	
	printk("======== func:%s,line:%d ========\n",__func__,__LINE__);//wangli_20140505	
	//Himax: Flash Read Disable
	cmd[0] = 0x00;	cmd[1] = 0x00;	cmd[2] = 0x02;	
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
	{
		printk(KERN_ERR "FW_Version i2c fail", __LINE__);
		//udelay(10);//wangli_20140505
		udelay(30);
		return -1;
	}
		
	#ifdef GN_MTK_BSP_DEVICECHECK
    	struct gn_device_info gn_mydev_info;
    	gn_mydev_info.gn_dev_type = GN_DEVICE_TYPE_TP;
    	sprintf(gn_mydev_info.name,"HX_%s_ofilm",CFG_VER_MIN_FLASH_buff);
    	gn_set_device_info(gn_mydev_info);
	#endif
	
	printk("======== func:%s,line:%d ========\n",__func__,__LINE__);//wangli_20140505		
	for (i = 0; i < CFG_VER_MIN_FLASH_LENG ; i++)	
	{
		printk(KERN_ERR "++++read FW_VER buff[%d]=0x%x,i_file=0x%x\n",i,CFG_VER_MIN_FLASH_buff[i],*(i_CTPM_FW + (CFG_VER_MIN_FLASH_ADDR ) + i));	
		if (CFG_VER_MIN_FLASH_buff[i] != *(i_CTPM_FW + (CFG_VER_MIN_FLASH_ADDR) + i))	
			return 1;	
	}

	return 0;

	err:
		printk(KERN_ERR "Himax TP[%s]:[%s] read FW version error exit\n",__func__,__LINE__);
		return -1;
}
		
		
static int himax_read_FW_checksum(void)  //20130124z
{
	printk("======== func:%s,line:%d ========\n",__func__,__LINE__);//wangli_20140505
	int fullFileLength = sizeof(i_CTPM_FW); //sizeof(HIMAX_FW); //Paul Check
	//u16 rem;
	u16 FLASH_VER_START_ADDR =1030;
	u16 FW_VER_END_ADDR = 4120;
	u16 i, j, k,m;
	unsigned char cmd[4];
	int ret;
	u8 fail_count = 0;
				
	printk(KERN_ERR "Himax  TP version check start");
	//if((i2c_smbus_write_i2c_block_data(i2c_client, 0x81, 0, &cmd[0]))< 0)	{ret = -1;	goto ret_proc;}	
	//mdelay(120);
			
	//rem = fullFileLength % 4;
	//FLASH_VER_START_ADDR = (fullFileLength / 4) - 3;
	FLASH_VER_START_ADDR = (fullFileLength / 4) - 1;
	//if (rem == 0 && FLASH_VER_START_ADDR > 1)
	//{	
	//	FLASH_VER_START_ADDR--;
	//	rem = 4;
	//}
	FW_VER_END_ADDR = FLASH_VER_START_ADDR * 4;

	himax_FlashMode(1);	

	printk(KERN_ERR "Himax TP version check for loop start");
	printk(KERN_ERR "FLASH_VER_START_ADDR=%d\n", FLASH_VER_START_ADDR);
	printk(KERN_ERR "FW_VER_END_ADDR=%d\n", FW_VER_END_ADDR);
	for (k = 0; k < 3; k++)
	{
		ret = 1;
		j = FW_VER_END_ADDR;
				
		cmd[0] = FLASH_VER_START_ADDR & 0x1F;
		cmd[1] = (FLASH_VER_START_ADDR >> 5) & 0x1F;
		cmd[2] = (FLASH_VER_START_ADDR >> 10) & 0x1F;
				
		if((i2c_smbus_write_i2c_block_data(i2c_client, 0x44, 3, &cmd[0]))< 0)	{ret = -1;	goto ret_proc;}
		if((i2c_smbus_write_i2c_block_data(i2c_client, 0x46, 0, &cmd[0]))< 0)	{ret = -1;	goto ret_proc;}
		if((i2c_smbus_read_i2c_block_data(i2c_client, 0x59, 4, &cmd[0]))< 0)	{ret = -1; 	goto ret_proc;}
				
		//for (i = 0; i < rem; i++)
		for (i = 0; i < 4; i++)
		{
			printk("Himax TP version check, CTPW[%x]:%x, cmd[0]:%x\n", j, i_CTPM_FW[j], cmd[i]);
			if (i_CTPM_FW[j] != cmd[i])
			{
				ret = 0;
				break;
			}
			j++;
		}

		if (ret == 0)	fail_count++;
		if (ret == 1)	break;
	}
			
	ret_proc:
		himax_FlashMode(0);	
			
		printk(KERN_ERR "Himax TP version check loop count[%d], fail count[%d]\n", k, fail_count);
		printk(KERN_ERR "Himax TP version check for loop end, return:%d", ret);
		//printk("\n================END himax_read_FW_checksum() ret = %d=================\n",ret);//wangli_20140505
		return ret;
}
		
		

static bool i_Check_FW_Version()
{
	u16 cfg_min_start_addr;
	u16 cfg_min_end_addr;
	u16 cfg_min_addr;
	u16 cfg_min_length;

	uint8_t cmd[12];
	u16 i = 0;
	u16 j = 0;	
	u16 k = 0;


	cfg_min_start_addr   = CFG_VER_MIN_FLASH_ADDR / 4;	// start addr = 134 / 4 = 33
	cfg_min_length       = CFG_VER_MIN_FLASH_LENG;		// length = 1
	cfg_min_end_addr     = (CFG_VER_MIN_FLASH_ADDR + cfg_min_length ) / 4 + 1;//wangli		// end addr = 135 / 4 = 33
	cfg_min_addr         = CFG_VER_MIN_FLASH_ADDR % 4;	// 134 mod 4 = 2	

	//Sleep out
	if( himax_i2c_write_data(i2c_client, 0x81, 0, &(cmd[0])) < 0)
	{
		printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
	}
	mdelay(120);

	//Enter flash mode
	himax_FlashMode(1);

	//FW Version MIN
	i = cfg_min_start_addr;//wangli
	do
	{
		cmd[0] = i & 0x1F;		//column	= 33 mod 32		= 1
		cmd[1] = (i >> 5) & 0x1F;	//page		= 33 / 32		= 1
		cmd[2] = (i >> 10) & 0x1F;	//sector	= 33 / 1024		= 0

		if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		}
		
		if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		}
		
		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0)
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
		}
		if(i == cfg_min_start_addr) //first page
		{
			j = 0;
			for(k = cfg_min_addr; k < 4 && j < cfg_min_length; k++)
			{
				CFG_VER_MIN_buff[j++] = cmd[k];
			}
		}
		else //other page
		{
			for(k = 0; k < 4 && j < cfg_min_length; k++)
			{
				CFG_VER_MIN_buff[j++] = cmd[k];
			}
		}
		i++;
	}while(i < cfg_min_end_addr);

	printk("Himax %s FW_VER_MIN_buff = %d \n",__func__,FW_VER_MIN_buff[0]);
	printk("Himax %s i_CTPM_FW[%d] = %d \n",__func__,FW_VER_MIN_FLASH_ADDR,i_CTPM_FW[FW_VER_MIN_FLASH_ADDR]);

	for (i = 0; i < CFG_VER_MIN_FLASH_LENG ; i++)	
	{
		//printk(KERN_ERR "++++read FW_VER buff[%d]=0x%x,i_file=0x%x\n",i,CFG_VER_MIN_FLASH_buff[i],*(i_CTPM_FW + (CFG_VER_MIN_FLASH_ADDR ) + i));//wangli_20140505	
		if (CFG_VER_MIN_buff[i] != *(i_CTPM_FW + (CFG_VER_MIN_FLASH_ADDR) + i))	
			return true;	
	}


  //TODO i_Check_FW_Version only check the FW VER Minor , please modify the condition by customer requirement.
/*	if(FW_VER_MIN_buff[0] < i_CTPM_FW[FW_VER_MIN_FLASH_ADDR])
	{
		return true; //need update
	}
	*/
	return false;
}


#ifdef CONFIG_SUPPORT_CTP_UPG
//wangli_20140618========================================================================================
int fw_upgrade_ioctl_with_i_file(unsigned char *i_buf,unsigned int i_length)
{
	printk("2.==== GO fw_upgrade_ioctl_with_i_file ====\n");
	unsigned char* ImageBuffer = i_buf;
	int fullFileLength = i_length;

	int i, j;
	uint8_t cmd[5], last_byte, prePage;
	int FileLength;
	uint8_t checksumResult = 0;

	//Try 3 Times
	for (j = 0; j < 3; j++) 
	{
		if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_CRC)
		{
			FileLength = fullFileLength;
		}
		else
		{
			FileLength = fullFileLength - 2;
		}

		#ifdef HX_RST_PIN_FUNC
		himax_HW_reset();
		#endif

		if( himax_i2c_write_data(i2c_client, 0x81, 0, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		mdelay(120);

		himax_unlock_flash(); //ok

		cmd[0] = 0x05;
		cmd[1] = 0x00;
		cmd[2] = 0x02;
		if( himax_i2c_write_data(i2c_client, 0x43, 3, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_write_data(i2c_client, 0x4F, 0, &(cmd[0])) < 0 )
		{

			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}
		mdelay(50);

		himax_ManualMode(1);
		himax_FlashMode(1);

		FileLength = (FileLength + 3) / 4;
		printk("======== FileLength= %d ========\n",FileLength);//wl
		for (i = 0, prePage = 0; i < FileLength; i++) 
		{
			last_byte = 0;

			cmd[0] = i & 0x1F;
			if (cmd[0] == 0x1F || i == FileLength - 1)
			{
				last_byte = 1;
			}
			cmd[1] = (i >> 5) & 0x1F;
			cmd[2] = (i >> 10) & 0x1F;





			if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			if (prePage != cmd[1] || i == 0) 
			{
				prePage = cmd[1];

				cmd[0] = 0x01;
				cmd[1] = 0x09;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x0D;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x09;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}
			}

			memcpy(&cmd[0], &ImageBuffer[4*i], 4);//Paul
			if( himax_i2c_write_data(i2c_client, 0x45, 4, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			cmd[0] = 0x01;
			cmd[1] = 0x0D;//cmd[2] = 0x02;
			if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			cmd[0] = 0x01;
			cmd[1] = 0x09;//cmd[2] = 0x02;
			if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			if (last_byte == 1) 
			{
				cmd[0] = 0x01;
				cmd[1] = 0x01;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{

					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x05;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x01;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x00;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}
				
				mdelay(10);
				if (i == (FileLength - 1))
				{
					//update time too long and need kick dog
					mtk_wdt_restart(WK_WDT_EXT_TYPE); //kick external WDT
					mtk_wdt_restart(WK_WDT_LOC_TYPE); //local WDT CPU0/CPU1 kick

					himax_FlashMode(0);
					himax_ManualMode(0);
					checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);

					printk("======== func:%s line:%d checksumResult:%d ========\n",__func__,__LINE__,checksumResult);//wangli_20140505
					himax_lock_flash();

					if (checksumResult) //Success
					{
						printk("======== func:%s SUCCESS line:%d ========\n",__func__,__LINE__);//wangli_20140505
						return 1;
					} 
					else //Fail
					{
						printk("======== func:%s FAIL line:%d ========\n",__func__,__LINE__);//wangli_20140505
						return 0;
					} 
				}
			}
		}
	}
	//return 0;
}

static unsigned char fw_upgrade_ioctl(unsigned char *i_buf,unsigned int i_length)
{
	int ret = 0;
	printk("1.==== GO fw_upgrade_ioctl ====\n");
	//tpd_resume();
	if (i_Check_FW_Version() > 0 || himax_calculateChecksum(i_buf, i_length) == 0 )
	{
		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
		ret = fw_upgrade_ioctl_with_i_file(i_buf,i_length);

		if(0 == ret)
		{
			printk("======== TP upgrade error ========\n");//wl
		}
		else
		{
			printk("======== TP upgrade OK ========\n");//wl
		}	
	}

	return ret;

}

//wangli_20140618 end========================================================================================
#endif

static int i_update_func(void)
{
	unsigned char* ImageBuffer = i_CTPM_FW;
  int fullFileLength = sizeof(i_CTPM_FW); 
    	
	if (i_Check_FW_Version() > 0 || himax_calculateChecksum(ImageBuffer, fullFileLength) == 0 )
	{
		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
		
		if(fts_ctpm_fw_upgrade_with_i_file() == 0)
			printk(KERN_ERR "TP upgrade error, line: %d\n", __LINE__);
		else
			printk(KERN_ERR "TP upgrade OK, line: %d\n", __LINE__);
/*			
		#ifdef HX_RST_PIN_FUNC
		himax_HW_reset();
		//msleep(50);
		himax_ts_poweron(); 
		#endif
*/		
		//mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	}

	return 0;
}

#endif
//----[HX_FW_UPDATE_BY_I_FILE]----------------------------------------------------------------------------end

//-------proximity ---------------------------------------------------------------------

#ifdef TPD_PROXIMITY
	static int tpd_get_ps_value(void)
	{
		return tpd_proximity_detect;
	}
	static int tpd_enable_ps(int enable)
	{
		int ret = 0;
		char data[1] = {0};
		
		if(enable)	//Proximity On
		{
			data[0] =0x01;
			if((i2c_smbus_write_i2c_block_data(i2c_client, 0x92, 1, &data[0]))< 0)
				ret = 1;
			else
				tpd_proximity_flag = 1;
		}
		else	//Proximity On
		{
			data[0] =0x00;
			if((i2c_smbus_write_i2c_block_data(i2c_client, 0x92, 1, &data[0]))< 0)
				ret = 1;
			else
				tpd_proximity_flag = 0;
		}
		msleep(1);

		return ret;
	}
	int tpd_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
			void* buff_out, int size_out, int* actualout)
	{
		int err = 0;
		int value;
		hwm_sensor_data *sensor_data;
		switch (command)
		{
			case SENSOR_DELAY:
				if((buff_in == NULL) || (size_in < sizeof(int)))
				{
					TPD_PROXIMITY_DMESG("Set delay parameter error!\n");
					err = -EINVAL;
				}
				break;
			case SENSOR_ENABLE:
				if((buff_in == NULL) || (size_in < sizeof(int)))
				{
					TPD_PROXIMITY_DMESG("Enable sensor parameter error!\n");
					err = -EINVAL;
				}
				else
				{				
					value = *(int *)buff_in;
					if(value)
					{
						if((tpd_enable_ps(1) != 0))
						{
							TPD_PROXIMITY_DMESG("enable ps fail: %d\n", err); 
							return -1;
						}
					}
					else
					{
						if((tpd_enable_ps(0) != 0))
						{
							TPD_PROXIMITY_DMESG("disable ps fail: %d\n", err); 
							return -1;
						}
					}
				}
				break;
			case SENSOR_GET_DATA:
				if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
				{
					TPD_PROXIMITY_DMESG("get sensor data parameter error!\n");
					err = -EINVAL;
				}
				else
				{
					sensor_data = (hwm_sensor_data *)buff_out;				
					sensor_data->values[0] = tpd_get_ps_value();
					sensor_data->value_divide = 1;
					sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;		
				}
				break;
			default:
				TPD_PROXIMITY_DMESG("proxmy sensor operate function no this parameter %d!\n", command);
				err = -1;
				break;
		}
		return err;
	}
	#endif

//-----------proximity -----------------------------------------------------------------------------



static int himax_ts_poweron(void)
{
	uint8_t buf0[11];
	int ret = 0;
	int config_fail_retry = 0;
    //printk("======== [Himax]GOTO %s ========\n",__func__);//SWU_fail_0731
	//----[ HX_85XX_C_SERIES_PWON]----------------------------------------------------------------------start
	if (IC_TYPE == HX_85XX_C_SERIES_PWON)
	{
		buf0[0] = 0x42;
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x35;
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x36;
		buf0[1] = 0x0F;
		buf0[2] = 0x53;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0xDD;
		buf0[1] = 0x05;
		buf0[2] = 0x03;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0xB9;
		buf0[1] = 0x01;
		buf0[2] = 0x2D;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		udelay(100);

		buf0[0] = 0xE3;
		buf0[1] = 0x00;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		udelay(100);

		buf0[0] = 0x83;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		msleep(120); //120ms

		#ifdef HX_LOADIN_CONFIG
		if(himax_config_flow() == -1)
		{
			printk("Himax send config fail\n");
			//goto send_i2c_msg_fail;
		}
		msleep(100); //100ms
		#endif

		buf0[0] = 0x81;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0)
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		msleep(120); //120ms
	}
	else if (IC_TYPE == HX_85XX_A_SERIES_PWON)
	{
		buf0[0] = 0x42;
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}

		udelay(100);

		buf0[0] = 0x35;
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x36;
		buf0[1] = 0x0F;
		buf0[2] = 0x53;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		udelay(100);

		buf0[0] = 0xDD;
		buf0[1] = 0x06;
		buf0[2] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		udelay(100);

		buf0[0] = 0x76;
		buf0[1] = 0x01;
		buf0[2] = 0x2D;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		udelay(100);

		#ifdef HX_LOADIN_CONFIG
		if(himax_config_flow() == -1)
		{
			printk("Himax send config fail\n");
		}
		msleep(100); //100ms
		#endif

		buf0[0] = 0x83;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		msleep(120); //120ms

		buf0[0] = 0x81;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		msleep(120); //120ms
	}
	else if (IC_TYPE == HX_85XX_B_SERIES_PWON) 
	{
		buf0[0] = 0x42;
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x35;
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);   

		buf0[0] = 0x36;
		buf0[1] = 0x0F;

		buf0[2] = 0x53;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0xDD;
		buf0[1] = 0x06;
		buf0[2] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x76;
		buf0[1] = 0x01;
		buf0[2] = 0x2D;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0) 
		{	
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		udelay(100);

		#ifdef HX_LOADIN_CONFIG
		if(himax_config_flow() == -1)
		{
			printk("Himax send config fail\n");
		}
		msleep(100); //100ms
		#endif

		buf0[0] = 0x83;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		msleep(120); //120ms

		buf0[0] = 0x81;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		msleep(120); //120ms
	}
	else if (IC_TYPE == HX_85XX_D_SERIES_PWON)
	{
        //printk("======== [Himax]%s IC_TYPE==HX_85XX_D_SERIES_PWON ========\n",__func__);//SWU_fail_0731
		buf0[0] = 0x42;	//0x42
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x36;	//0x36
		buf0[1] = 0x0F;
		buf0[2] = 0x53;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		udelay(100);

		buf0[0] = 0xDD;	//0xDD
		//buf0[1] = 0x04;
		buf0[1] = 0x05;//To avoid FW errors //wangli_20140522
		buf0[2] = 0x03;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		udelay(100);

		buf0[0] = 0xB9;	//setCVDD
		buf0[1] = 0x01;
		buf0[2] = 0x36;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0xCB;
		buf0[1] = 0x01;
		buf0[2] = 0xF5;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		udelay(100);
		//normal mode wangli_20140613
		buf0[0] = 0x91;
		buf0[1] = 0x00;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &buf0[1]);
		if(ret < 0)
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;			
		}
		//normal mode end
		/*
		buf0[0] = HX_CMD_TSSON;
		ret = i2c_himax_master_write(ts_modify->client, buf0, 1, DEFAULT_RETRY_CNT);//sense on
		if(ret < 0) 
		{
		printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",ts_modify->client->addr);
		} 
		msleep(120); //120ms
		*/

		#ifdef HX_LOADIN_CONFIG				
			//load config
			printk("Himax start load config.\n");
			config_fail_retry = 0;
			while(true)
			{
				if(himax_config_flow() == -1)
				{
					config_fail_retry++;
					if(config_fail_retry == 3)
					{
						printk("himax_config_flow retry fail.\n");
						goto send_i2c_msg_fail;
					}
					printk("Himax config retry = %d \n",config_fail_retry);
				}
				else
				{
					break;
				}
			}
			printk("Himax end load config.\n");
			msleep(100); //100ms
		#endif

		buf0[0] = 0x83;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 
		//msleep(120); //120ms
		msleep(50);

		buf0[0] = 0x81;	//0x81
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		} 	
		//msleep(120); //120ms
	}
	else
	{
		printk("[Himax] %s IC Type Error. \n",__func__);
	}
	return ret;

send_i2c_msg_fail:

	printk(KERN_ERR "[Himax]:send_i2c_msg_failline: %d \n",__LINE__);
	
		return -1;
}

static uint8_t himax_calculateChecksum(char *ImageBuffer, int fullLength)//, int address, int RST)
{
	//----[ HX_TP_BIN_CHECKSUM_SW]----------------------------------------------------------------------start
	if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_SW)
	{
		u16 checksum = 0;
		uint8_t cmd[5], last_byte;
		int FileLength, i, readLen, k, lastLength;

		FileLength = fullLength - 2;
		memset(cmd, 0x00, sizeof(cmd));

		himax_FlashMode(1);

		FileLength = (FileLength + 3) / 4;
		for (i = 0; i < FileLength; i++) 
		{
			last_byte = 0;
			readLen = 0;

			cmd[0] = i & 0x1F;
			if (cmd[0] == 0x1F || i == FileLength - 1)
			last_byte = 1;
			cmd[1] = (i >> 5) & 0x1F;cmd[2] = (i >> 10) & 0x1F;
			if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			if (i < (FileLength - 1))
			{
				checksum += cmd[0] + cmd[1] + cmd[2] + cmd[3];
				if (i == 0)
				{
					printk(KERN_ERR "[TP] %s: himax_marked cmd 0 to 3 (first 4 bytes): %d, %d, %d, %d\n", __func__, cmd[0], cmd[1], cmd[2], cmd[3]);
				}
			}
			else 
			{
				printk(KERN_ERR "[TP] %s: himax_marked cmd 0 to 3 (last 4 bytes): %d, %d, %d, %d\n", __func__, cmd[0], cmd[1], cmd[2], cmd[3]);
				printk(KERN_ERR "[TP] %s: himax_marked, checksum (not last): %d\n", __func__, checksum);

				lastLength = (((fullLength - 2) % 4) > 0)?((fullLength - 2) % 4):4;

				for (k = 0; k < lastLength; k++) 
				{
					checksum += cmd[k];
				}
				printk(KERN_ERR "[TP] %s: himax_marked, checksum (final): %d\n", __func__, checksum);

				//Check Success
				if (ImageBuffer[fullLength - 1] == (u8)(0xFF & (checksum >> 8)) && ImageBuffer[fullLength - 2] == (u8)(0xFF & checksum)) 
				{
					himax_FlashMode(0);
					return 1;
				} 
				else //Check Fail
				{
					himax_FlashMode(0);
					return 0;
				}
			}
		}
	}
	else if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_HW)
	{
		u32 sw_checksum = 0;
		u32 hw_checksum = 0;
		uint8_t cmd[5], last_byte;
		int FileLength, i, readLen, k, lastLength;

		FileLength = fullLength;
		memset(cmd, 0x00, sizeof(cmd));

		himax_FlashMode(1);

		FileLength = (FileLength + 3) / 4;
		for (i = 0; i < FileLength; i++) 
		{
			last_byte = 0;
			readLen = 0;
			
			cmd[0] = i & 0x1F;
			if (cmd[0] == 0x1F || i == FileLength - 1)
			{
				last_byte = 1;
			}
			cmd[1] = (i >> 5) & 0x1F;cmd[2] = (i >> 10) & 0x1F;
			if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			if (i < (FileLength - 1))
			{
				sw_checksum += cmd[0] + cmd[1] + cmd[2] + cmd[3];
				if (i == 0)
				{
					printk(KERN_ERR "[TP] %s: himax_marked cmd 0 to 3 (first 4 bytes): %d, %d, %d, %d\n", __func__, cmd[0], cmd[1], cmd[2], cmd[3]);
				}
			}
			else 
			{
				printk(KERN_ERR "[TP] %s: himax_marked cmd 0 to 3 (last 4 bytes): %d, %d, %d, %d\n", __func__, cmd[0], cmd[1], cmd[2], cmd[3]);
				printk(KERN_ERR "[TP] %s: himax_marked, sw_checksum (not last): %d\n", __func__, sw_checksum);

				lastLength = ((fullLength % 4) > 0)?(fullLength % 4):4;

				for (k = 0; k < lastLength; k++) 
				{
					sw_checksum += cmd[k];
				}
				printk(KERN_ERR "[TP] %s: himax_marked, sw_checksum (final): %d\n", __func__, sw_checksum);

				//Enable HW Checksum function.
				cmd[0] = 0x01;
				if( himax_i2c_write_data(i2c_client, 0xE5, 1, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				//Must sleep 5 ms.
				msleep(30);

				//Get HW Checksum. 
				if( himax_i2c_read_data(i2c_client, 0xAD, 4, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				hw_checksum = cmd[0] + cmd[1]*0x100 + cmd[2]*0x10000 + cmd[3]*1000000;
				printk("[Touch_FH] %s: himax_marked, sw_checksum (final): %d\n", __func__, sw_checksum);
				printk("[Touch_FH] %s: himax_marked, hw_checkusm (final): %d\n", __func__, hw_checksum);

				//Compare the checksum.
				if( hw_checksum == sw_checksum )
				{
					himax_FlashMode(0);
					return 1;
				}            
				else
				{
					himax_FlashMode(0);
					return 0;
				}
			}
		}
	}
	else if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_CRC)
	{
		uint8_t cmd[5];

		//Set Flash Clock Rate
		if( himax_i2c_read_data(i2c_client, 0x7F, 5, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}
		cmd[3] = 0x02;

		if( himax_i2c_write_data(i2c_client, 0x7F, 5, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		//Enable Flash
		himax_FlashMode(1);

		//Select CRC Mode
		cmd[0] = 0x05;
		cmd[1] = 0x00;
		cmd[2] = 0x00;
		if( himax_i2c_write_data(i2c_client, 0xD2, 3, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		} 

		//Enable CRC Function
		cmd[0] = 0x01;
		if( himax_i2c_write_data(i2c_client, 0xE5, 1, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}
		
		//Must delay 30 ms
		msleep(30);

		//Read HW CRC
		if( himax_i2c_read_data(i2c_client, 0xAD, 4, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}
		
		if( cmd[0] == 0 && cmd[1] == 0 && cmd[2] == 0 && cmd[3] == 0 )
		{
			himax_FlashMode(0);
			return 1;
		}
		else 
		{
			himax_FlashMode(0);
			return 0;
		}
	}
	return 0;
}

static int himax_read_flash(unsigned char *buf, unsigned int addr_start, unsigned int length) 
{
	u16 i = 0;
	u16 j = 0;
	u16 k = 0;
	uint8_t cmd[4];
	u16 local_start_addr = addr_start / 4;
	u16 local_length = length;
	u16 local_end_addr = (addr_start + length ) / 4 + 1;
	u16 local_addr = addr_start % 4;

	printk("Himax %s addr_start = %d , local_start_addr = %d , local_length = %d , local_end_addr = %d , local_addr = %d \n",__func__,addr_start,local_start_addr,local_length,local_end_addr,local_addr);
	if( himax_i2c_write_data(i2c_client, 0x81, 0, &(cmd[0])) < 0 )
	{
		printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 81 fail.\n",__func__);
		return -1;
	}
	msleep(120);
	if( himax_i2c_write_data(i2c_client, 0x82, 0, &(cmd[0])) < 0 )
	{
		printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 82 fail.\n",__func__);
		return -1;
	}
	msleep(100);
	cmd[0] = 0x01;
	if( himax_i2c_write_data(i2c_client, 0x43, 1, &(cmd[0])) < 0 )
	{
		printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
		return -1;
	}
	msleep(100);
	i = local_start_addr;
	do
	{
		cmd[0] = i & 0x1F;
		cmd[1] = (i >> 5) & 0x1F;
		cmd[2] = (i >> 10) & 0x1F;
		if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}
		
		if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}
		
		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}
		
		printk("Himax cmd[0]=%d,cmd[1]=%d,cmd[2]=%d,cmd[3]=%d\n",cmd[0],cmd[1],cmd[2],cmd[3]);
		if(i == local_start_addr) //first page
		{
			j = 0;
			for(k = local_addr; k < 4 && j < local_length; k++)
			{
				buf[j++] = cmd[k];
			}
		}
		else //other page
		{
			for(k = 0; k < 4 && j < local_length; k++)
			{
				buf[j++] = cmd[k];
			}
		}
		i++;
	}
	while(i < local_end_addr);
	cmd[0] = 0;
	if( himax_i2c_write_data(i2c_client, 0x43, 1, &(cmd[0])) < 0 )
	{
		return -1;
	}
	return 1;
}	

void calculate_point_number(void)
{
	printk("Himax HX_MAX_PT = %d\n",HX_MAX_PT);
	HX_TOUCH_INFO_POINT_CNT = HX_MAX_PT * 4 ;

	if( (HX_MAX_PT % 4) == 0)
	{
		HX_TOUCH_INFO_POINT_CNT += (HX_MAX_PT / 4) * 4 ;
	}
	else
	{
		HX_TOUCH_INFO_POINT_CNT += ((HX_MAX_PT / 4) +1) * 4 ;
	}
	printk("Himax HX_TOUCH_INFO_POINT_CNT = %d\n",HX_TOUCH_INFO_POINT_CNT);
}

void himax_touch_information(void)
{
	static unsigned char temp_buffer[6];

	TPD_DMESG("start %s, %d\n", __FUNCTION__, __LINE__);

	if(IC_TYPE == HX_85XX_A_SERIES_PWON)
	{
		HX_RX_NUM = 0;
		HX_TX_NUM = 0;
		HX_BT_NUM = 0;
		HX_X_RES = 0;
		HX_Y_RES = 0;
		HX_MAX_PT = 0;
		HX_INT_IS_EDGE = false;
	}	
	else if(IC_TYPE == HX_85XX_B_SERIES_PWON)
	{
		HX_RX_NUM = 0;
		HX_TX_NUM = 0;
		HX_BT_NUM = 0;
		HX_X_RES = 0;
		HX_Y_RES = 0;
		HX_MAX_PT = 0;
		HX_INT_IS_EDGE = false;
	}
	else if(IC_TYPE == HX_85XX_C_SERIES_PWON)
	{
		//RX,TX,BT Channel num
		himax_read_flash( temp_buffer, 0x3D5, 3);
		HX_RX_NUM = temp_buffer[0];
		HX_TX_NUM = temp_buffer[1];
		HX_BT_NUM = (temp_buffer[2]) & 0x1F;

		//Resolution
		himax_read_flash( temp_buffer, 0x345, 4);
		HX_X_RES = temp_buffer[0]*256 + temp_buffer[1];
		HX_Y_RES = temp_buffer[2]*256 + temp_buffer[3];				

		//Point number
		himax_read_flash( temp_buffer, 0x3ED, 1);
		HX_MAX_PT = temp_buffer[0] >> 4;

		//Interrupt is level or edge
		himax_read_flash( temp_buffer, 0x3EE, 2);
		if( (temp_buffer[1] && 0x01) == 1 )
		{
			HX_INT_IS_EDGE = true;
		}
		else
		{
			HX_INT_IS_EDGE = false;
		}
	}
	else if(IC_TYPE == HX_85XX_D_SERIES_PWON)
	{
		himax_read_flash( temp_buffer, 0x26E, 5);
		HX_RX_NUM = temp_buffer[0];
		HX_TX_NUM = temp_buffer[1];
		HX_MAX_PT = (temp_buffer[2] & 0xF0) >> 4;
		//HX_BT_NUM = (temp_buffer[2] & 0x0F);
		HX_XY_REV = (temp_buffer[4] & 0x04);
		
		#ifdef HX_EN_SEL_BUTTON
		HX_BT_NUM = (temp_buffer[2] & 0x0F);
		#endif
		
		
		#ifdef HX_EN_MUT_BUTTON
		himax_read_flash( temp_buffer, 0x262, 1);
		HX_BT_NUM = (temp_buffer[0] & 0x07);
		#endif

		himax_read_flash( temp_buffer, 0x272, 6);
		if(HX_XY_REV==0)
		{
		HX_X_RES = temp_buffer[2]*256 + temp_buffer[3];
		HX_Y_RES = temp_buffer[4]*256 + temp_buffer[5];
        }
		else
		{
		HX_Y_RES = temp_buffer[2]*256 + temp_buffer[3];
		HX_X_RES = temp_buffer[4]*256 + temp_buffer[5];
		}
		
		himax_read_flash( temp_buffer, 0x200, 6);
		if( (temp_buffer[1] && 0x01) == 1 )
		{
			HX_INT_IS_EDGE = true;
		}
		else
		{
			HX_INT_IS_EDGE = false;
		}
		
		printk("Himax HX_RX_NUM = %d \n",HX_RX_NUM);
		printk("Himax HX_TX_NUM = %d \n",HX_TX_NUM);
		printk("Himax HX_MAX_PT = %d \n",HX_MAX_PT);
		printk("Himax HX_BT_NUM = %d \n",HX_BT_NUM);
		printk("Himax HX_X_RES = %d \n",HX_X_RES);
		printk("Himax HX_Y_RES = %d \n",HX_Y_RES);
		if(HX_INT_IS_EDGE)
		{
			printk("Himax HX_INT_IS_EDGE = true \n");
		}
		else
		{
			printk("Himax HX_INT_IS_EDGE = false \n");
		}
	}
	else
	{
		HX_RX_NUM = 0;
		HX_TX_NUM = 0;
		HX_BT_NUM = 0;
		HX_X_RES = 0;
		HX_Y_RES = 0;
		HX_MAX_PT = 0;
		HX_INT_IS_EDGE = false;
	}

	TPD_DMESG("end %s, %d\n", __FUNCTION__, __LINE__);
}

int himax_hang_shaking(void)    //0:Running, 1:Stop, 2:I2C Fail
{
	int ret, result;
	uint8_t hw_reset_check[1];
	uint8_t hw_reset_check_2[1];
	uint8_t buf0[2];

	//Write 0x92
	buf0[0] = 0x92;
	if(IC_STATUS_CHECK == 0xAA)
	{
		buf0[1] = 0xAA;
		IC_STATUS_CHECK = 0x55;
	}
	else
	{
		buf0[1] = 0x55;
		IC_STATUS_CHECK = 0xAA;
	}

	if( himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1])) < 0 )
	{
		printk(KERN_ERR "[Himax]:write 0x92 failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}
	msleep(15); //Must more than 1 frame

	buf0[0] = 0x92;
	buf0[1] = 0x00;
	if( himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1])) < 0 )
	{
		printk(KERN_ERR "[Himax]:write 0x92 failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}
	msleep(2);

	if( himax_i2c_read_data(i2c_client, 0xDA, 1, &(hw_reset_check[0]))<0 )
	{
		printk(KERN_ERR "[Himax]:i2c_himax_read 0xDA failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}
	//printk("[Himax]: ESD 0xDA - 0x%x.\n", hw_reset_check[0]); 

	if((IC_STATUS_CHECK != hw_reset_check[0]))
	{
		msleep(2);
		if( himax_i2c_read_data(i2c_client, 0xDA, 1, &(hw_reset_check_2[0]))<0 )
		{
			printk(KERN_ERR "[Himax]:i2c_himax_read 0xDA failed line: %d \n",__LINE__);
			goto work_func_send_i2c_msg_fail;
		}
		//printk("[Himax]: ESD check 2 0xDA - 0x%x.\n", hw_reset_check_2[0]);

		if(hw_reset_check[0] == hw_reset_check_2[0])
		{
			result = 1; //MCU Stop
		}
		else
		{
			result = 0; //MCU Running
		}
	}
	else
	{
		result = 0; //MCU Running
	}

	return result;

work_func_send_i2c_msg_fail:
	return 2;
}


#ifdef HX_ESD_WORKAROUND
void ESD_HW_REST(void)
{
	ESD_RESET_ACTIVATE = 1;
	//ESD_COUNTER = 0;

	printk("Himax TP: ESD - Reset\n");

	#ifdef HX_RST_BY_POWER
		//power off by different platform's API
		hwPowerDown(MT6323_POWER_LDO_VGP1,  "TP");
		//hwPowerDown(MT65XX_POWER_LDO_VGP5, "TP_EINT");
		msleep(100);
				
		//power on by different platform's API
		hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_2800, "TP");
		//hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_1800, "TP_EINT");
		msleep(100);
		printk("Himax %s TP rst IN ESD  ESD ESD by HX_RST_BY_POWER %d\n",__func__,__LINE__);

	#else	
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
		msleep(20);
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
		msleep(20);
		printk("Himax %s TP rst  in ESD ESD ESD  by RST PIN %d\n",__func__,__LINE__);
	#endif
	
	himax_ts_poweron();

	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM); 
}
#endif
//----[HX_ESD_WORKAROUND]-------------------------------------------------------------------------------end

//----[ENABLE_CHIP_STATUS_MONITOR]--------------------------------------------------------------------start
#ifdef ENABLE_CHIP_STATUS_MONITOR
static int himax_chip_monitor_function(struct work_struct *dat) //for ESD solution
{
	int ret;
	
	if(running_status == 0)//&& himax_chip->suspend_state == 0)
	{
		if(mt_get_gpio_in(GPIO_CTP_EINT_PIN) == 0)
		{
			printk("[Himax]%s: IRQ = 0, Enable IRQ\n", __func__);
			mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM); 
		}
		
		ret = himax_hang_shaking(); //0:Running, 1:Stop, 2:I2C Fail
		if(ret == 2)
		{
		//	queue_delayed_work(himax_wq, &himax_chip_reset_work, 0);
		        ESD_HW_REST(); 
			printk(KERN_INFO "[Himax] %s: I2C Fail \n", __func__);
		}
		if(ret == 1)
		{
			printk(KERN_INFO "[Himax] %s: MCU Stop \n", __func__);
			
			#ifdef HX_ESD_WORKAROUND
			ESD_HW_REST();
			#endif
		}
		//else
		//printk(KERN_INFO "[Himax] %s: MCU Running \n", __func__);
		
		queue_delayed_work(himax_wq, &himax_chip_monitor, 10*HZ);
	}
	return 0;
}
#endif
//----[ENABLE_CHIP_STATUS_MONITOR]----------------------------------------------------------------------end

//----[HX_RST_PIN_FUNC]-------------------------------------------------------------------------------start
#ifdef HX_RST_PIN_FUNC    
void himax_HW_reset(void)
{
	#ifdef HX_ESD_WORKAROUND
	ESD_RESET_ACTIVATE = 1;
	#endif

	#ifdef HX_RST_BY_POWER
		//power off by different platform's API
		hwPowerDown(MT6323_POWER_LDO_VGP1,  "TP");
		//hwPowerDown(MT65XX_POWER_LDO_VGP5, "TP_EINT");
		msleep(100);
				
		//power on by different platform's API
		hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_2800, "TP");
		//hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_1800, "TP_EINT");
		msleep(100);			
		printk("Himax %s TP rst by HX_RST_BY_POWER %d\n",__func__,__LINE__);
	#else
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
		//msleep(100);
		msleep(5);
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
		//msleep(100);
        msleep(10);
		printk("Himax %s TP rst by RST PIN %d\n",__func__,__LINE__);
	#endif
}
#endif	
//----[HX_RST_PIN_FUNC]---------------------------------------------------------------------------------end

//=============================================================================================================
//
//	Segment : Himax SYS Debug Function
//
//=============================================================================================================

//----[HX_TP_SYS_REGISTER]------------------------------------------------------------------------------start
#ifdef HX_TP_SYS_REGISTER
static ssize_t himax_register_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int base = 0;
	uint16_t loop_i,loop_j;
	uint8_t data[128];
	uint8_t outData[5];

	memset(outData, 0x00, sizeof(outData));
	memset(data, 0x00, sizeof(data));

	printk(KERN_INFO "Himax multi_register_command = %d \n",multi_register_command);

	if(multi_register_command == 1)
	{
		base = 0;
	
		for(loop_i = 0; loop_i < 6; loop_i++)
		{
			if(multi_register[loop_i] != 0x00)
			{
				if(multi_cfg_bank[loop_i] == 1) //config bank register
				{
					outData[0] = 0x15;
					himax_i2c_write_data(i2c_client, 0xE1, 1, &(outData[0]));
					msleep(10);

					outData[0] = 0x00;
					outData[1] = multi_register[loop_i];
					himax_i2c_write_data(i2c_client, 0xD8, 2, &(outData[0]));
					msleep(10);

					himax_i2c_read_data(i2c_client, 0x5A, 128, &(data[0]));




					outData[0] = 0x00;
					himax_i2c_write_data(i2c_client, 0xE1, 1, &(outData[0]));

					for(loop_j=0; loop_j<128; loop_j++)
					{
						multi_value[base++] = data[loop_j];
					}
				}
				else //normal register
				{
					himax_i2c_read_data(i2c_client, multi_register[loop_i], 128, &(data[0]));							
					
					for(loop_j=0; loop_j<128; loop_j++)
					{
						multi_value[base++] = data[loop_j];
					}
				}
			}
		}

		base = 0;
		for(loop_i = 0; loop_i < 6; loop_i++)
		{
			if(multi_register[loop_i] != 0x00)
			{
				if(multi_cfg_bank[loop_i] == 1)
				{
					ret += sprintf(buf + ret, "Register: FE(%x)\n", multi_register[loop_i]);
				}
				else
				{
					ret += sprintf(buf + ret, "Register: %x\n", multi_register[loop_i]);
				}

				for (loop_j = 0; loop_j < 128; loop_j++)
				{
					ret += sprintf(buf + ret, "0x%2.2X ", multi_value[base++]);
					if ((loop_j % 16) == 15)
					{
						ret += sprintf(buf + ret, "\n");
					}
				}
			}
		}
		return ret;
	}

	if(config_bank_reg)
	{
		printk(KERN_INFO "[TP] %s: register_command = FE(%x)\n", __func__, register_command);

		//Config bank register read flow.
		outData[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(outData[0]));

		msleep(10);

		outData[0] = 0x00;
		outData[1] = register_command;
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(outData[0]));

		msleep(10);

		himax_i2c_read_data(i2c_client, 0x5A, 128, &(data[0]));

		msleep(10);

		outData[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(outData[0]));
	}
	else
	{
		if ( himax_i2c_read_data(i2c_client, register_command, 128, &(data[0])) < 0 )
		{
			return ret;
		}
	}

	if(config_bank_reg)
	{
		ret += sprintf(buf, "command: FE(%x)\n", register_command);
	}
	else
	{
		ret += sprintf(buf, "command: %x\n", register_command);
	}

	for (loop_i = 0; loop_i < 128; loop_i++) 
	{
		ret += sprintf(buf + ret, "0x%2.2X ", data[loop_i]);
		if ((loop_i % 16) == 15)
		{
			ret += sprintf(buf + ret, "\n");
		}
	}

	ret += sprintf(buf + ret, "\n");
	return ret;
}

static ssize_t himax_register_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	char buf_tmp[6], length = 0;
	unsigned long result = 0;
	uint8_t loop_i = 0;
	uint16_t base = 5;
	uint8_t write_da[128];
	uint8_t outData[5];

	memset(buf_tmp, 0x0, sizeof(buf_tmp));
	memset(write_da, 0x0, sizeof(write_da));
	memset(outData, 0x0, sizeof(outData));

	printk("himax %s \n",buf);

	if( buf[0] == 'm' && buf[1] == 'r' && buf[2] == ':')
	{
		memset(multi_register, 0x00, sizeof(multi_register));
		memset(multi_cfg_bank, 0x00, sizeof(multi_cfg_bank));
		memset(multi_value, 0x00, sizeof(multi_value));
		
		printk("himax multi register enter\n");
		
		multi_register_command = 1;

		base 		= 2;
		loop_i 	= 0;

		while(true)
		{
			if(buf[base] == '\n')
			{
				break;
			}

			if(loop_i >= 6 )
			{
				break;
			}

			if(buf[base] == ':' && buf[base+1] == 'x' && buf[base+2] == 'F' && buf[base+3] == 'E' && buf[base+4] != ':')
			{
				memcpy(buf_tmp, buf + base + 4, 2);
				if (!strict_strtoul(buf_tmp, 16, &result))
				{
					multi_register[loop_i] = result;
					multi_cfg_bank[loop_i++] = 1;
				}
				base += 6;
			}
			else
			{
				memcpy(buf_tmp, buf + base + 2, 2);
				if (!strict_strtoul(buf_tmp, 16, &result))
				{
					multi_register[loop_i] = result;
					multi_cfg_bank[loop_i++] = 0;
				}
				base += 4;
			}
		}

		printk(KERN_INFO "========================== \n");
		for(loop_i = 0; loop_i < 6; loop_i++)
		{
			printk(KERN_INFO "%d,%d:",multi_register[loop_i],multi_cfg_bank[loop_i]);
		}
		printk(KERN_INFO "\n");
	}
	else if ((buf[0] == 'r' || buf[0] == 'w') && buf[1] == ':') 
	{
		multi_register_command = 0;

		if (buf[2] == 'x')
		{
			if(buf[3] == 'F' && buf[4] == 'E') //Config bank register
			{
				config_bank_reg = true;

				memcpy(buf_tmp, buf + 5, 2);
				if (!strict_strtoul(buf_tmp, 16, &result))
				{
					register_command = result;
				}
				base = 7;

				printk(KERN_INFO "CMD: FE(%x)\n", register_command);
			}
			else
			{
				config_bank_reg = false;
				
				memcpy(buf_tmp, buf + 3, 2);
				if (!strict_strtoul(buf_tmp, 16, &result))
				{
					register_command = result;
				}
				base = 5;
				printk(KERN_INFO "CMD: %x\n", register_command);
			}

			for (loop_i = 0; loop_i < 128; loop_i++) 
			{
				if (buf[base] == '\n') 
				{
					if (buf[0] == 'w')
					{
						if(config_bank_reg)
						{
							outData[0] = 0x15;
							himax_i2c_write_data(i2c_client, 0xE1, 1, &(outData[0]));

							msleep(10);

							outData[0] = 0x00;
							outData[1] = register_command;
							himax_i2c_write_data(i2c_client, 0xD8, 2, &(outData[0]));

							msleep(10);
							himax_i2c_write_data(i2c_client, 0x40, length, &(write_da[0]));

							msleep(10);

							outData[0] = 0x00;
							himax_i2c_write_data(i2c_client, 0xE1, 1, &(outData[0]));

							printk(KERN_INFO "CMD: FE(%x), %x, %d\n", register_command,write_da[0], length);
						}
						else
						{
							himax_i2c_write_data(i2c_client, register_command, length, &(write_da[0]));
							printk(KERN_INFO "CMD: %x, %x, %d\n", register_command,write_da[0], length);
						}
					}

					printk(KERN_INFO "\n");
					return count;
				}
				if (buf[base + 1] == 'x') 
				{
					buf_tmp[4] = '\n';
					buf_tmp[5] = '\0';
					memcpy(buf_tmp, buf + base + 2, 2);
					if (!strict_strtoul(buf_tmp, 16, &result))
					{
						write_da[loop_i] = result;
					}
					length++;
				}
				base += 4;
			}
		}
	}
	return count;
}

static DEVICE_ATTR(register, 0644, himax_register_show, himax_register_store);
#endif
//----[HX_TP_SYS_REGISTER]--------------------------------------------------------------------------------end

//----[HX_TP_SYS_DEBUG_LEVEL]-----------------------------------------------------------------------------start
#ifdef HX_TP_SYS_DEBUG_LEVEL
int fts_ctpm_fw_upgrade_with_sys_fs(unsigned char *fw, int len)
{
	unsigned char* ImageBuffer = fw; //CTPM_FW;
	int fullFileLength = len; //sizeof(CTPM_FW); //Paul Check
	int i, j;
	uint8_t cmd[5], last_byte, prePage;
	int FileLength;
	uint8_t checksumResult = 0;

	//Try 3 Times
	for (j = 0; j < 3; j++)
	{
		if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_CRC)
		{
			FileLength = fullFileLength;
		}
		else
		{
			FileLength = fullFileLength - 2;
		}

		#ifdef HX_RST_PIN_FUNC
		himax_HW_reset();
		#endif

		if( himax_i2c_write_data(i2c_client, 0x81, 0, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		mdelay(120);

		himax_unlock_flash();  //ok

		cmd[0] = 0x05;cmd[1] = 0x00;cmd[2] = 0x02;
		if( himax_i2c_write_data(i2c_client, 0x43, 3, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_write_data(i2c_client, 0x4F, 0, &(cmd[0])) < 0 )
		{
			printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
			return -1;
		}
		mdelay(50);

		himax_ManualMode(1); 
		himax_FlashMode(1); 

		FileLength = (FileLength + 3) / 4;
		for (i = 0, prePage = 0; i < FileLength; i++) 
		{
			last_byte = 0;
			cmd[0] = i & 0x1F;
			if (cmd[0] == 0x1F || i == FileLength - 1)
			{
				last_byte = 1;
			}

			cmd[1] = (i >> 5) & 0x1F;
			cmd[2] = (i >> 10) & 0x1F;
			if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			if (prePage != cmd[1] || i == 0) 
			{
				prePage = cmd[1];
				cmd[0] = 0x01;cmd[1] = 0x09;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;cmd[1] = 0x0D;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}


				cmd[0] = 0x01;cmd[1] = 0x09;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);

					return -1;
				}
			}

			memcpy(&cmd[0], &ImageBuffer[4*i], 4);//Paul
			if( himax_i2c_write_data(i2c_client, 0x45, 4, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			cmd[0] = 0x01;cmd[1] = 0x0D;//cmd[2] = 0x02;
			if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			cmd[0] = 0x01;cmd[1] = 0x09;//cmd[2] = 0x02;
			if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
			{
				printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
				return -1;
			}

			if (last_byte == 1) 
			{
				cmd[0] = 0x01;cmd[1] = 0x01;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;cmd[1] = 0x05;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;cmd[1] = 0x01;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;cmd[1] = 0x00;//cmd[2] = 0x02;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					printk(KERN_ERR "[TP] %s: i2c access fail!\n", __func__);
					return -1;
				}

				mdelay(10);
				if (i == (FileLength - 1)) 
				{
					himax_FlashMode(0);
					himax_ManualMode(0);
					checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);//, address, RST);
					//himax_ManualMode(0);
					himax_lock_flash();
					
					if (checksumResult > 0) //Success
					{
						return 1;
					} 
					else //Fail
					{
						return 0;
					} 
				}
			}
		}
	}
	//return 0;
}

static ssize_t himax_debug_level_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t count = 0;
	int i = 0;
	
	if(debug_level_cmd == 't')
	{
		if(fw_update_complete)
		{
			count += sprintf(buf, "FW Update Complete \n");
		}
		else
		{
			count += sprintf(buf, "FW Update Fail \n");
		}
	}
	else if(debug_level_cmd == 'i')
	{
		if(irq_enable)
		{
			count += sprintf(buf, "IRQ is enable\n");
		}
		else
		{
			count += sprintf(buf, "IRQ is disable\n");
		}
	}
	else if(debug_level_cmd == 'r')
	{
		if(power_ret<0)
		{
			count += sprintf(buf, " himax power on fail \n");
		}
		else
		{
			count += sprintf(buf, "himax power on OK \n");
		}
	}
	else if(debug_level_cmd == 'h')
	{
		if(handshaking_result == 0)
		{
			count += sprintf(buf, "Handshaking Result = %d (MCU Running)\n",handshaking_result);
		}
		else if(handshaking_result == 1)
		{
			count += sprintf(buf, "Handshaking Result = %d (MCU Stop)\n",handshaking_result);
		}
		else if(handshaking_result == 2)
		{
			count += sprintf(buf, "Handshaking Result = %d (I2C Error)\n",handshaking_result);
		}
		else
		{
			count += sprintf(buf, "Handshaking Result = error \n");
		}
	}
	else if(debug_level_cmd == 'v')
	{
		count += sprintf(buf + count, "FW_VER_MAJ_buff = ");
		count += sprintf(buf + count, "0x%2.2X \n",FW_VER_MAJ_buff[0]);

		count += sprintf(buf + count, "FW_VER_MIN_buff = ");
		count += sprintf(buf + count, "0x%2.2X \n",FW_VER_MIN_buff[0]);

		count += sprintf(buf + count, "CFG_VER_MAJ_buff = ");
		for( i=0 ; i<12 ; i++)
		{
			count += sprintf(buf + count, "0x%2.2X ",CFG_VER_MAJ_buff[i]);
		}
		count += sprintf(buf + count, "\n");

		count += sprintf(buf + count, "CFG_VER_MIN_buff = ");
		for( i=0 ; i<12 ; i++)
		{
			count += sprintf(buf + count, "0x%2.2X ",CFG_VER_MIN_buff[i]);
		}
		count += sprintf(buf + count, "\n");

		count += sprintf(buf + count, "i_file_version = ");
#ifdef  HX_FW_UPDATE_BY_I_FILE
		for( i=0 ; i<12 ; i++)
		{
			count += sprintf(buf + count, "0x%2.2X ",i_CTPM_FW[CFG_VER_MIN_FLASH_ADDR + i]);
		}
		count += sprintf(buf + count, "\n");
#endif
	}
	else if(debug_level_cmd == 'd')
	{
		count += sprintf(buf + count, "Himax Touch IC Information :\n");
		if(IC_TYPE == HX_85XX_A_SERIES_PWON)
		{
			count += sprintf(buf + count, "IC Type : A\n");
		}
		else if(IC_TYPE == HX_85XX_B_SERIES_PWON)
		{
			count += sprintf(buf + count, "IC Type : B\n");
		}
		else if(IC_TYPE == HX_85XX_C_SERIES_PWON)
		{
			count += sprintf(buf + count, "IC Type : C\n");
		}
		else if(IC_TYPE == HX_85XX_D_SERIES_PWON)
		{
			count += sprintf(buf + count, "IC Type : D\n");
		}
		else 
		{
			count += sprintf(buf + count, "IC Type error.\n");
		}
		
		if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_SW)
		{
			count += sprintf(buf + count, "IC Checksum : SW\n");
		}
		else if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_HW)
		{
			count += sprintf(buf + count, "IC Checksum : HW\n");
		}
		else if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_CRC)
		{
			count += sprintf(buf + count, "IC Checksum : CRC\n");
		}
		else
		{
			count += sprintf(buf + count, "IC Checksum error.\n");
		}

		if(HX_INT_IS_EDGE)
		{
			count += sprintf(buf + count, "Interrupt : EDGE TIRGGER\n");
		}
		else
		{
			count += sprintf(buf + count, "Interrupt : LEVEL TRIGGER\n");
		}

		count += sprintf(buf + count, "RX Num : %d\n",HX_RX_NUM);
		count += sprintf(buf + count, "TX Num : %d\n",HX_TX_NUM);
		count += sprintf(buf + count, "BT Num : %d\n",HX_BT_NUM);
		count += sprintf(buf + count, "X Resolution : %d\n",HX_X_RES);
		count += sprintf(buf + count, "Y Resolution : %d\n",HX_Y_RES);
		count += sprintf(buf + count, "Max Point : %d\n",HX_MAX_PT);
	}
	else
	{
		count += sprintf(buf, "%d\n", debug_log_level);
	}
	return count;
}

static ssize_t himax_debug_level_dump(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	char data[3];
	char fileName[128];
	struct file* filp = NULL;
	mm_segment_t oldfs;
	int result = 0;
       int k;

	if (buf[0] >= '0' && buf[0] <= '9' && buf[1] == '\n')
	{
		debug_log_level = buf[0] - '0';
	}

	if (buf[0] == 'i') //irq
	{
		debug_level_cmd = buf[0];
	
		if( buf[2] == '1') //enable irq	
		{
			mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM); 
			irq_enable = true;
		}
		else if(buf[2] == '0') //disable irq
		{
			mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM); 
			irq_enable = false;
		}
		else
		{
			printk(KERN_ERR "[Himax] %s: debug_level command = 'i' , parameter error.\n", __func__);
		}
		return count;
	}

	if( buf[0] == 'h') //handshaking
	{
		debug_level_cmd = buf[0];

		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);

		handshaking_result = himax_hang_shaking(); //0:Running, 1:Stop, 2:I2C Fail 

		mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);

		return count;
	}
	
	if( buf[0] == 'v') //firmware version
	{
		debug_level_cmd = buf[0];
		himax_read_FW_ver();
		return count;
	}
	
	if( buf[0] == 'd') //test
	{
		debug_level_cmd = buf[0];
		return count;
	}
	
	if( buf[0] == 'r') //rst and power on 
	{
		debug_level_cmd = buf[0];
		himax_HW_reset();
		msleep(50);
		power_ret=himax_ts_poweron(); 
		return count;
	}
#ifdef BUTTON_CHECK
    if(buf[0] == 'b') //example : "b:10
    {
        if(sscanf(&(buf[2]), "%d", &bt_confirm_cnt) == 0)
        {
            printk("++++bt_confirm_cnt=0x%x\n", bt_confirm_cnt);            
        }
    }


    if(buf[0] == 'o') //example : "0:2
    {
        if(sscanf(&(buf[2]), "%d", &obs_intvl) == 0)
        {
            printk("++++obs_intvl=0x%x\n", obs_intvl);            
        }
    } 

    if(buf[0] == 'c') //example : "0:2
    {
        if(sscanf(&(buf[2]), "%d", &himax_debug_flag) == 0)
        {
            printk("++++himax_debug_flag=0x%x\n", himax_debug_flag);            
        }
    }  


#endif

#ifdef PT_NUM_LOG
        if(buf[0] == 'z') //example : "0:2

        {
            if(sscanf(&(buf[2]), "%d", &himax_debug_flag) == 0)
            {
                printk("++++himax_debug_flag=0x%x\n", himax_debug_flag);            
            }
        }  

        printk("++++curr_ptr=%d\n", curr_ptr);       
        for(k = 0 ; k < PT_ARRAY_SZ ; k++)
        {
            printk("[%d] ", point_cnt_array[k]);    
            if(k%10 == 9)
            {
                printk("\n");
            }
        }  
#endif

	if(buf[0] == 't')
	{
		debug_level_cmd = buf[0];
		fw_update_complete = false;

		memset(fileName, 0, 128);
		// parse the file name
		snprintf(fileName, count-2, "%s", &buf[2]);
		printk(KERN_INFO "[TP] %s: upgrade from file(%s) start!\n", __func__, fileName);
		// open file
		filp = filp_open(fileName, O_RDONLY, 0);
		if(IS_ERR(filp)) 
		{
			printk(KERN_ERR "[TP] %s: open firmware file failed\n", __func__);
			goto firmware_upgrade_done;
			//return count;
		}
		oldfs = get_fs();
		set_fs(get_ds());

		// read the latest firmware binary file
		result=filp->f_op->read(filp,upgrade_fw,sizeof(upgrade_fw), &filp->f_pos);
		if(result < 0) 
		{
			printk(KERN_ERR "[TP] %s: read firmware file failed\n", __func__);
			goto firmware_upgrade_done;
			//return count;
		}

		set_fs(oldfs);
		filp_close(filp, NULL);

		printk(KERN_INFO "[TP] %s: upgrade start,len %d: %02X, %02X, %02X, %02X\n", __func__, result, upgrade_fw[0], upgrade_fw[1], upgrade_fw[2], upgrade_fw[3]);

		if(result > 0)
		{
			// start to upgrade
			mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
			if(fts_ctpm_fw_upgrade_with_sys_fs(upgrade_fw, result) <= 0)
			{
				printk(KERN_INFO "[TP] %s: TP upgrade error, line: %d\n", __func__, __LINE__);
				fw_update_complete = false;
			}
			else
			{
				printk(KERN_INFO "[TP] %s: TP upgrade OK, line: %d\n", __func__, __LINE__);
				fw_update_complete = true;
			}
			mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
			goto firmware_upgrade_done;
			//return count;
		}
	}

	#ifdef HX_FW_UPDATE_BY_I_FILE
	if(buf[0] == 'f')
	{
		printk(KERN_INFO "[TP] %s: upgrade firmware from kernel image start!\n", __func__);
		if (i_isTP_Updated == 0)
		{
			printk("himax touch isTP_Updated: %d\n", i_isTP_Updated);
			if(1)
			{
				mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
				printk("himax touch firmware upgrade: %d\n", i_isTP_Updated);
				if(fts_ctpm_fw_upgrade_with_i_file() == 0)
				{
					printk("himax_marked TP upgrade error, line: %d\n", __LINE__);
					fw_update_complete = false;
				}
				else
				{
					printk("himax_marked TP upgrade OK, line: %d\n", __LINE__);
					fw_update_complete = true;
				}
				mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
				i_isTP_Updated = 1;
				goto firmware_upgrade_done;
			}
		}
	}
	#endif

HimaxErr:
	TPD_DMESG("Himax TP: I2C transfer error, line: %d\n", __LINE__);
	return count;

firmware_upgrade_done:
	
	return count;
}

static DEVICE_ATTR(debug_level, 0755, himax_debug_level_show, himax_debug_level_dump);
#endif
//----[HX_TP_SYS_DEBUG_LEVEL]-------------------------------------------------------------------------------end

//----[HX_TP_SYS_DIAG]------------------------------------------------------------------------------------start
#ifdef HX_TP_SYS_DIAG 
static uint8_t *getMutualBuffer(void)
{
	return diag_mutual;
}

static uint8_t *getSelfBuffer(void)
{
	return &diag_self[0];
}

static uint8_t getXChannel(void)
{
	return x_channel;
}

static uint8_t getYChannel(void)
{
	return y_channel;
}

static uint8_t getDiagCommand(void)
{
	return diag_command;
}

static void setXChannel(uint8_t x)
{
	x_channel = x;
}

static void setYChannel(uint8_t y)
{
	y_channel = y;
}

static void setMutualBuffer(void)
{
	diag_mutual = kzalloc(x_channel * y_channel * sizeof(uint8_t), GFP_KERNEL);
}

static ssize_t himax_diag_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t count = 0;
	uint32_t loop_i;
	uint16_t mutual_num, self_num, width;

	mutual_num = x_channel * y_channel;
	self_num = x_channel + y_channel; //don't add KEY_COUNT

	width = x_channel;
	count += sprintf(buf + count, "ChannelStart: %4d, %4d\n\n", x_channel, y_channel);

	// start to show out the raw data in adb shell
	if (diag_command >= 1 && diag_command <= 6)
	{
		if (diag_command <= 3)
		{
			for (loop_i = 0; loop_i < mutual_num; loop_i++) 
			{
				count += sprintf(buf + count, "%4d", diag_mutual[loop_i]);
				if ((loop_i % width) == (width - 1)) 
				{
				count += sprintf(buf + count, " %3d\n", diag_self[width + loop_i/width]);
				}
			}
			count += sprintf(buf + count, "\n");
			for (loop_i = 0; loop_i < width; loop_i++) 
			{
				count += sprintf(buf + count, "%4d", diag_self[loop_i]);
				if (((loop_i) % width) == (width - 1))
				{
					count += sprintf(buf + count, "\n");
				}
			}

			#ifdef HX_EN_SEL_BUTTON
			count += sprintf(buf + count, "\n");
			for (loop_i = 0; loop_i < HX_BT_NUM; loop_i++)
			{
				count += sprintf(buf + count, "%4d", diag_self[HX_RX_NUM + HX_TX_NUM + loop_i]);
			}
			#endif
		}
		else if (diag_command > 4) 
		{
			for (loop_i = 0; loop_i < self_num; loop_i++) 
			{
				count += sprintf(buf + count, "%4d", diag_self[loop_i]);
				if (((loop_i - mutual_num) % width) == (width - 1))
				{
					count += sprintf(buf + count, "\n");
				}
			}
		} 
		else
		{
			for (loop_i = 0; loop_i < mutual_num; loop_i++) 
			{
				count += sprintf(buf + count, "%4d", diag_mutual[loop_i]);
				if ((loop_i % width) == (width - 1))
				{
					count += sprintf(buf + count, "\n");
				}
			}
		}
		count += sprintf(buf + count, "ChannelEnd");
		count += sprintf(buf + count, "\n");
	}
	else if (diag_command == 7)
	{
		for (loop_i = 0; loop_i < 128 ;loop_i++)
		{
			if((loop_i % 16) == 0)
			{
				count += sprintf(buf + count, "LineStart:");
			}

			count += sprintf(buf + count, "%4d", diag_coor[loop_i]);
			if((loop_i % 16) == 15)
			{
				count += sprintf(buf + count, "\n");
			}
		}
	}
	return count;
}

static ssize_t himax_diag_dump(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	const uint8_t command_ec_128_raw_flag = 0x01;
	const uint8_t command_ec_24_normal_flag = 0x00;

	uint8_t command_ec_128_raw_baseline_flag = 0x02;
	uint8_t command_ec_128_raw_bank_flag = 0x03;

	uint8_t command_91h[2] = {0x91, 0x00};
	uint8_t command_82h[1] = {0x82};
	uint8_t command_F3h[2] = {0xF3, 0x00};
	uint8_t command_83h[1] = {0x83};
	uint8_t receive[1];

	if (IC_TYPE != HX_85XX_D_SERIES_PWON)
	{
		command_ec_128_raw_baseline_flag = 0x02 | command_ec_128_raw_flag;
	}
	else
	{
		command_ec_128_raw_baseline_flag = 0x02;
		command_ec_128_raw_bank_flag = 0x03;
	}

	if (buf[0] == '1') //IIR
	{
		command_91h[1] = command_ec_128_raw_baseline_flag; //A:0x03 , D:0x02
		himax_i2c_write_data(i2c_client, command_91h[0], 1, &(command_91h[1]));
		diag_command = buf[0] - '0';
		printk(KERN_ERR "[Himax]diag_command=0x%x\n",diag_command);
	}
	else if (buf[0] == '2')	//DC
	{
		command_91h[1] = command_ec_128_raw_flag;	//0x01
		himax_i2c_write_data(i2c_client, command_91h[0], 1, &(command_91h[1]));
		diag_command = buf[0] - '0';
		printk(KERN_ERR "[Himax]diag_command=0x%x\n",diag_command);
	}
	else if (buf[0] == '3')	//BANK
	{
		if (IC_TYPE != HX_85XX_D_SERIES_PWON)
		{
			himax_i2c_write_data(i2c_client, command_82h[0], 0, &(command_82h[0]));
			msleep(50);

			himax_i2c_read_data(i2c_client, command_F3h[0], 1, &(receive[0]));
			command_F3h[1] = (receive[0] | 0x80);

			himax_i2c_write_data(i2c_client, command_F3h[0], 1, &(command_F3h[1]));

			command_91h[1] = command_ec_128_raw_baseline_flag;
			himax_i2c_write_data(i2c_client, command_91h[0], 1, &(command_91h[0]));	

			himax_i2c_write_data(i2c_client, command_83h[0], 0, &(command_83h[0]));
			msleep(50);
		}
		else
		{
			command_91h[1] = command_ec_128_raw_bank_flag;	//0x03
			himax_i2c_write_data(i2c_client, command_91h[0], 1, &(command_91h[1]));
		}
		diag_command = buf[0] - '0';
		printk(KERN_ERR "[Himax]diag_command=0x%x\n",diag_command);
	}
	else if (buf[0] == '7')
	{
		diag_command = buf[0] - '0';
	}
	//coordinate dump start

	else if (buf[0] == '8')
	{
		diag_command = buf[0] - '0';

		coordinate_fn = filp_open(DIAG_COORDINATE_FILE,O_CREAT | O_WRONLY | O_APPEND | O_TRUNC,0666);
		if(IS_ERR(coordinate_fn))
		{
			printk(KERN_INFO "[HIMAX TP ERROR]%s: coordinate_dump_file_create error\n", __func__);
			coordinate_dump_enable = 0;
			filp_close(coordinate_fn,NULL);
		}
		coordinate_dump_enable = 1;
	}
	else if (buf[0] == '9') 
	{
		coordinate_dump_enable = 0;
		diag_command = buf[0] - '0';

		if(!IS_ERR(coordinate_fn))
		{
			filp_close(coordinate_fn,NULL);
		}
	}
	//coordinate dump end
	else
	{
		if (IC_TYPE != HX_85XX_D_SERIES_PWON)
		{
			himax_i2c_write_data(i2c_client, command_82h[0], 0, &(command_82h[0]));
			msleep(50);

			command_91h[1] = command_ec_24_normal_flag;
			himax_i2c_write_data(i2c_client, command_91h[0], 1, &(command_91h[1]));			
			himax_i2c_read_data(i2c_client, command_F3h[0], 1, &(receive[0]));
			command_F3h[1] = (receive[0] & 0x7F);
			himax_i2c_write_data(i2c_client, command_F3h[0], 1, &(command_F3h[1]));
			himax_i2c_write_data(i2c_client, command_83h[0], 0, &(command_83h[0]));
		}
		else
		{
			command_91h[1] = command_ec_24_normal_flag;
			himax_i2c_write_data(i2c_client, command_91h[0], 1, &(command_91h[1]));
		}
		diag_command = 0;
		printk(KERN_ERR "[Himax]diag_command=0x%x\n",diag_command);
	}
	return count;
}
static DEVICE_ATTR(diag, 0644, himax_diag_show, himax_diag_dump);
#endif 
//----[HX_TP_SYS_DIAG]--------------------------------------------------------------------------------------end

//----[HX_TP_SYS_FLASH_DUMP]----------------------------------------------------------------------------start
#ifdef HX_TP_SYS_FLASH_DUMP

static uint8_t getFlashCommand(void)
{
	return flash_command;
}

static uint8_t getFlashDumpProgress(void)
{
	return flash_progress;
}

static uint8_t getFlashDumpComplete(void)
{
	return flash_dump_complete;
}

static uint8_t getFlashDumpFail(void)
{
	return flash_dump_fail;
}

static uint8_t getSysOperation(void)
{
	return sys_operation;
}

static uint8_t getFlashReadStep(void)
{
	return flash_read_step;
}

static uint8_t getFlashDumpSector(void)
{
	return flash_dump_sector;
}

static uint8_t getFlashDumpPage(void)
{
	return flash_dump_page;
}

static bool getFlashDumpGoing(void)
{
	return flash_dump_going;
}

static void setFlashBuffer(void)
{
	int i=0;
	flash_buffer = kzalloc(32768*sizeof(uint8_t), GFP_KERNEL);
	for(i=0; i<32768; i++)
	{
		flash_buffer[i] = 0x00;
	}
}

static void setSysOperation(uint8_t operation)
{
	sys_operation = operation;
}

static void setFlashDumpProgress(uint8_t progress)
{
	flash_progress = progress;
	printk("TPPPP setFlashDumpProgress : progress = %d ,flash_progress = %d \n",progress,flash_progress);
}

static void setFlashDumpComplete(uint8_t status)
{
	flash_dump_complete = status;
}

static void setFlashDumpFail(uint8_t fail)
{
	flash_dump_fail = fail;
}

static void setFlashCommand(uint8_t command)
{
	flash_command = command;
}

static void setFlashReadStep(uint8_t step)
{
	flash_read_step = step;
}

static void setFlashDumpSector(uint8_t sector)
{
	flash_dump_sector = sector;
}

static void setFlashDumpPage(uint8_t page)
{
	flash_dump_page = page;
}		

static void setFlashDumpGoing(bool going)
{
	flash_dump_going = going;
}

static ssize_t himax_flash_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int loop_i;
	uint8_t local_flash_read_step =0;
	uint8_t local_flash_complete = 0;
	uint8_t local_flash_progress = 0;
	uint8_t local_flash_command = 0;
	uint8_t local_flash_fail = 0;

	local_flash_complete = getFlashDumpComplete();
	local_flash_progress = getFlashDumpProgress();
	local_flash_command = getFlashCommand();
	local_flash_fail = getFlashDumpFail();

	printk("TPPPP flash_progress = %d \n",local_flash_progress);

	if(local_flash_fail)
	{
		ret += sprintf(buf+ret, "FlashStart:Fail \n");
		ret += sprintf(buf + ret, "FlashEnd");
		ret += sprintf(buf + ret, "\n");
		return ret;
	}

	if(!local_flash_complete)
	{
		ret += sprintf(buf+ret, "FlashStart:Ongoing:0x%2.2x \n",flash_progress);
		ret += sprintf(buf + ret, "FlashEnd");
		ret += sprintf(buf + ret, "\n");
		return ret;
	}

	if(local_flash_command == 1 && local_flash_complete)
	{
		ret += sprintf(buf+ret, "FlashStart:Complete \n");
		ret += sprintf(buf + ret, "FlashEnd");
		ret += sprintf(buf + ret, "\n");
		return ret;
	}

	if(local_flash_command == 3 && local_flash_complete)
	{
		ret += sprintf(buf+ret, "FlashStart: \n");
		for(loop_i = 0; loop_i < 128; loop_i++)
		{
			ret += sprintf(buf + ret, "x%2.2x", flash_buffer[loop_i]);
			if((loop_i % 16) == 15)
			{
				ret += sprintf(buf + ret, "\n");
			}
		}
		ret += sprintf(buf + ret, "FlashEnd");
		ret += sprintf(buf + ret, "\n");
		return ret;
	}

	//flash command == 0 , report the data
	local_flash_read_step = getFlashReadStep();

	ret += sprintf(buf+ret, "FlashStart:%2.2x \n",local_flash_read_step);

	for (loop_i = 0; loop_i < 1024; loop_i++) 
	{
		ret += sprintf(buf + ret, "x%2.2X", flash_buffer[local_flash_read_step*1024 + loop_i]);

		if ((loop_i % 16) == 15)
		{
			ret += sprintf(buf + ret, "\n");
		}
	}

	ret += sprintf(buf + ret, "FlashEnd");
	ret += sprintf(buf + ret, "\n");
	return ret;
}

//-----------------------------------------------------------------------------------
//himax_flash_store
//
//command 0 : Read the page by step number
//command 1 : driver start to dump flash data, save it to mem
//command 2 : driver start to dump flash data, save it to sdcard/Flash_Dump.bin
//
//-----------------------------------------------------------------------------------
static ssize_t himax_flash_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	char buf_tmp[6];
	unsigned long result = 0;
	uint8_t loop_i = 0;
	int base = 0;

	memset(buf_tmp, 0x0, sizeof(buf_tmp));

	printk(KERN_INFO "[TP] %s: buf[0] = %s\n", __func__, buf);

	if(getSysOperation() == 1)
	{
		printk("[TP] %s: SYS is busy , return!\n", __func__);
		return count;
	}

	if(buf[0] == '0')
	{
		setFlashCommand(0);
		if(buf[1] == ':' && buf[2] == 'x')
		{
			memcpy(buf_tmp, buf + 3, 2);
			printk(KERN_INFO "[TP] %s: read_Step = %s\n", __func__, buf_tmp);
			if (!strict_strtoul(buf_tmp, 16, &result))
			{
				printk("[TP] %s: read_Step = %lu \n", __func__, result);
				setFlashReadStep(result);
			}
		}
	} 
	else if(buf[0] == '1')
	{
		setSysOperation(1);
		setFlashCommand(1);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);
		queue_work(flash_wq, &flash_work);
	}
	else if(buf[0] == '2')
	{
		setSysOperation(1);
		setFlashCommand(2);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		queue_work(flash_wq, &flash_work);
	}
	else if(buf[0] == '3')
	{
		setSysOperation(1);
		setFlashCommand(3);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		memcpy(buf_tmp, buf + 3, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{

			setFlashDumpSector(result);
		}
		
		memcpy(buf_tmp, buf + 7, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{
			setFlashDumpPage(result);
		}

		queue_work(flash_wq, &flash_work);
	}
	else if(buf[0] == '4')
	{
		printk(KERN_INFO "[TP] %s: command 4 enter.\n", __func__);
		setSysOperation(1);
		setFlashCommand(4);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);
		
		memcpy(buf_tmp, buf + 3, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{
			setFlashDumpSector(result);
		}
		else
		{
			printk(KERN_INFO "[TP] %s: command 4 , sector error.\n", __func__);
			return count;
		}

		memcpy(buf_tmp, buf + 7, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{
			setFlashDumpPage(result);
		}
		else
		{
			printk(KERN_INFO "[TP] %s: command 4 , page error.\n", __func__);
			return count;

		}

		base = 11;

		printk(KERN_INFO "=========Himax flash page buffer start=========\n");
		for(loop_i=0;loop_i<128;loop_i++)
		{
			memcpy(buf_tmp, buf + base, 2);
			if (!strict_strtoul(buf_tmp, 16, &result))
			{
				flash_buffer[loop_i] = result;
				printk(" %d ",flash_buffer[loop_i]);
				if(loop_i % 16 == 15)
				{
					printk("\n");
				}
			}
			base += 3;
		}
		printk(KERN_INFO "=========Himax flash page buffer end=========\n");

		queue_work(flash_wq, &flash_work);		
	}
	return count;
}
static DEVICE_ATTR(flash_dump, 0644, himax_flash_show, himax_flash_store);
	
static void himax_ts_flash_work_func(struct work_struct *work)
{
	uint8_t page_tmp[128];
	uint8_t x59_tmp[4] = {0,0,0,0};
	int i=0, j=0, k=0, l=0,/* j_limit = 0,*/ buffer_ptr = 0, flash_end_count = 0;
	uint8_t local_flash_command = 0;
	uint8_t sector = 0;
	uint8_t page = 0;
	
	uint8_t x81_command[2] = {0x81,0x00};
	uint8_t x82_command[2] = {0x82,0x00};
	uint8_t x42_command[2] = {0x42,0x00};
	uint8_t x43_command[4] = {0x43,0x00,0x00,0x00};
	uint8_t x44_command[4] = {0x44,0x00,0x00,0x00};
	uint8_t x45_command[5] = {0x45,0x00,0x00,0x00,0x00};
	uint8_t x46_command[2] = {0x46,0x00};
	uint8_t x4A_command[2] = {0x4A,0x00};
	uint8_t x4D_command[2] = {0x4D,0x00};
	/*uint8_t x59_command[2] = {0x59,0x00};*/

	mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
	setFlashDumpGoing(true);	

	#ifdef HX_RST_PIN_FUNC
	himax_HW_reset();
	#endif

	sector = getFlashDumpSector();
	page   = getFlashDumpPage();
	
	local_flash_command = getFlashCommand();
	
	if( himax_i2c_write_data(i2c_client, x81_command[0], 0, &(x81_command[1])) < 0 )
	{
		printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 81 fail.\n",__func__);
		goto Flash_Dump_i2c_transfer_error;
	}
	msleep(120);

	if( himax_i2c_write_data(i2c_client, x82_command[0], 0, &(x82_command[1])) < 0 )
	{
		printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 82 fail.\n",__func__);
		goto Flash_Dump_i2c_transfer_error;
	}
	msleep(100);

	printk(KERN_INFO "[TP] %s: local_flash_command = %d enter.\n", __func__,local_flash_command);
	printk(KERN_INFO "[TP] %s: flash buffer start.\n", __func__);
	for(i=0;i<128;i++)
	{
		printk(KERN_INFO " %2.2x ",flash_buffer[i]);
		if((i%16) == 15)
		{
			printk("\n");
		}
	}
	printk(KERN_INFO "[TP] %s: flash buffer end.\n", __func__);

	if(local_flash_command == 1 || local_flash_command == 2)
	{
		x43_command[1] = 0x01;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 1, &(x43_command[1])) < 0 )
		{
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(100);

		for( i=0 ; i<8 ;i++)
		{
			for(j=0 ; j<32 ; j++)
			{
				printk("TPPPP Step 2 i=%d , j=%d %s\n",i,j,__func__);
				//read page start
				for(k=0; k<128; k++)
				{
					page_tmp[k] = 0x00;
				}
				for(k=0; k<32; k++)
				{
					x44_command[1] = k;
					x44_command[2] = j;
					x44_command[3] = i;
					if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
					{
						printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 44 fail.\n",__func__);
						goto Flash_Dump_i2c_transfer_error;
					}

					if( himax_i2c_write_data(i2c_client, x46_command[0], 0, &(x46_command[1])) < 0 )
					{
						printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 46 fail.\n",__func__);
						goto Flash_Dump_i2c_transfer_error;
					}
					//msleep(2);
					if( himax_i2c_read_data(i2c_client, 0x59, 4, &(x59_tmp[0])) < 0 )
					{
						printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 59 fail.\n",__func__);
						goto Flash_Dump_i2c_transfer_error;
					}
					//msleep(2);
					for(l=0; l<4; l++)
					{
						page_tmp[k*4+l] = x59_tmp[l];
					}
					//msleep(10);
				}
				//read page end 

				for(k=0; k<128; k++)
				{
					flash_buffer[buffer_ptr++] = page_tmp[k];
					
					if(page_tmp[k] == 0xFF)
					{
						flash_end_count ++;
						if(flash_end_count == 32)
						{
							flash_end_count = 0;
							buffer_ptr = buffer_ptr -32;
							goto FLASH_END;
						}
					}
					else
					{
						flash_end_count = 0;
					}
				}
				setFlashDumpProgress(i*32 + j);
			}
		}
	}
	else if(local_flash_command == 3)
	{
		x43_command[1] = 0x01;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 1, &(x43_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(100);

		for(i=0; i<128; i++)
		{
			page_tmp[i] = 0x00;
		}

		for(i=0; i<32; i++)
		{
			x44_command[1] = i;
			x44_command[2] = page;
			x44_command[3] = sector;
			
			if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
			{
				printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 44 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			
			if( himax_i2c_write_data(i2c_client, x46_command[0], 0, &(x46_command[1])) < 0 )
			{
				printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 46 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			//msleep(2);
			if( himax_i2c_read_data(i2c_client, 0x59, 4, &(x59_tmp[0])) < 0 )
			{
				printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 59 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			//msleep(2);
			for(j=0; j<4; j++)
			{
				page_tmp[i*4+j] = x59_tmp[j];
			}
			//msleep(10);
		}
		//read page end
		for(i=0; i<128; i++)
		{
			flash_buffer[buffer_ptr++] = page_tmp[i];
		}
	}
	else if(local_flash_command == 4)
	{
		//page write flow.
		printk(KERN_INFO "[TP] %s: local_flash_command = 4, enter.\n", __func__);

		//-----------------------------------------------------------------------------------------------
		// unlock flash
		//-----------------------------------------------------------------------------------------------
		x43_command[1] = 0x01;
		x43_command[2] = 0x00;
		x43_command[3] = 0x06;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 3, &(x43_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x44_command[1] = 0x03;
		x44_command[2] = 0x00;
		x44_command[3] = 0x00;
		if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x45_command[1] = 0x00;
		x45_command[2] = 0x00;
		x45_command[3] = 0x3D;
		x45_command[4] = 0x03;
		if( himax_i2c_write_data(i2c_client, x45_command[0], 4, &(x45_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 45 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		if( himax_i2c_write_data(i2c_client, x4A_command[0], 0, &(x4A_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 4A fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(50);

		//-----------------------------------------------------------------------------------------------
		// page erase
		//-----------------------------------------------------------------------------------------------
		x43_command[1] = 0x01;
		x43_command[2] = 0x00;
		x43_command[3] = 0x02;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 3, &(x43_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x44_command[1] = 0x00;
		x44_command[2] = page;
		x44_command[3] = sector;
		if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		if( himax_i2c_write_data(i2c_client, x4D_command[0], 0, &(x4D_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 4D fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(100);

		//-----------------------------------------------------------------------------------------------



		// enter manual mode
		//-----------------------------------------------------------------------------------------------
		x42_command[1] = 0x01;
		if( himax_i2c_write_data(i2c_client, x42_command[0], 1, &(x42_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 42 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(100);

		//-----------------------------------------------------------------------------------------------
		// flash enable
		//-----------------------------------------------------------------------------------------------
		x43_command[1] = 0x01;
		x43_command[2] = 0x00;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		//-----------------------------------------------------------------------------------------------
		// set flash address
		//-----------------------------------------------------------------------------------------------
		x44_command[1] = 0x00;
		x44_command[2] = page;
		x44_command[3] = sector;
		if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		//-----------------------------------------------------------------------------------------------
		// manual mode command : 47 to latch the flash address when page address change.
		//-----------------------------------------------------------------------------------------------
		x43_command[1] = 0x01;
		x43_command[2] = 0x09;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x0D;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x09;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		for(i=0; i<32; i++)
		{
			printk(KERN_INFO "himax :i=%d \n",i);
			x44_command[1] = i; 
			x44_command[2] = page; 
			x44_command[3] = sector;
			if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
			{
				printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 44 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(10);

			x45_command[1] = flash_buffer[i*4 + 0];
			x45_command[2] = flash_buffer[i*4 + 1];
			x45_command[3] = flash_buffer[i*4 + 2];
			x45_command[4] = flash_buffer[i*4 + 3];
			if( himax_i2c_write_data(i2c_client, x45_command[0], 4, &(x45_command[1])) < 0 )
			{
				printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 45 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(10);

			//-----------------------------------------------------------------------------------------------
			// manual mode command : 48 ,data will be written into flash buffer
			//-----------------------------------------------------------------------------------------------
			x43_command[1] = 0x01;


			x43_command[2] = 0x0D;
			if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
			{
				printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(10);

			x43_command[1] = 0x01;
			x43_command[2] = 0x09;
			if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
			{
				printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(10);
		}

		//-----------------------------------------------------------------------------------------------
		// manual mode command : 49 ,program data from flash buffer to this page
		//-----------------------------------------------------------------------------------------------
		x43_command[1] = 0x01;
		x43_command[2] = 0x01;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x05;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x01;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x00;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		//-----------------------------------------------------------------------------------------------
		// flash disable
		//-----------------------------------------------------------------------------------------------
		x43_command[1] = 0x00;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 1, &(x43_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		//-----------------------------------------------------------------------------------------------
		// leave manual mode
		//-----------------------------------------------------------------------------------------------
		x42_command[1] = 0x00;
		if( himax_i2c_write_data(i2c_client, x42_command[0], 1, &(x42_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		//-----------------------------------------------------------------------------------------------
		// lock flash
		//-----------------------------------------------------------------------------------------------
		x43_command[1] = 0x01; 
		x43_command[2] = 0x00; 
		x43_command[3] = 0x06;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 3, &(x43_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x44_command[1] = 0x03;
		x44_command[2] = 0x00;
		x44_command[3] = 0x00;
		if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x45_command[1] = 0x00;
		x45_command[2] = 0x00;
		x45_command[3] = 0x7D;
		x45_command[4] = 0x03;
		if( himax_i2c_write_data(i2c_client, x45_command[0], 4, &(x45_command[1])) < 0 )
		{   
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 45 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}   
		msleep(10);

		if( himax_i2c_write_data(i2c_client, x4A_command[0], 0, &(x4A_command[1])) < 0 )
		{
			printk(KERN_ERR "[TP]TOUCH_ERR: %s i2c write 4D fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		msleep(50);

		buffer_ptr = 128;
		printk(KERN_INFO "Himax: Flash page write Complete~~~~~~~~~~~~~~~~~~~~~~~\n");
	}


FLASH_END:

	printk("Complete~~~~~~~~~~~~~~~~~~~~~~~\n");

	printk(" buffer_ptr = %d \n",buffer_ptr);

	for (i = 0; i < buffer_ptr; i++) 
	{
		printk("%2.2X ", flash_buffer[i]);
	
		if ((i % 16) == 15)
		{
			printk("\n");
		}
	}
	printk("End~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	himax_i2c_write_data(i2c_client, x43_command[0], 0, &(x43_command[1]));
	msleep(50);

	if(local_flash_command == 2)
	{
		struct file *fn;
		
		fn = filp_open(FLASH_DUMP_FILE,O_CREAT | O_WRONLY ,0);
		if(!IS_ERR(fn))
		{
			fn->f_op->write(fn,flash_buffer,buffer_ptr*sizeof(uint8_t),&fn->f_pos);
			filp_close(fn,NULL);
		}
	}

	

	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	setFlashDumpGoing(false);

	setFlashDumpComplete(1);
	setSysOperation(0);
	return;

Flash_Dump_i2c_transfer_error:
		
	

	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	setFlashDumpGoing(false);
	setFlashDumpComplete(0);
	setFlashDumpFail(1);
	setSysOperation(0);
	return;
}
#endif
//----[HX_TP_SYS_FLASH_DUMP]------------------------------------------------------------------------------end

//----[HX_TP_SYS_SELF_TEST]-----------------------------------------------------------------------------start
#ifdef HX_TP_SYS_SELF_TEST
static ssize_t himax_chip_self_test_function(struct device *dev, struct device_attribute *attr, char *buf)
{
	int val=0x00;
	val = himax_chip_self_test();
	return sprintf(buf, "%d\n",val);
}

static int himax_chip_self_test(void)
{
	uint8_t cmdbuf[11];
	int ret = 0;
	uint8_t valuebuf[16];
	int i=0, pf_value=0x00;

	//----[HX_RST_PIN_FUNC]-----------------------------------------------------------------------------start
	#ifdef HX_RST_PIN_FUNC
	himax_HW_reset();
	himax_ts_poweron();
	#endif
	//----[HX_RST_PIN_FUNC]-------------------------------------------------------------------------------end

	if (IC_TYPE == HX_85XX_C_SERIES_PWON)
	{
		//sense off to write self-test parameters
		cmdbuf[0] = 0x82;
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 0, &(cmdbuf[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "[Himax]:write TSSOFF failed line: %d \n",__LINE__);
		} 
		mdelay(120); //120ms
		
		cmdbuf[0] = 0x8D;
		cmdbuf[1] = 0xB4; //180
		cmdbuf[2] = 0x64; //100
		cmdbuf[3] = 0x36;
		cmdbuf[4] = 0x09;
		cmdbuf[5] = 0x2D;
		cmdbuf[6] = 0x09;
		cmdbuf[7] = 0x32;
		cmdbuf[8] = 0x08; //0x19
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 8, &(cmdbuf[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "[Himax]:write HX_CMD_SELFTEST_BUFFER failed line: %d \n",__LINE__);
		}
  	
		udelay(100);
  	
		ret = himax_i2c_read_data(i2c_client, 0x8D, 9, &(valuebuf[0]));
		if(ret < 0) 
		{
			printk(KERN_ERR "[Himax]:read HX_CMD_SELFTEST_BUFFER failed line: %d \n",__LINE__);
		}
		printk("[Himax]:0x8D[0] = 0x%x\n",valuebuf[0]);
		printk("[Himax]:0x8D[1] = 0x%x\n",valuebuf[1]);
		printk("[Himax]:0x8D[2] = 0x%x\n",valuebuf[2]);
		printk("[Himax]:0x8D[3] = 0x%x\n",valuebuf[3]);
		printk("[Himax]:0x8D[4] = 0x%x\n",valuebuf[4]);
		printk("[Himax]:0x8D[5] = 0x%x\n",valuebuf[5]);
		printk("[Himax]:0x8D[6] = 0x%x\n",valuebuf[6]);
		printk("[Himax]:0x8D[7] = 0x%x\n",valuebuf[7]);
		printk("[Himax]:0x8D[8] = 0x%x\n",valuebuf[8]);
  	
		cmdbuf[0] = 0xE9;
		cmdbuf[1] = 0x00;
		cmdbuf[2] = 0x06;
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 2, &(cmdbuf[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "[Himax]:write sernsor to self-test mode failed line: %d \n",__LINE__);
		} 
		udelay(100);
  	
		ret = himax_i2c_read_data(i2c_client, 0xE9, 3, &(valuebuf[0]));
		if(ret < 0) 
		{
			printk(KERN_ERR "[Himax]:read 0xE9 failed line: %d \n",__LINE__);
		}
		printk("[Himax]:0xE9[0] = 0x%x\n",valuebuf[0]);
		printk("[Himax]:0xE9[1] = 0x%x\n",valuebuf[1]);
		printk("[Himax]:0xE9[2] = 0x%x\n",valuebuf[2]);
  	
		cmdbuf[0] = 0x83;
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 0, &(cmdbuf[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "[Himax]:write HX_CMD_TSSON failed line: %d \n",__LINE__);
		} 
		mdelay(1500); //1500ms
  	
		cmdbuf[0] = 0x82;
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 0, &(cmdbuf[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "[Himax]:write TSSOFF failed line: %d \n",__LINE__);
		} 

		mdelay(120); //120ms
  	
		memset(valuebuf, 0x00 , 16);
		ret = himax_i2c_read_data(i2c_client, 0x8D, 16, &(valuebuf[0]));
		if(ret < 0) 
		{
			printk(KERN_ERR "[Himax]:read HX_CMD_FW_VERSION_ID failed line: %d \n",__LINE__);
		}
		else
		{
			if(valuebuf[0]==0xAA)
			{
				printk("[Himax]: self-test pass\n");
				pf_value = 0x0;
			}
			else
			{
				printk("[Himax]: self-test fail\n");
				pf_value = 0x1;
				for(i=0;i<16;i++)
				{
					printk("[Himax]:0x8D buff[%d] = 0x%x\n",i,valuebuf[i]);
				}
			}
		}
		mdelay(120); //120ms
  	
		cmdbuf[0] = 0xE9;
		cmdbuf[1] = 0x00;
		cmdbuf[2] = 0x00;
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 2, &(cmdbuf[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "[Himax]:write sensor to normal mode failed line: %d \n",__LINE__);
		} 
		mdelay(120); //120ms
  	
		cmdbuf[0] = 0x83;
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 0, &(cmdbuf[1]));
		if(ret < 0) 
		{
			printk(KERN_ERR "[Himax]:write HX_CMD_TSSON failed line: %d \n",__LINE__);
		}
		msleep(120); //120ms
	}
	else if(IC_TYPE == HX_85XX_D_SERIES_PWON)
	{
		//Step 0 : sensor off
		himax_i2c_write_data(i2c_client, 0x82, 0, &(cmdbuf[0]));
		msleep(120);
		
		//Step 1 : Close Re-Calibration FE02
		//-->Read 0xFE02
		cmdbuf[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));		
		msleep(10);
		
		cmdbuf[0] = 0x00;
		cmdbuf[1] = 0x02; //FE02 
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(cmdbuf[0]));
		msleep(10);
		
		himax_i2c_read_data(i2c_client, 0x5A, 2, &(valuebuf[0]));
		msleep(10);
		
		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		
		msleep(30);
		
		printk("[Himax]:0xFE02_0 = 0x%x\n",valuebuf[0]);
		printk("[Himax]:0xFE02_1 = 0x%x\n",valuebuf[1]);
		
		valuebuf[0] = valuebuf[1] & 0xFD; // close re-calibration  , shift first byte of config bank register read issue.
		
		printk("[Himax]:0xFE02_valuebuf = 0x%x\n",valuebuf[0]);
		
		//-->Write 0xFE02
		cmdbuf[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		msleep(10);
		
		cmdbuf[0] = 0x00;
		cmdbuf[1] = 0x02; //FE02 
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(cmdbuf[0]));
		msleep(10);
		
		cmdbuf[0] = valuebuf[0];
		himax_i2c_write_data(i2c_client, 0x40, 1, &(cmdbuf[0]));

		msleep(10);
		
		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		
		msleep(30);
		//0xFE02 Read Back
		
		//-->Read 0xFE02
		cmdbuf[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));	
		msleep(10);
		
		cmdbuf[0] = 0x00;
		cmdbuf[1] = 0x02; //FE02 
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(cmdbuf[0]));
		msleep(10);
		
		himax_i2c_read_data(i2c_client, 0x5A, 2, &(valuebuf[0]));
		msleep(10);
		
		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		msleep(30);
		
		printk("[Himax]:0xFE02_0_back = 0x%x\n",valuebuf[0]);
		printk("[Himax]:0xFE02_1_back = 0x%x\n",valuebuf[1]);
		
		//Step 2 : Close Flash-Reload
		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE3, 1, &(cmdbuf[0]));
		
		msleep(30);
		
		himax_i2c_read_data(i2c_client, 0xE3, 1, &(valuebuf[0]));

		printk("[Himax]:0xE3_back = 0x%x\n",valuebuf[0]);
		
		//Step 4 : Write self_test parameter to FE96~FE9D
		//-->Write FE96~FE9D
		cmdbuf[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		msleep(10);
		
		cmdbuf[0] = 0x00;
		cmdbuf[1] = 0x96; //FE96 
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(cmdbuf[0]));
		msleep(10);
		
		//-->Modify the initial value of self_test.
		cmdbuf[0] = 0xB4; 
		cmdbuf[1] = 0x64; 
		cmdbuf[2] = 0x3F; 
		cmdbuf[3] = 0x3F; 
		cmdbuf[4] = 0x3C; 
		cmdbuf[5] = 0x00; 
		cmdbuf[6] = 0x3C; 
		cmdbuf[7] = 0x00; 
		himax_i2c_write_data(i2c_client, 0x40, 8, &(cmdbuf[0]));
		msleep(10);
		
		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		
		msleep(30);
		
		//Read back
		cmdbuf[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		msleep(10);
		
		cmdbuf[0] = 0x00;
		cmdbuf[1] = 0x96; //FE96
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(cmdbuf[0]));
		msleep(10);
		
		himax_i2c_read_data(i2c_client, 0x5A, 16, &(valuebuf[0]));
		msleep(10);
		
		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		
		for(i=1;i<16;i++)
		{
			printk("[Himax]:0xFE96 buff_back[%d] = 0x%x\n",i,valuebuf[i]);
		}
		
		msleep(30);
		
		//Step 5 : Enter self_test mode
		cmdbuf[0] = 0x16;
		himax_i2c_write_data(i2c_client, 0x91, 1, &(cmdbuf[0]));
		
		himax_i2c_read_data(i2c_client, 0x91, 1, &(valuebuf[0]));
		
		printk("[Himax]:0x91_back = 0x%x\n",valuebuf[0]);
		msleep(10);
		
		//Step 6 : Sensor On
		himax_i2c_write_data(i2c_client, 0x83, 0, &(cmdbuf[0]));
		
		mdelay(2000);
		
		//Step 7 : Sensor Off
		himax_i2c_write_data(i2c_client, 0x82, 0, &(cmdbuf[0]));
		
		msleep(30);
		
		//Step 8 : Get self_test result
		cmdbuf[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		msleep(10);
		
		cmdbuf[0] = 0x00;
		cmdbuf[1] = 0x96; //FE96 
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(cmdbuf[0]));
		msleep(10);
		
		himax_i2c_read_data(i2c_client, 0x5A, 16, &(valuebuf[0]));
		msleep(10);
		
		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		
		//Final : Leave self_test mode
		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0x91, 1, &(cmdbuf[0]));
		
		if(valuebuf[1]==0xAA) //get the self_test result , shift first byte for config bank read issue.
		{
			printk("[Himax]: self-test pass\n");
			pf_value = 0x0;
		}
		else
		{
			printk("[Himax]: self-test fail\n");
			pf_value = 0x1;
			for(i=1;i<16;i++)
			{
				printk("[Himax]:0xFE96 buff[%d] = 0x%x\n",i,valuebuf[i]);
			}
		}
		
		//HW reset and power on again.
		//----[HX_RST_PIN_FUNC]-----------------------------------------------------------------------------start
			#ifdef HX_RST_PIN_FUNC
			himax_HW_reset();
			#endif
		//----[HX_RST_PIN_FUNC]-------------------------------------------------------------------------------end
		
		himax_ts_poweron();
	}
	return pf_value;
}

static DEVICE_ATTR(tp_self_test, (S_IWUSR|S_IRUGO), himax_chip_self_test_function, NULL);
#endif
//----[HX_TP_SYS_SELF_TEST]-------------------------------------------------------------------------------end

//----[HX_TP_SYS_RESET]---------------------------------------------------------------------------------start
#ifdef HX_TP_SYS_RESET
static ssize_t himax_reset_set(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	
	return count;
}

static DEVICE_ATTR(reset, (S_IWUSR|S_IRUGO),NULL, himax_reset_set);
#endif
//----[HX_TP_SYS_RESET]----------------------------------------------------------------------------------end	

static int himax_touch_sysfs_init(void)
{
	int ret;
	android_touch_kobj = kobject_create_and_add("android_touch", NULL);
	if (android_touch_kobj == NULL) 
	{
		TPD_DMESG(KERN_ERR "[Himax]: subsystem_register failed\n");
		ret = -ENOMEM;
		return ret;
	}

	//----[HX_TP_SYS_DEBUG_LEVEL]---------------------------------------------------------------------------start
	#ifdef HX_TP_SYS_DEBUG_LEVEL
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_debug_level.attr);
	if (ret) 
	{
		TPD_DMESG(KERN_ERR "[Himax]: create_file debug_level failed\n");
		return ret;
	}
	#endif
	//----[HX_TP_SYS_DEBUG_LEVEL]-----------------------------------------------------------------------------end

	//----[HX_TP_SYS_REGISTER]------------------------------------------------------------------------------start
	#ifdef HX_TP_SYS_REGISTER
	register_command = 0;
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_register.attr);
	if (ret) 
	{
		TPD_DMESG(KERN_ERR "[Himax]: create_file register failed\n");
		return ret;
	}
	#endif
	//----[HX_TP_SYS_REGISTER]--------------------------------------------------------------------------------end

	//----[HX_TP_SYS_DIAG]----------------------------------------------------------------------------------start
	#ifdef HX_TP_SYS_DIAG
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_diag.attr);
	if (ret) 
	{
		TPD_DMESG(KERN_ERR "[Himax]: sysfs_create_file failed\n");
		return ret;
	}
	#endif
	//----[HX_TP_SYS_DIAG]------------------------------------------------------------------------------------end

	//----[HX_TP_SYS_FLASH_DUMP]----------------------------------------------------------------------------start
	#ifdef HX_TP_SYS_FLASH_DUMP
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_flash_dump.attr);
	if (ret) 
	{
		printk(KERN_ERR "[TP]TOUCH_ERR: sysfs_create_file failed\n");
		return ret;
	}
	#endif
	//----[HX_TP_SYS_FLASH_DUMP]------------------------------------------------------------------------------end

	//----[HX_TP_SYS_SELF_TEST]-----------------------------------------------------------------------------start
	#ifdef HX_TP_SYS_SELF_TEST
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_tp_self_test.attr);
	if (ret) 
	{
		printk(KERN_ERR "[Himax]TOUCH_ERR: sysfs_create_file dev_attr_tp_self_test failed\n");
		return ret;
	}
	#endif
	//----[HX_TP_SYS_SELF_TEST]-------------------------------------------------------------------------------end

	//----[HX_TP_SYS_SELF_TEST]-------------------------------------------------------------------------------end
	#ifdef HX_TP_SYS_RESET
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_reset.attr);
	if (ret)
	{
		printk(KERN_ERR "[TP]TOUCH_ERR: sysfs_create_file failed\n");
		return ret;	
	}
	#endif
	//----[HX_TP_SYS_SELF_TEST]-------------------------------------------------------------------------------end
		
	return 0 ;
}

static void himax_touch_sysfs_deinit(void)
{
	//----[HX_TP_SYS_DEBUG_LEVEL]---------------------------------------------------------------------------start
	#ifdef HX_TP_SYS_DEBUG_LEVEL
	sysfs_remove_file(android_touch_kobj, &dev_attr_debug_level.attr);
	#endif
	//----[HX_TP_SYS_DEBUG_LEVEL]-----------------------------------------------------------------------------end

	//----[HX_TP_SYS_DIAG]----------------------------------------------------------------------------------start
	#ifdef HX_TP_SYS_DIAG	
	sysfs_remove_file(android_touch_kobj, &dev_attr_diag.attr);
	#endif
	//----[HX_TP_SYS_DIAG]------------------------------------------------------------------------------------end

	//----[HX_TP_SYS_REGISTER]------------------------------------------------------------------------------start
	#ifdef HX_TP_SYS_REGISTER
	sysfs_remove_file(android_touch_kobj, &dev_attr_register.attr);
	#endif
	//----[HX_TP_SYS_REGISTER]--------------------------------------------------------------------------------end

	//----[HX_TP_SYS_SELF_TEST]-----------------------------------------------------------------------------start
	#ifdef HX_TP_SYS_SELF_TEST
	sysfs_remove_file(android_touch_kobj, &dev_attr_tp_self_test.attr);
	#endif
	//----[HX_TP_SYS_SELF_TEST]-------------------------------------------------------------------------------end

	kobject_del(android_touch_kobj);
}

// ---add on 0112----------------------------
int himax_charge_switch(s32 dir_update)
{
    u32 chr_status = 0;
	uint8_t charge_buf[2] = {0};
    u8 chr_cmd[3] = {0x80, 0x40};
    static u8 chr_pluggedin = 0;
    int ret = 0;
    chr_status = upmu_is_chr_det();

    mutex_lock(&i2c_access);//SWU_fail_0731
    
	if (chr_status)

	{       
		if (!chr_pluggedin || dir_update)
       		 {
			
			printk("himax >>>work_incharging\n");
			charge_buf[0] = 0x01;


			himax_i2c_write_data(i2c_client, 0x90, 1, &(charge_buf[0]));
			chr_pluggedin = 1;


			#if 1
			int charge_flag = 0;
			himax_i2c_read_data(i2c_client,0x90,1,&charge_flag);
			printk("himax >> charge_flag = %d .\n",charge_flag);
			#endif
		}		
	}
    else 
    {
		if (chr_pluggedin || dir_update)
        		{
			charge_buf[0] = 0x00;
			himax_i2c_write_data(i2c_client, 0x90, 1, &(charge_buf[0]));
			printk("himax >>>work_outcharging\n");
			chr_pluggedin = 0;
//ningyd add for debug
			#if 1
			int charge_flag = 0;
			himax_i2c_read_data(i2c_client,0x90,1,&charge_flag);
			printk("himax >> charge_flag = %d .\n",charge_flag);
			#endif
		}		
	}
    mutex_unlock(&i2c_access);//SWU_fail_0731
    
	return 0;
}

//---- add on 0112-----------------



#ifdef FTS_PRESSURE
static  void tpd_down(int x, int y, int press, int p)//ckt-chunhui.lin
#else
static  void tpd_down(int x, int y, int p)
#endif
{
#ifdef HX_PORTING_DEB_MSG
    TPD_DMESG("[Himax] tpd_down[%4d %4d]\n ", x, y);
#endif
    input_report_key(tpd->dev, BTN_TOUCH, 1);
#ifdef FTS_PRESSURE
    unsigned int area = press;
    if(area > 31) {
	  area = (area >> 3); 
    }
    input_report_abs(tpd->dev, ABS_MT_PRESSURE, press);//ckt-chunhui.lin
    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, area);//ckt-chunhui.lin
#else
    input_report_abs(tpd->dev, ABS_MT_PRESSURE, 1);//wangli
    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 20);//wangli
#endif
    //input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 1);	
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);

    /* track id Start 0 */
    input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, p);
    input_mt_sync(tpd->dev);
    if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
    {
        tpd_button(x, y, 1);
    }
}

//static  int tpd_up(int x, int y, int *count)//wangli_20140430
static  void tpd_up(int x, int y, int *count)
{
#ifdef HX_PORTING_DEB_MSG
    TPD_DMESG("[Himax] tpd_up[%4d %4d]\n ", x, y);
#endif
    //printk("\nHIMAX **************tpd_up[%4d %4d]***************\n ", x, y);//wangli_20140504
#ifdef FTS_PRESSURE
    input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0); //ckt-chunhui.lin
    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0);
#else
    input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0);//wangli_20140429 
#endif
    input_report_key(tpd->dev, BTN_TOUCH, 0);
    input_mt_sync(tpd->dev);
    if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
    {
        tpd_button(x, y, 0);
    }
}


//=============================================================================================================
//
//	Segment : Touch Work Function
//
//=============================================================================================================
static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo)
{
	int i = 0,temp1, temp2,ret = 0,res=0;
	char data[128] = {0};
	char gesture_flag = '0';
	u8 check_sum_cal = 0;
	u16 high_byte,low_byte;
	int err[4] = {0};
	unsigned int x=0, y=0, area=0, press=0;
	const unsigned int x_res = HX_X_RES;
	const unsigned int y_res = HX_Y_RES;
	unsigned int temp_x[HX_MAX_PT], temp_y[HX_MAX_PT];
	
	int check_FC = 0;

#ifdef TPD_PROXIMITY
	int err_1;
	hwm_sensor_data sensor_data;
	u8 proximity_status;
#endif

#ifdef BUTTON_CHECK
	int button_cancel=0;
	// int k=0;
	int cur_frm_max  = 0;
#endif

#ifdef HX_TP_SYS_DIAG
	uint8_t *mutual_data;
	uint8_t *self_data;
	uint8_t diag_cmd;
	int mul_num; 
	int self_num;
	int index = 0;

	//coordinate dump start
	char coordinate_char[15+(HX_MAX_PT+5)*2*5+2];
	struct timeval t;
	struct tm broken;
	//coordinate dump end
#endif

	//TPD_DMESG("start %s, %d\n", __FUNCTION__, __LINE__);

	int read_len;
	int raw_cnt_max = HX_MAX_PT/4;
	int raw_cnt_rmd = HX_MAX_PT%4;
	int hx_touch_info_size, RawDataLen;
	if(raw_cnt_rmd != 0x00)
	{
		if (IC_TYPE == HX_85XX_D_SERIES_PWON)
		{
			RawDataLen = 128 - ((HX_MAX_PT+raw_cnt_max+3)*4) - 1;
	 	}
		else
		{
			RawDataLen = 128 - ((HX_MAX_PT+raw_cnt_max+3)*4);
		}
		hx_touch_info_size = (HX_MAX_PT+raw_cnt_max+2)*4;
	}
	else
	{
		if (IC_TYPE == HX_85XX_D_SERIES_PWON)
		{
			RawDataLen = 128 - ((HX_MAX_PT+raw_cnt_max+2)*4) - 1;
		}
		else
		{
			RawDataLen = 128 - ((HX_MAX_PT+raw_cnt_max+2)*4);
		}
		hx_touch_info_size = (HX_MAX_PT+raw_cnt_max+1)*4;
	}

//	point_proximity_position=hx_touch_info_size-2;//wangli

#ifdef ENABLE_CHIP_STATUS_MONITOR
	running_status = 1;
	cancel_delayed_work_sync(&ts->himax_chip_monitor);
#endif

#ifdef HX_TP_SYS_DIAG
	if(diag_command) //count the i2c read length
#else
		if(false)
#endif
		{
			read_len = 128; //hx_touch_info_size + RawDataLen + 4 + 1; //4: RawData Header
		}
		else
		{
			read_len =  hx_touch_info_size;
		}

	//Himax: Receive raw data about Coordinates and
	mutex_lock(&i2c_access);
	//	printk("ningyd : tpd_halt = %d in touchinfo\n",tpd_halt);
	if (tpd_halt) // Touch Driver is enter suspend
	{

		//mutex_unlock(&i2c_access);
		//gionee ningyd add for CR01028440 20140125 begin
		if(HX_Gesture==1)
		{
#ifdef HX_PORTING_DEB_MSG
			TPD_DMESG("[Himax] Himax TP: tpd halt but in gesture mode.\n");
#endif
		}
		else
		{
            mutex_unlock(&i2c_access);//SWU_fail_0731
			//gionee ningyd add for CR01028440 20140125 end
#ifdef HX_PORTING_DEB_MSG
			TPD_DMESG("[Himax] Himax TP: tpd_touchinfo return ..\n");
#endif
			return false;
		}
	}

#ifdef HX_PORTING_DEB_MSG
	printk("[Himax] %s: read_len = %d \n",__func__,read_len);
#endif

	if(HX_Gesture==1)
	{
		mdelay(50);
	}
#ifdef HX_MTK_DMA

	if(himax_i2c_read_data(i2c_client, 0x86, read_len, &(data[0])) < 0 )
	{
		printk(KERN_INFO "[HIMAX TP ERROR]:%s:i2c_transfer fail.\n", __func__);
		memset(data, 0xff , 128);

		mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);//? 
		goto work_func_send_i2c_msg_fail;
	}
#else

	for ( i = 0; i < read_len; i = i+8)
	{
	himax_i2c_read_data(i2c_client, 0x86, 8, &(data[i]));
	}
#endif	
	mutex_unlock(&i2c_access);
	
	#ifdef HX_ESD_WORKAROUND
	for(i = 0; i < hx_touch_info_size; i++)
	{
		if(data[i] == 0x00)
		{
			check_sum_cal = 1;
		}
		else if(data[i] == 0xED)
		{
			check_sum_cal = 2;
		}
		else
		{
			check_sum_cal = 0;
			i = hx_touch_info_size;
		}
	}

	//	printk("HIMAX 1 TOUCH_UP_COUNTER = %d\n",TOUCH_UP_COUNTER);	
	//IC status is abnormal ,do hand shaking
	#ifdef HX_TP_SYS_DIAG
	diag_cmd = getDiagCommand();
		
		#ifdef HX_ESD_WORKAROUND
		if((check_sum_cal != 0 || TOUCH_UP_COUNTER > 10) && ESD_RESET_ACTIVATE == 0 && diag_cmd == 0)  //ESD Check
		#else 
		if(check_sum_cal != 0 && diag_cmd == 0)
		#endif
	#else
		#ifdef HX_ESD_WORKAROUND
		if((check_sum_cal != 0 || TOUCH_UP_COUNTER >10 ) && ESD_RESET_ACTIVATE == 0 )  //ESD Check
		#else
		if(check_sum_cal !=0)
		#endif
	#endif
		{
			ret = himax_hang_shaking(); //0:Running, 1:Stop, 2:I2C Fail
			//		enable_irq(ts->client->irq);
			//	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM); 
			if(ret == 2)
			{
				goto work_func_send_i2c_msg_fail;
			}

			if((ret == 1) && (check_sum_cal == 1))
			{
				printk("[HIMAX TP MSG]: ESD event checked - ALL Zero.\n");
				ESD_HW_REST();
			}
			else if(check_sum_cal == 2)
			{
				printk("[HIMAX TP MSG]: ESD event checked - ALL 0xED.\n");
				ESD_HW_REST();
			}
			if(TOUCH_UP_COUNTER > 10)
			{
				printk("[HIMAX TP MSG]: TOUCH UP COUNTER > 10.\n");
				ESD_HW_REST();

			}
			if(TOUCH_UP_COUNTER > 10)
				TOUCH_UP_COUNTER = 0;
#ifdef ENABLE_CHIP_STATUS_MONITOR
			running_status = 0;
			queue_delayed_work(himax_wq, &himax_chip_monitor, 10*HZ);
#endif
			return;
		}
		else if(ESD_RESET_ACTIVATE)
		{
			ESD_RESET_ACTIVATE = 0;
			printk(KERN_INFO "[HIMAX TP MSG]:%s: Back from ESD reset, ready to serve.\n", __func__);
			//Mutexlock Protect Start
			//mutex_unlock(&ts->mutex_lock);
			//Mutexlock Protect End
			//			enable_irq(ts->client->irq);
			//	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM); 
#ifdef ENABLE_CHIP_STATUS_MONITOR
			running_status = 0;
			queue_delayed_work(himax_wq, &thimax_chip_monitor, 10*HZ);
#endif
			return;
		}
#endif

#ifdef Himax_Gesture

#ifdef HX_PORTING_DEB_MSG
		printk("GPG key begin !\n");
#endif
		for(i=0;i<read_len;i++)
		{
#ifdef HX_PORTING_DEB_MSG
			printk(" GPG key data[%d] is %x\n",i, data[i]);
#endif 
			if (check_FC==0)
			{
				if((data[i]==0xFC)||(data[i]==0xF9)||(data[i]==0xF8)||(data[i]==0xFB)||(data[i]==0xFA))
				{
					check_FC = 1;
					gesture_flag = data[i];
				}
				else
				{
					check_FC = 0;
					break;
				}
			}
			else
			{
				if(data[i]!=gesture_flag)
				{
					check_FC = 0;
					break;
				}		
			}
		}

#ifdef HX_PORTING_DEB_MSG
		printk("check_FC is 1 input_report_key on%c , gesture_flag= %c\n ",check_FC,gesture_flag );
		printk("check_FC is   %d!\n", check_FC);
#endif
		if(check_FC == 1 && GestureEnable == 1)
		{
			switch(gesture_flag)
			{
				case GESTURE_DOUBLE_TAP:
					input_report_key(tpd->dev, KEY_POWER, 1);
					input_sync(tpd->dev);

					input_report_key(tpd->dev, KEY_POWER, 0);
					input_sync(tpd->dev);

					HX_Gesture=0;//Avoid TP ic in state of chaos //wangli_20140818
					tpd_halt = 0;                    

                    custom_vibration_enable(50);
                    
					printk("======== 0xFC T-T ========\n");				
					break;
				case GESTURE_NONE:
/*					input_report_key(tpd->dev, KEY_POWER, 1);
					input_sync(tpd->dev);

					input_report_key(tpd->dev, KEY_POWER, 0);
					input_sync(tpd->dev);
*/
					printk("======== 0xFB ? ========\n");				
					break;
				case GESTURE_Q:
					input_report_key(tpd->dev, KEY_POWER, 1);
					input_sync(tpd->dev);

					input_report_key(tpd->dev, KEY_POWER, 0);
					input_sync(tpd->dev);

					printk("======== 0xFA q ========\n");				
					break;
				case GESTURE_E:
					input_report_key(tpd->dev, KEY_POWER, 1);
					input_sync(tpd->dev);

					input_report_key(tpd->dev, KEY_POWER, 0);
					input_sync(tpd->dev);
					printk("======== 0xF8 e ========\n");				
					break;
				case GESTURE_Z:
					input_report_key(tpd->dev, KEY_POWER, 1);
					input_sync(tpd->dev);

					input_report_key(tpd->dev, KEY_POWER, 0);
					input_sync(tpd->dev);
					printk("======== 0xF8 z ========\n");				
					break;
				default:
				
					break;
			}	
/*
			if ((gesture_flag==0xF8)&&(gesture_switch==1))
			{
				//input_report_key(tpd->dev, KEY_F14, 1);//WL
				input_report_key(tpd->dev, KEY_POWER, 1);
				//  input_mt_sync(tpd->kpd);
				input_sync(tpd->dev);

				//input_report_key(tpd->dev, KEY_F14, 0);//WL
				input_report_key(tpd->dev, KEY_POWER, 0);
				//  input_mt_sync(tpd->kpd);
				input_sync(tpd->dev);
				printk("check_FC is KEY_F14\n");

			}
			if ((gesture_flag==0xF9)&&(gesture_switch==1))
			{
				//input_report_key(tpd->dev, KEY_F13, 1);//WL
				input_report_key(tpd->dev, KEY_POWER, 1);
				//  input_mt_sync(tpd->kpd);
				input_sync(tpd->dev);

				//input_report_key(tpd->dev, KEY_F13, 0);//WL
				input_report_key(tpd->dev, KEY_POWER, 0);
				//  input_mt_sync(tpd->kpd);
				input_sync(tpd->dev);
				printk("check_FC is KEY_F13\n");
			}
			if ((gesture_flag==0xFA)&&(gesture_switch==1))
			{
				//input_report_key(tpd->dev, KEY_F16, 1);//WL
				input_report_key(tpd->dev, KEY_POWER, 1);
				//  input_mt_sync(tpd->kpd);
				input_sync(tpd->dev);

				//input_report_key(tpd->dev, KEY_F16, 0);//WL
				input_report_key(tpd->dev, KEY_POWER, 0);
				//  input_mt_sync(tpd->kpd);
				input_sync(tpd->dev);
				printk("check_FC is KEY_F15\n");
			}
			if ((gesture_flag==0xFB)&&(gesture_switch==1))
			{
				//input_report_key(tpd->dev, KEY_F15, 1);//WL
				input_report_key(tpd->dev, KEY_POWER, 1);
				//  input_mt_sync(tpd->kpd);
				input_sync(tpd->kpd);

				//input_report_key(tpd->dev, KEY_F15, 0);//WL
				input_report_key(tpd->dev, KEY_POWER, 0);
				//  input_mt_sync(tpd->kpd);
				input_sync(tpd->dev);
				printk("check_FC is KEY_F16\n");
			}
			if ((gesture_flag==0xFC)&&(wake_switch== 1))
			{
				//input_report_key(tpd->dev, KEY_F17, 1);//WL
				input_report_key(tpd->dev, KEY_POWER, 1);
				//  input_mt_sync(tpd->kpd);
				input_sync(tpd->dev);

				//input_report_key(tpd->dev, KEY_F17, 0);//WL
				input_report_key(tpd->dev, KEY_POWER, 0);
				//  input_mt_sync(tpd->kpd);
				input_sync(tpd->dev);
				printk("check_FC is KEY_F17\n");
			}
*/
#ifdef HX_PORTING_DEB_MSG
			printk("check_FC is 1 input_report_key on \n " );
			printk("Himax GPG key end");
#endif
			return false;
		}

		else
		{
#endif  //Himax_Gesture end
			//calculate the checksum
			check_sum_cal = 0;
			for(i = 0; i < hx_touch_info_size; i++)
			{
				check_sum_cal += data[i];
			}

			//check sum fail
			if ((check_sum_cal != 0x00) || (data[HX_TOUCH_INFO_POINT_CNT] & 0xF0 )!= 0xF0)
			{
				printk(KERN_INFO "[HIMAX TP MSG] checksum fail : check_sum_cal: 0x%02X\n", check_sum_cal);

				mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM); 



#ifdef ENABLE_CHIP_STATUS_MONITOR
				running_status = 0;
				queue_delayed_work(himax_wq, &himax_chip_monitor, 10*HZ);
#endif
				return;
			}

#ifdef TPD_PROXIMITY

			if (tpd_proximity_flag == 1)
			{
				printk("data[point_proximity_position]************:%d\n",data[point_proximity_position]);

				for(i = 0; i < hx_touch_info_size-1; i++)
				{
					if(data[i] == 0xFF)
						check_sum_cal = 0;
					else
					{
						check_sum_cal = 1;
						i = hx_touch_info_size-1;
					}
				}


				if(((data[point_proximity_position] & 0x04) == 0) || (check_sum_cal == 0))
					tpd_proximity_detect = 1;	//No Proxi Detect
				else
				{
					tpd_proximity_detect = 0;	//Proxi	Detect
					TPD_PROXIMITY_DEBUG(" ps change***********************:%d\n",  tpd_proximity_detect);
				}

				//get raw data
				TPD_PROXIMITY_DEBUG(" ps change:%d\n",  tpd_proximity_detect);

				//map and store data to hwm_sensor_data
				sensor_data.values[0] = tpd_get_ps_value();
				sensor_data.value_divide = 1;
				sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;

				//let up layer to know
				if((err_1 = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
					TPD_PROXIMITY_DMESG("call hwmsen_get_interrupt_data fail = %d\n", err_1);

				if(!tpd_proximity_detect)
					return 0;
			}
#endif  //TPD_PROXIMITY end


#ifdef HX_TP_SYS_DIAG
			diag_cmd = getDiagCommand();
			if (diag_cmd >= 1 && diag_cmd <= 6) 
	{
		if (IC_TYPE == HX_85XX_D_SERIES_PWON)
		{
			//Check 128th byte CRC
			for (i = hx_touch_info_size, check_sum_cal = 0; i < 128; i++)
			{
				check_sum_cal += data[i];
			}

			if (check_sum_cal % 0x100 != 0)
			{
				goto bypass_checksum_failed_packet;
			}
		}

		mutual_data = getMutualBuffer();
		self_data   = getSelfBuffer();

		// initiallize the block number of mutual and self
		mul_num = getXChannel() * getYChannel();

		#ifdef HX_EN_SEL_BUTTON
		self_num = getXChannel() + getYChannel() + HX_BT_NUM;
		#else
		self_num = getXChannel() + getYChannel();
		#endif

		//Himax: Check Raw-Data Header
		if(data[hx_touch_info_size] == data[hx_touch_info_size+1] && data[hx_touch_info_size+1] == data[hx_touch_info_size+2] 
		&& data[hx_touch_info_size+2] == data[hx_touch_info_size+3] && data[hx_touch_info_size] > 0) 
		{
			index = (data[hx_touch_info_size] - 1) * RawDataLen;
			//printk("Header[%d]: %x, %x, %x, %x, mutual: %d, self: %d\n", index, buf[56], buf[57], buf[58], buf[59], mul_num, self_num);
			for (i = 0; i < RawDataLen; i++) 
			{
				if (IC_TYPE == HX_85XX_D_SERIES_PWON)
				{
					temp1 = index + i;
				}
				else
				{
					temp1 = index;
				}

				if(temp1 < mul_num)
				{ //mutual
					mutual_data[index + i] = data[i + hx_touch_info_size+4]; //4: RawData Header
				} 
				else 
				{//self
					if (IC_TYPE == HX_85XX_D_SERIES_PWON)
					{
						temp1 = i + index;
						temp2 = self_num + mul_num;
					}
					else
					{
						temp1 = i;
						temp2 = self_num;
					}
					if(temp1 >= temp2)
					{
						break;
					}

					if (IC_TYPE == HX_85XX_D_SERIES_PWON)
					{
						self_data[i+index-mul_num] = data[i + hx_touch_info_size+4]; //4: RawData Header
					}
					else
					{
						self_data[i] = data[i + hx_touch_info_size+4]; //4: RawData Header
					}
				}
			}
		}
		else
		{
			printk(KERN_INFO "[HIMAX TP MSG]%s: header format is wrong!\n", __func__);
		}
	}
	else if(diag_cmd == 7)
	{
		memcpy(&(diag_coor[0]), &data[0], 128);
	}
	//coordinate dump start
	if(coordinate_dump_enable == 1)
	{
		for(i=0; i<(15 + (HX_MAX_PT+5)*2*5); i++)
		{
			coordinate_char[i] = 0x20;
		}
		coordinate_char[15 + (HX_MAX_PT+5)*2*5] = 0xD;
		coordinate_char[15 + (HX_MAX_PT+5)*2*5 + 1] = 0xA;
	}
	//coordinate dump end
	#endif
	//----[HX_TP_SYS_DIAG]----------------------------------------------------------------------------------end

	bypass_checksum_failed_packet:

	#if defined(HX_EN_SEL_BUTTON) || defined(HX_EN_MUT_BUTTON) 
	tpd_key = (data[HX_TOUCH_INFO_POINT_CNT+2]>>4);
	
	// if(tpd_key != 0x0F)
//	return true; 
	
	if(tpd_key == 0x0F)
	{
		tpd_key = 0xFF;
	}	 
	//printk("TPD BT:  %x\r\n", tpd_key);
	#else
	tpd_key = 0xFF;
	#endif

	p_point_num = hx_point_num;

	if(data[HX_TOUCH_INFO_POINT_CNT] == 0xff || data[HX_TOUCH_INFO_POINT_CNT] == 0xf0)
	{
		hx_point_num = 0;
	}
	else
	{
		hx_point_num= data[HX_TOUCH_INFO_POINT_CNT] & 0x0f;
	}

#ifdef HX_PORTING_DEB_MSG
	printk("Himax p_point_num = %d \n",p_point_num);
	printk("Himax hx_point_num = %d \n",hx_point_num);
	printk("Himax tpd_key = %d \n",tpd_key);
#endif

#ifdef BUTTON_CHECK
        cur_frm_max  = 0;
        if(tpd_key!=0xff)
            bt_cnt = 0;    
#endif

#ifdef PT_NUM_LOG
       if(hx_point_num) 
            point_cnt_array[curr_ptr] =100*hx_point_num;
#endif


       if(hx_point_num !=0  && tpd_key==0xFF)
       { 
	       HX_KEY_HIT_FLAG=false;
	       for (i = 0; i < HX_MAX_PT; i++)
	       {
		       if (data[4*i] != 0xFF)
		       {
			       // x and y axis
			       x = data[4 * i + 1] | (data[4 * i] << 8) ;
			       y = data[4 * i + 3] | (data[4 * i + 2] << 8);

			       temp_x[i] = x;
			       temp_y[i] = y;

			       if((x <= x_res) && (y <= y_res))
			       {

				       press = data[4*HX_MAX_PT+i] * 2;//ckt-chunhui.lin
				       area = press;
				       if(area > 31)
				       {
					       area = (area >> 3);
				       }

				       cinfo->x[i] = x;
				       cinfo->y[i] = y;
				       cinfo->p[i] = press;
				       cinfo->id[i] = i;

#ifdef BUTTON_CHECK
				       if(y< 1280)
				       {
					       if(y > cur_frm_max )
					       {
						       pos_queue[p_latest].pos = y;
						       pos_queue[p_latest].timestamp = jiffies;
						       cur_frm_max = y;
					       }
				       }
#endif

#ifdef PT_NUM_LOG
				       if(hx_point_num) 
					       point_cnt_array[curr_ptr] +=1;
#endif


#ifdef HX_PORTING_DEB_MSG
				       printk("[HIMAX PORTING MSG]%s Touch DOWN x = %d, y = %d, area = %d, press = %d.\n",__func__, x, y, area, press);                               
#endif
#ifdef HX_TP_SYS_DIAG
				       //coordinate dump start
				       if(coordinate_dump_enable == 1)
				       {
					       do_gettimeofday(&t);
					       time_to_tm(t.tv_sec, 0, &broken);

					       sprintf(&coordinate_char[0], "%2d:%2d:%2d:%3li,", broken.tm_hour, broken.tm_min, broken.tm_sec, t.tv_usec/1000);				

					       sprintf(&coordinate_char[15 + (i*2)*5], "%4d,", x);
					       sprintf(&coordinate_char[15 + (i*2)*5 + 5], "%4d,", y);


					       coordinate_fn->f_op->write(coordinate_fn,&coordinate_char[0],15 + (HX_MAX_PT+5)*2*sizeof(char)*5 + 2,&coordinate_fn->f_pos);
				       }
				       //coordinate dump end
#endif
			       }
			       else
			       {
				       cinfo->x[i] = 0xFFFF;
				       cinfo->y[i] = 0xFFFF;
				       cinfo->id[i] = i; 

#ifdef HX_PORTING_DEB_MSG
				       printk("[HIMAX PORTING MSG]%s Coor Error : Touch DOWN x = %d, y = %d, area = %d, press = %d.\n",__func__, x, y, area, press);                               
#endif
				       continue;
			       }	

			       cinfo->count++;
		       }
		       else
		       {
			       cinfo->x[i] = 0xFFFF;
			       cinfo->y[i] = 0xFFFF;
		       }
	       }

#ifdef HX_ESD_WORKAROUND
	      TOUCH_UP_COUNTER = 0; 
#endif
       }
       else if(hx_point_num !=0 && tpd_key !=0xFF)
       {
	       //point_key_flag=true;//wangli_20140504  why do this?

       }
       else if(hx_point_num==0 && tpd_key !=0xFF)
       {   
#ifdef BUTTON_CHECK 
	       cur_frm_max = 1;  
	       if(pointFromAA())
	       {
		       if(bt_cnt < 0xffff)
			       bt_cnt+=1;

		       if(bt_cnt< bt_confirm_cnt)          
		       {
			       cur_frm_max = 0;                
		       }
	       }

	       if(cur_frm_max)
	       {
		       if(point_key_flag==false)
		       {
#ifdef FTS_PRESSURE
                               //ckt-chunhui.lin
			       tpd_down(tpd_keys_dim_local[tpd_key-1][0],tpd_keys_dim_local[tpd_key-1][1], 1, 0);
#else
			       tpd_down(tpd_keys_dim_local[tpd_key-1][0],tpd_keys_dim_local[tpd_key-1][1], 0);
#endif
#ifdef HX_ESD_WORKAROUND
			       TOUCH_UP_COUNTER = 0;
#endif 
		       }
		       HX_KEY_HIT_FLAG=true;
#ifdef HX_PORTING_DEB_MSG
		       printk("Press BTN*** \r\n");              
#endif
		       point_key_flag=false;
	       }
#else 
#ifdef FTS_PRESSURE   
               //ckt-chunhui.lin
	       tpd_down(tpd_keys_dim_local[tpd_key-1][0],tpd_keys_dim_local[tpd_key-1][1], 1, 0);
#else
	       tpd_down(tpd_keys_dim_local[tpd_key-1][0],tpd_keys_dim_local[tpd_key-1][1], 0);
#endif
	       HX_KEY_HIT_FLAG=true;
#ifdef HX_ESD_WORKAROUND
	       TOUCH_UP_COUNTER = 0;
#endif
#ifdef HX_PORTING_DEB_MSG
	       printk("Press BTN*** \r\n");
#endif
#endif

       }
       else if(hx_point_num==0 && tpd_key ==0xFF)
       {


	       for(i=0;i<HX_MAX_PT;i++)
	       {
		       cinfo->x[i] = 0xFFFF;
		       cinfo->y[i] = 0xFFFF;
	       }
	       if (tpd_key_old != 0xFF)
	       {


		       tpd_up(tpd_keys_dim_local[tpd_key_old-1][0],tpd_keys_dim_local[tpd_key_old-1][1], 0);
#ifdef HX_PORTING_DEB_MSG
		       printk("Himax Press BTN up*** tpd_key=%d\r\n");
#endif
		       HX_KEY_HIT_FLAG=true;

	       }
	       else
	       {
		       HX_KEY_HIT_FLAG=false;

 
#ifdef HX_ESD_WORKAROUND
		       TOUCH_UP_COUNTER++ ;
#endif
#ifdef HX_TP_SYS_DIAG
		       //coordinate dump start
		       if(coordinate_dump_enable == 1)
		       {
			       do_gettimeofday(&t);
			       time_to_tm(t.tv_sec, 0, &broken);

			       sprintf(&coordinate_char[0], "%2d:%2d:%2d:%lu,", broken.tm_hour, broken.tm_min, broken.tm_sec, t.tv_usec/1000);
			       sprintf(&coordinate_char[15], "Touch up!");
			       coordinate_fn->f_op->write(coordinate_fn,&coordinate_char[0],15 + (HX_MAX_PT+5)*2*sizeof(char)*5 + 2,&coordinate_fn->f_pos);
		       }
		       //coordinate dump end
#endif
	       }
       }
       tpd_key_old = tpd_key;

#ifdef BUTTON_CHECK
       p_prev = p_latest++;
       if(p_latest>= POS_QUEUE_LEN)
	       p_latest = 0;    
#endif

#ifdef PT_NUM_LOG
       if(hx_point_num) 
       {
	       curr_ptr+=1;
	       if(curr_ptr>=PT_ARRAY_SZ)
		       curr_ptr = 0;    
       }
#endif


#ifdef ENABLE_CHIP_STATUS_MONITOR
       running_status = 0;
       queue_delayed_work(himax_wq, &himax_chip_monitor, 10*HZ);
#endif

       TPD_DMESG("end %s, %d\n", __FUNCTION__, __LINE__);

#ifdef Himax_Gesture 
		}
#endif
		return true;

work_func_send_i2c_msg_fail:

		printk(KERN_ERR "[HIMAX TP ERROR]:work_func_send_i2c_msg_fail: %d \n",__LINE__);

#ifdef ENABLE_CHIP_STATUS_MONITOR
		running_status = 0;
		queue_delayed_work(himax_wq, &himax_chip_monitor, 10*HZ);
#endif
}



static void tpd_eint_interrupt_handler(void)
{
    printk("TPD interrupt has been triggered\n");//wangli_20140504
/** /
    if(tpd_load_status==0)
    {
        printk("4.======== tpd_load_status is 0,return ========\n");
        return;
    }
/**/    
    tpd_flag = 1;
    wake_up_interruptible(&waiter);

}

static int touch_event_handler(void *unused)
{
	struct touch_info cinfo, pinfo;
	const u8 PT_LEAVE = 1;
	u8 i;
	static u8 last_hx_point_num = 0;
	struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
	sched_setscheduler(current, SCHED_RR, &param);

	do
	{
		mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
		set_current_state(TASK_INTERRUPTIBLE); 
		wait_event_interruptible(waiter,tpd_flag!=0);

		tpd_flag = 0;
		set_current_state(TASK_RUNNING);
		himax_charge_switch(0);
		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
		if(tpd_touchinfo(&cinfo, &pinfo))
		{
			for(i = 0; i < HX_MAX_PT; i++)
			{
				if (cinfo.x[i] != 0xFFFF)//if (cinfo.y[i] != 0xFFFF) //wangli_20140507
				{
					printk("touch_event_handler HX_MAX_PT=%d,cinfo.x[i]=%d cinfo.y[i]=%d\n",HX_MAX_PT,cinfo.x[i],cinfo.y[i]);//wangli_20140504
					if(HX_KEY_HIT_FLAG == false)
#ifdef FTS_PRESSURE
                                                //ckt-chunhui.lin
						tpd_down(cinfo.x[i], cinfo.y[i], cinfo.p[i], cinfo.id[i]);
#else
						tpd_down(cinfo.x[i], cinfo.y[i], cinfo.id[i]);
						//tpd_down(cinfo.y[i], cinfo.x[i], cinfo.id[i]); //wangli_20140507
#endif
				}
			}
			printk("\nhx_point_num = %d   last_hx_point_num = %d\n",hx_point_num,last_hx_point_num);//wangli_20140504
			if (hx_point_num == 0&&last_hx_point_num !=0)
			{
				printk("\n========>>>>>WILL UP<<<<<======== \n");//wangli_20140504
				printk("\nlast_point_key_flag = %d\n",last_point_key_flag);//wangli_20140504
				//if(last_point_key_flag==false) 			
				tpd_up(cinfo.x[0], cinfo.y[0], i + 1); 
				//tpd_up(cinfo.y[0], cinfo.x[0], i + 1);//wangli_20140430	
			}
			last_hx_point_num=hx_point_num;
			//last_point_key_flag=point_key_flag;//wangli_20140504      
			input_sync(tpd->dev);
		}
		else
		{
			printk("======== IN GESTURE MODE========");
		}
	}
	while(!kthread_should_stop());

	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);//wangli_20140730
    
	return 0;
}

//=============================================================================================================
//
//	Segment : Linux Driver Probe Function
//
//=============================================================================================================

#ifdef Android4_0
	static const struct i2c_device_id tpd_i2c_id[] = {{"hx8527",0},{}};
	static struct i2c_board_info __initdata himax_i2c_tpd={ I2C_BOARD_INFO("hx8527", (0x90>>1))};
	
	static struct i2c_driver tpd_i2c_driver = 
	{
		.driver = 
		{
			.name = "hx8527",
		},
		.probe 	      = tpd_probe,
		.remove       = __devexit_p(tpd_remove),

		.id_table     = tpd_i2c_id,
		.detect       = tpd_detect,
	};
#else
	static const struct i2c_device_id tpd_id[] = {{TPD_DEVICE,0},{}};
	static  unsigned short force[] = {0,0x90,I2C_CLIENT_END,I2C_CLIENT_END}; 
	static const unsigned short * const forces[] = { force, NULL }; 
	static struct i2c_client_address_data addr_data = { .forces = forces, }; 
	
	static struct i2c_driver tpd_i2c_driver = 
	{  
		.driver = 
		{
			.name  = TPD_DEVICE,
			.owner = THIS_MODULE,
		},
		.probe         = tpd_probe,
		.remove        = __devexit_p(tpd_remove),
		.id_table      = tpd_id,
		.detect        = tpd_detect,
		.address_data  = &addr_data,
	};
#endif

static int tpd_detect (struct i2c_client *client, int kind, struct i2c_board_info *info) 
{
	strcpy(info->type, TPD_DEVICE);
	return 0;
}
  u8 isTP_Updated = 0;
static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	printk("======= enter tpd_probe() ========");//wangli	
	int retval = 0;
	char data[5];
	int fw_ret;
	int k;
	int err = 0;
#ifdef TPD_PROXIMITY
	struct hwmsen_object obj_ps;
	int err;
#endif

#ifdef BUTTON_CHECK
	//clear the queue
	for(k = 0 ; k < POS_QUEUE_LEN ; k++)
	{
		pos_queue[k].pos = INVALID_POS;
		pos_queue[k].timestamp = 0;
	}
	p_latest = 1;
	p_prev = 0 ;
	bt_cnt = 0;
#endif

#ifdef PT_NUM_LOG
	for(k = 0 ; k < PT_ARRAY_SZ ; k++) 
		point_cnt_array[k] = 0;

	curr_ptr = 0;
#endif

	client->addr |= I2C_ENEXT_FLAG;
	i2c_client = client;
	i2c_client->timing = 400;


	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	mdelay(100);

	//Interrupt/Reset Pin Setup
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
        
	//Himax: SET Interrupt GPIO, no setting PULL LOW or PULL HIGH  
	mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_DISABLE);
	//mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);

	//Power Pin Setup
	//hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_2800, "TP");
	//hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_1800, "TP_EINT");
	printk("======== hx8527 power setting ========\n");//wangli
	hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_2800, "TP");//wangli
	msleep(100);

	// HW Reset
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	msleep(100);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
	msleep(100);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);	
	msleep(100);

	// Allocate the MTK's DMA memory
#ifdef HX_MTK_DMA    

	gpDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &gpDMABuf_pa, GFP_KERNEL);
	if(!gpDMABuf_va)
	{
		printk(KERN_INFO "[Himax] Allocate DMA I2C Buffer failed\n");
	}
#endif

	// Himax Information / Initial / SYS , I2C must be ready
	if (!himax_ic_package_check())

	{
		printk("[HIMAX TP ERROR] %s: himax_ic_package_check failed\n", __func__);
		return -1;
	}

#ifdef Himax_Gesture
	input_set_capability(tpd->dev, EV_KEY, KEY_POWER);
/*	input_set_capability(tpd->dev, EV_KEY, KEY_F13);
	input_set_capability(tpd->dev, EV_KEY, KEY_F14);
	input_set_capability(tpd->dev, EV_KEY, KEY_F15);
	input_set_capability(tpd->dev, EV_KEY, KEY_F16);
	input_set_capability(tpd->dev, EV_KEY, KEY_F17);*/
#endif
#ifdef FTS_PRESSURE
        input_set_abs_params(tpd->dev, ABS_MT_PRESSURE, 0, PRESS_MAX, 0, 0); //ckt-chunhui.lin
        input_set_abs_params(tpd->dev, ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
#endif

#ifdef MT6592
	//mt_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_TYPE);
	//mt_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_TYPE, tpd_eint_interrupt_handler, 1);


#else
    //Himax: Probe Interrupt Function (Trigger Type detemine by CUST_EINT_TOUCH_PANEL_SENSITIVE = 0 Level Trigger; 1 Edge Trigger)
	mt65xx_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE);  
	mt65xx_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);

	//Himax: Edge Trigger Type  detemine by CUST_EINT_TOUCH_PANEL_POLARITY, Setting = 0 Falling Edge ; 1 Rising Edge) 
	//mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_TOUCH_PANEL_POLARITY, tpd_eint_interrupt_handler,0);	
	mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_TOUCH_PANEL_POLARITY, tpd_eint_interrupt_handler,1);
#endif

msleep(100);
#ifdef HX_TP_SYS_DIAG

	setMutualBuffer();
	if (getMutualBuffer() == NULL) 
	{
		printk(KERN_ERR "[HIMAX TP ERROR] %s: mutual buffer allocate fail failed\n", __func__);
		return -1; 
	}
#endif
	himax_touch_sysfs_init();

#ifdef TPD_PROXIMITY
	hwmsen_detach(ID_PROXIMITY);
	//obj_ps.self = NULL;
	obj_ps.polling = 0;//0-interrupt mode, 1-polling mode
	obj_ps.sensor_operate = tpd_ps_operate;
	if((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
		TPD_PROXIMITY_DMESG("attach fail = %d\n", err);
	else
		TPD_PROXIMITY_DMESG("attach OK = %d\n", err);
#endif

#ifdef  HX_TP_SYS_FLASH_DUMP
	flash_wq = create_singlethread_workqueue("himax_flash_wq");
	if (!flash_wq) 
	{
		printk(KERN_ERR "[HIMAX TP ERROR] %s: create flash workqueue failed\n", __func__);
		//err = -ENOMEM;
		//goto err_create_wq_failed;
	}

	INIT_WORK(&flash_work, himax_ts_flash_work_func);

	setSysOperation(0);
	setFlashBuffer();
#endif
	himax_wq = create_singlethread_workqueue("himax_wq");
	if (!himax_wq) 
	{
		printk(KERN_ERR "[HIMAX TP ERROR] %s: create workqueue failed\n", __func__);
	}

#ifdef ENABLE_CHIP_STATUS_MONITOR
	INIT_DELAYED_WORK(&himax_chip_monitor, himax_chip_monitor_function); //for ESD solution
	running_status = 0;
#endif

//add register misc_device wangli_20140617
#ifdef CONFIG_SUPPORT_CTP_UPG
	if((err = misc_register(&tpd_misc_device)))
	{
		printk("==== mek_tpd:tpd_misc_device register failed! ====\n");
	}
#endif
#if 1  //wangli_20140619
#ifdef HX_FW_UPDATE_BY_I_FILE
	if (isTP_Updated == 0)
	{
		printk(KERN_ERR "Himax TP: isTP_Updated == 0, line:%d\n", __LINE__);//wangli
		fw_ret = Check_FW_Version();
		printk(KERN_ERR "======== func:%s fw_ret=%d line:%d ========\n",__func__,fw_ret,__LINE__);//wangli_20140505
		if ((fw_ret == 1)|| (fw_ret == 0 && himax_read_FW_checksum() == 0))
		{
			if (fts_ctpm_fw_upgrade_with_i_file() <= 0)
			{
				isTP_Updated = 0;
				printk(KERN_ERR "Himax TP: Upgrade Error, line:%d\n", __LINE__);
			}
			else
			{
				isTP_Updated = 1;
				printk(KERN_ERR "Himax TP: Upgrade OK, line:%d\n", __LINE__);
			}
			
			printk(KERN_ERR "======== func:%s line:%d isTP_Updated:%d ========\n",__func__,__LINE__,isTP_Updated);//wangli_20140505

			mdelay(100);
		}
		
	}

#endif

#endif
	himax_touch_information();

	himax_HW_reset();
	calculate_point_number();
	msleep(10);
	setXChannel(HX_RX_NUM); // X channel
	setYChannel(HX_TX_NUM); // Y channel

	himax_ts_poweron();


	printk("[Himax] %s End power on \n",__func__); 

	tpd_halt = 0;
	touch_thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
	if (IS_ERR(touch_thread))
	{ 
		retval = PTR_ERR(touch_thread);
		TPD_DMESG(TPD_DEVICE "[Himax] Himax TP: Failed to create kernel thread: %d\n", retval);
	}

#ifdef ENABLE_CHIP_STATUS_MONITOR
	queue_delayed_work(himax_wq, &himax_chip_monitor, 60*HZ);   //for ESD solution
#endif

	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);

	TPD_DMESG("[Himax] Himax TP: Touch Panel Device Probe %s\n", (retval < 0) ? "FAIL" : "PASS");
	return 0;

HimaxErr:

#ifdef ENABLE_CHIP_STATUS_MONITOR
	cancel_delayed_work(&himax_chip_monitor);
#endif
	TPD_DMESG("[Himax] Himax TP: I2C transfer error, line: %d\n", __LINE__);

//set atomic 0,no interrupt 
#ifdef CONFIG_SUPPORT_CTP_UPG
	atomic_set(&upgrading,0);
#endif	
	return -1;
}

static int __devexit tpd_remove(struct i2c_client *client)
{
	himax_touch_sysfs_deinit();

	#ifdef HX_MTK_DMA    
	if(gpDMABuf_va)
	{
		dma_free_coherent(NULL, 4096, gpDMABuf_va, gpDMABuf_pa);
		gpDMABuf_va = NULL;
		gpDMABuf_pa = NULL;
	}
	#endif

	TPD_DMESG("[Himax] Himax TP: TPD removed\n");
	return 0;
}


//=============================================================================================================
//
//	Segment : Himax Linux Driver Probe Function - MTK
//
//=============================================================================================================

static int tpd_local_init(void)
{ 
	TPD_DMESG("[Himax] HIMAX_TS I2C Touchscreen Driver (Built %s @ %s)\n", __DATE__, __TIME__);

	if(i2c_add_driver(&tpd_i2c_driver)!=0)
	{
		TPD_DMESG("[Himax] unable to add i2c driver.\n");
		return -1;
	}
	
	#if defined(HX_EN_SEL_BUTTON) || defined(HX_EN_MUT_BUTTON) 
	tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
	#endif   
	printk("ningyd : tpd_halt = %d in local_init\n",tpd_halt);
	TPD_DMESG("[Himax] end %s, %d\n", __FUNCTION__, __LINE__);
	tpd_type_cap = 1;//wangli_20140530  
	return 0; 
}

static int tpd_resume(struct i2c_client *client)
{
	int retval = 0;
	int ret = 0;
  	char data[2];
	//if (tpd_proximity_flag == 1)
	//return 0;

	himax_charge_switch(1);
	#ifdef Himax_Gesture
	if (GestureEnable==1)
	{	

		printk("[Himax]%s enter , write 0x90:0x00 \n",__func__);
        
		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);//SWU_fail_0731
		mutex_lock(&i2c_access);//SWU_fail_0731

        HX_Gesture=0;
		tpd_halt = 0;
        
        himax_HW_reset();
        
	    if(himax_ts_poweron() < 0)
	    {
		    printk("[Himax] tpd_resume himax_ts_poweron failed\n");
	    }        
/** /
		data[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0x91, 1,&data[0]);//normal mode wangli_20140613

		data[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0x90, 1, &data[0]);
		//////---------------- modify on 1209////////////////
		mdelay(10);

		data[0] =0x00;
		if( himax_i2c_write_data(i2c_client, 0xD7, 1, &(data[0])) < 0 )
		{
			printk("[Himax] smart_resume send comand D7 failed\n");
			return -1;
		}        
		msleep(1);
       
		himax_i2c_write_data(i2c_client, 0x82, 0, &data[0]);
		mdelay(120);        

		himax_i2c_write_data(i2c_client, 0x83, 0, &data[0]);
		mdelay(120);
		/////------------------modify on 1209///////////////

		himax_i2c_write_data(i2c_client, 0x81, 0, &data[0]);
/**/
        mutex_unlock(&i2c_access);//SWU_fail_0731
		//msleep(120); //120ms
		mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);//wangli_20140730
        
		//HX_Gesture=0;
		//tpd_halt = 0;
	}
	else
	#endif
	{
	    #ifdef HX_CLOSE_POWER_IN_SLEEP			
	    hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_2800, "TP");
	    //hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_1800, "TP_EINT");//wangli_20140530
	    printk("Himax rst in resume should not show up ");
	    himax_HW_reset();
	    #endif    
	
	    data[0] =0x00;
	    if( himax_i2c_write_data(i2c_client, 0xD7, 1, &(data[0])) < 0 )
	    {
		printk("[Himax] tpd_resume send comand D7 failed\n");
	        return -1;
	    }        
	    msleep(1);
	
	    if( himax_ts_poweron() < 0)
	    {
		printk("[Himax] tpd_resumehimax_ts_poweron failed\n");
		return -1;
	    }
	
	    printk("Himax suspend cmd in resume should not show up ");

	    #ifdef HX_ESD_WORKAROUND
	    ret = himax_hang_shaking(); //0:Running, 1:Stop, 2:I2C Fail
	    if(ret == 2)
	    {
		//queue_delayed_work(himax_wq, &himax_chip_reset_work, 0);
                ESD_HW_REST();
		printk(KERN_INFO "[Himax] %s: I2C Fail \n", __func__);
	    }
	    if(ret == 1)
	    {
		printk(KERN_INFO "[Himax] %s: MCU Stop \n", __func__);
		//Do HW_RESET??
		ESD_HW_REST();
	    }
	    else
	    {
		printk(KERN_INFO "[Himax] %s: MCU Running \n", __func__);
	    }
	    #endif
	
	    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	    tpd_halt = 0;
	    /* next 2 lines: up once when ctp resume   wangli_20140504*/	
	    tpd_up(0,0,0);
	    input_sync(tpd->dev);		

	}
	return retval;
}

//Himax: Suspend Function 
static int tpd_suspend(struct i2c_client *client, pm_message_t message)
{
	int retval = 0;
	int i;
	static char data[2];
	#ifdef Himax_Gesture		
	if (GestureEnable == 1)
	{
		//mdelay(50);

		mutex_lock(&i2c_access);//SWU_fail_0731  
        
		tpd_halt = 1;
		HX_Gesture=1;       
        
        //clean interrupt stack
		data[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0x88, 0, &data[0]);
		//mdelay(50);
		mdelay(5);
        
		data[0] = 0x10;
		himax_i2c_write_data(i2c_client, 0x90, 1, &data[0]);
        
		mutex_unlock(&i2c_access);//SWU_fail_0731
		//mdelay(50);
        
		printk("[Himax]%s enter , write 0x90:0x10 \n",__func__);
		
	}
	else
	#endif
	{
    	#ifdef HX_TP_SYS_FLASH_DUMP
		if(getFlashDumpGoing())
		{
			printk(KERN_INFO "[himax] %s: Flash dump is going, reject suspend\n",__func__);
			return 0;
		}
		#endif

		tpd_halt = 1;
		TPD_DMESG("[Himax] Himax TP: Suspend\n");
		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
		mutex_lock(&i2c_access);

		#ifdef HX_CLOSE_POWER_IN_SLEEP
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
		printk("Himax rst in suspend should not show up ");
		hwPowerDown(MT6323_POWER_LDO_VGP1,  "TP"); //wangli_20140430
		//hwPowerDown(MT6323_POWER_LDO_VGP1,  "TP");
		//hwPowerDown(MT65XX_POWER_LDO_VGP5, "TP_EINT"); //wangli_20140430
		#else
		//Himax: Sense Off
		himax_i2c_write_data(i2c_client, 0x82, 0, &(data[0]));
		msleep(120);
	
		//Himax: Sleep In
		himax_i2c_write_data(i2c_client, 0x80, 0, &(data[0]));
		msleep(120);
	
		//Himax: Deep Sleep In
		data[0] =0x01;
		himax_i2c_write_data(i2c_client, 0xD7, 1, &(data[0]));
		msleep(100);
		printk("Himax suspend cmd in suspend should not show up ");
		#endif

		mutex_unlock(&i2c_access);
	}
	return retval;
} 
//For factory mode to get TP info //wangli_20140522
static ssize_t show_chipinfo(struct device *dev,struct device_attribute *attr,char *buf)
{
	struct i2c_client *client = i2c_client;
	unsigned char ver=0;
	unsigned ver_h = 0;
	unsigned ver_l = 0;

	if (NULL == i2c_client)
	{
		printk("i2c client is null!!!\n");
		return 0;
	}

	himax_read_FW_ver();
	ver_h = CFG_VER_MIN_buff[1] - '0';
	ver_l = CFG_VER_MIN_buff[2] - '0';
	printk("CFG_VER_MIN_buff[1] = %x,CFG_VER_MIN_buff[2] = %x\n",CFG_VER_MIN_buff[1],CFG_VER_MIN_buff[2]);

	return sprintf(buf,"ID:0x00 VER:0x%x%x IC:hx8527 VENDOR:truely\n",ver_h,ver_l);
}

static DEVICE_ATTR(chipinfo,0444,show_chipinfo,NULL);
// add double tap sysfs interface
#ifdef	Himax_Gesture
static ssize_t show_control_double_tap(struct device *dev,struct device_attribute *attr,char *buf)
{
	struct i2c_client *client = i2c_client;
	
	return sprintf(buf,"gesture state:%s \n",GestureEnable==0 ? "Disable" : "Enable");
}

static ssize_t store_control_double_tap(struct device *dev,struct device_attribute *attr,const char *buf,size_t size)
{
	char *pvalue = NULL;
	if(buf != NULL && size != 0)
	{
		printk("[hx8527]store_control_double_tap buf is %s and size is %d \n",buf,size);
		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
		GestureEnable = simple_strtoul(buf,&pvalue,16);
		mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
		printk("[hx8527]store_control_double_tap : %s\n",GestureEnable==0 ? "Disable" : "Enable");
	}
}

static DEVICE_ATTR(control_double_tap,0666,show_control_double_tap,store_control_double_tap);
#endif 

static const struct device_attribute * const ctp_attributes[] = {
	&dev_attr_chipinfo,
#ifdef	Himax_Gesture
	&dev_attr_control_double_tap
#endif
};

//Himax: Touch Driver Structure
static struct tpd_driver_t tpd_device_driver = 
{
	.tpd_device_name = "HIMAX_TS",
	.tpd_local_init = tpd_local_init,
	.suspend = tpd_suspend,
	.resume = tpd_resume,
	//For factory mode :read chip info //wangli_20140522
	.attrs=
	{
		.attr=ctp_attributes,
		#ifdef	Himax_Gesture
		.num=2
		#else
		.num=1
		#endif
	},
	//end //wangli_20140522
	#if defined(HX_EN_SEL_BUTTON) || defined(HX_EN_MUT_BUTTON) 
	.tpd_have_button = 1,
	#else
	.tpd_have_button = 0,
	#endif		
};

//Himax: Called when loaded into kernel
static int __init tpd_driver_init(void) 
{
	i2c_register_board_info(0, &himax_i2c_tpd, 1);
	TPD_DMESG("[Himax] MediaTek HIMAX_TS touch panel driver init\n");
	
	if(tpd_driver_add(&tpd_device_driver) < 0)
		TPD_DMESG("[Himax] add HIMAX_TS driver failed\n");
		
	return 0;
}
 
/* should never be called */
static void __exit tpd_driver_exit(void) 
{
	TPD_DMESG("[Himax] MediaTek HIMAX_TS touch panel driver exit\n");
	tpd_driver_remove(&tpd_device_driver);
}
 
module_init(tpd_driver_init);
module_exit(tpd_driver_exit);


//=============================================================================================================
//
//	Segment : Other Function
//
//=============================================================================================================
int himax_cable_status(int status)
{
    uint8_t buf0[2] = {0};
	
    return 0;
}
EXPORT_SYMBOL(himax_cable_status);
