#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"
#include <cust_gpio_usage.h>
#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_gpio.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1280)

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#define HX8394_LCM_ID				(0x94)

#ifndef BUILD_LK
static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
#endif
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)



#define   LCM_DSI_CMD_MODE							0

static bool lcm_is_init = false;


#ifdef BUILD_LK
#include  <platform/mt_pmic.h>
static void dct_pmic_VGP2_enable(bool dctEnable)
{
	pmic_config_interface(DIGLDO_CON29, 0x5, PMIC_RG_VGP2_VOSEL_MASK, PMIC_RG_VGP2_VOSEL_SHIFT); // 2.8v ËÕ ÓÂ 2013Äê10ÔÂ31ÈÕ 17:55:43
	pmic_config_interface( (U32)(DIGLDO_CON8),
                             (U32)(dctEnable),
                             (U32)(PMIC_RG_VGP2_EN_MASK),
                             (U32)(PMIC_RG_VGP2_EN_SHIFT)
	                         );
}
#else
void dct_pmic_VGP2_enable(bool dctEnable);
#endif

extern void DSI_clk_ULP_mode(bool enter);
extern void DSI_lane0_ULP_mode(bool enter);
static void DSI_Enter_ULPM(bool enter)
{
	DSI_clk_ULP_mode(enter);  //enter ULPM
	DSI_lane0_ULP_mode(enter);
}

static LCM_setting_table_V3 lcm_initialization_setting[] = {
    {0x39,0xB9,3,{0xFF,0x83,0x94}},
    {0x39,0xBA,2,{0x33,0x83}},
    {0x39,0xB1,15,{0x6c,0x12,0x12,0x26,0x04,0x11,0xF1,0x81,0x3a,0x54,0x23,0x80,0xC0,0xD2,0x58}},//SET POWER
    {0x39,0xB4,12,{0x00,0xFF,0x51,0x5A,0x59,0x5A,0x03,0x5A,0x01,0x70,0x01,0x70}},//SET CYC
    {0x39,0xB2,11,{0x00,0x64,0x0E,0x0D,0x22,0x1C,0x08,0x08,0x1C,0x4D,0x00}},//SET DISPLAY RELATED REGISTER
    {0x15,0xbc,1,{0x07}},//add new
    {0x39,0xbf,3,{0x41,0x0e,0x01}},//add new
    {0x39,0xD3,37,{0x00,0x0f,0x00,0x40,0x07,0x10,0x00,0x08,0x10,0x08,0x00,0x08,0x54,0x15,0x0E,0x05,0x0E,0x02,0x15,0x06,0x05,0x06,0x47,0x44,0x0A,0x0A,0x4B,0x10,0x07,0x07,0x08,0x00,0x00,0x00,0x0A,0x00,0x01}},
    {0x39,0xD5,44,{0x1A,0x1A,0x1B,0x1B,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x24,0x25,0x18,0x18,0x26,0x27,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x20,0x21,0x18,0x18,0x18,0x18}},
    {0x39,0xD6,44,{0x1A,0x1A,0x1B,0x1B,0x0B,0x0A,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00,0x21,0x20,0x58,0x58,0x27,0x26,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x25,0x24,0x18,0x18,0x18,0x18}},
    {0x15,0xCC,1,{0x09}},
    {0x39,0xC0,2,{0x30,0x14}},
    {0x39,0xC7,4,{0x00,0xC0,0x40,0xC0}},
    {0x39,0xB6,2,{0x67,0x67}},

#if 0 //  2.2
    {0x15,0xBD,1,{0x00}},
    {0x39,0xC1,43,{0x01,0x00,0x06,0x0c,0x14,0x1D,0x27,0x2f,0x38,0x41,0x49,0x51,0x59,0x61,0x69,0x71,0x79,0x81,0x89,0x91,0x99,0xa1,0xa9,0xb2,0xb9,0xc1,0xCa,0xd1,0xd8,0xe2,0xea,0xf0,0xf7,0xFF,0x38,0xfc,0x3f,0x0b,0xc1,0x13,0xf1,0x0d,0xc0}},
    {0x15,0xBD,1,{0x01}},
    {0x39,0xC1,42,{0x00,0x06,0x0c,0x14,0x1D,0x27,0x2f,0x38,0x41,0x49,0x51,0x59,0x61,0x69,0x71,0x79,0x81,0x89,0x91,0x99,0xa1,0xa9,0xb2,0xb9,0xc1,0xCa,0xd1,0xd8,0xe2,0xea,0xf0,0xf7,0xFF,0x38,0xfc,0x3f,0x0b,0xc1,0x13,0xf1,0x0d,0xc0}},
    {0x15,0xBD,1,{0x02}},
    {0x39,0xC1,42,{0x00,0x06,0x0c,0x14,0x1D,0x27,0x2f,0x38,0x41,0x49,0x51,0x59,0x61,0x69,0x71,0x79,0x81,0x89,0x91,0x99,0xa1,0xa9,0xb2,0xb9,0xc1,0xCa,0xd1,0xd8,0xe2,0xea,0xf0,0xf7,0xFF,0x38,0xfc,0x3f,0x0b,0xc1,0x13,0xf1,0x0d,0xc0}},//SET GAMMA 
#elif 1 // 2.0
    {0x15,0xBD,1,{0x00}},
    {0x39,0xC1,43,{0x01,0x00,0x08,0x11,0x1A,0x26,0x2F,0x38,0x41,0x4A,0x52,0x5A,0x62,0x6A,0x72,0x7B,0x82,0x89,0x91,0x99,0xA0,0xA8,0xB0,0xB8,0xBF,0xC7,0xCE,0xD4,0xDC,0xE5,0xEC,0xF2,0xF8,0xFF,0x03,0x13,0x54,0x18,0x82,0x63,0x95,0x10,0xC0}},
    {0x15,0xBD,1,{0x01}},
    {0x39,0xC1,42,{0x00,0x08,0x11,0x1A,0x26,0x2F,0x38,0x41,0x4A,0x52,0x5A,0x62,0x6A,0x72,0x7B,0x82,0x89,0x91,0x99,0xA0,0xA8,0xB0,0xB8,0xBF,0xC7,0xCE,0xD4,0xDC,0xE5,0xEC,0xF2,0xF8,0xFF,0x03,0x13,0x54,0x18,0x82,0x63,0x95,0x10,0xC0}},
    {0x15,0xBD,1,{0x02}},
    {0x39,0xC1,42,{0x00,0x08,0x11,0x1A,0x26,0x2F,0x38,0x41,0x4A,0x52,0x5A,0x62,0x6A,0x72,0x7B,0x82,0x89,0x91,0x99,0xA0,0xA8,0xB0,0xB8,0xBF,0xC7,0xCE,0xD4,0xDC,0xE5,0xEC,0xF2,0xF8,0xFF,0x03,0x13,0x54,0x18,0x82,0x63,0x95,0x10,0xC0}},//SET GAMMA 
#elif 0 // 2.4
    {0x15,0xBD,1,{0x00}},
    {0x39,0xC1,43,{0x01,0x00,0x06,0x08,0x0F,0x17,0x20,0x28,0x30,0x38,0x41,0x49,0x51,0x59,0x60,0x69,0x71,0x7A,0x81,0x89,0x92,0x9A,0xA2,0xAB,0xB4,0xBC,0xC5,0xCD,0xD5,0xDE,0xE8,0xEF,0xF6,0xFF,0x1F,0x0D,0x9F,0x72,0x3C,0xFE,0xBC,0xCF,0xC0}},
    {0x15,0xBD,1,{0x01}},
    {0x39,0xC1,42,{0x00,0x06,0x08,0x0F,0x17,0x20,0x28,0x30,0x38,0x41,0x49,0x51,0x59,0x60,0x69,0x71,0x7A,0x81,0x89,0x92,0x9A,0xA2,0xAB,0xB4,0xBC,0xC5,0xCD,0xD5,0xDE,0xE8,0xEF,0xF6,0xFF,0x1F,0x0D,0x9F,0x72,0x3C,0xFE,0xBC,0xCF,0xC0}},
    {0x15,0xBD,1,{0x02}},
    {0x39,0xC1,42,{0x00,0x06,0x08,0x0F,0x17,0x20,0x28,0x30,0x38,0x41,0x49,0x51,0x59,0x60,0x69,0x71,0x7A,0x81,0x89,0x92,0x9A,0xA2,0xAB,0xB4,0xBC,0xC5,0xCD,0xD5,0xDE,0xE8,0xEF,0xF6,0xFF,0x1F,0x0D,0x9F,0x72,0x3C,0xFE,0xBC,0xCF,0xC0}},//SET GAMMA 
#elif 0 // 2.6
    {0x15,0xBD,1,{0x00}},
    {0x39,0xC1,43,{0x01,0x00,0x05,0x07,0x0C,0x11,0x19,0x22,0x2A,0x31,0x39,0x42,0x4A,0x52,0x59,0x61,0x69,0x72,0x7B,0x83,0x8B,0x94,0x9C,0xA5,0xAF,0xB8,0xC0,0xCB,0xD2,0xDB,0xE6,0xEE,0xF6,0xFF,0x28,0xE0,0x20,0x3B,0x40,0x3D,0x30,0x84,0xC0}},
    {0x15,0xBD,1,{0x01}},
    {0x39,0xC1,42,{0x00,0x05,0x07,0x0C,0x11,0x19,0x22,0x2A,0x31,0x39,0x42,0x4A,0x52,0x59,0x61,0x69,0x72,0x7B,0x83,0x8B,0x94,0x9C,0xA5,0xAF,0xB8,0xC0,0xCB,0xD2,0xDB,0xE6,0xEE,0xF6,0xFF,0x28,0xE0,0x20,0x3B,0x40,0x3D,0x30,0x84,0xC0}},
    {0x15,0xBD,1,{0x02}},
    {0x39,0xC1,42,{0x00,0x05,0x07,0x0C,0x11,0x19,0x22,0x2A,0x31,0x39,0x42,0x4A,0x52,0x59,0x61,0x69,0x72,0x7B,0x83,0x8B,0x94,0x9C,0xA5,0xAF,0xB8,0xC0,0xCB,0xD2,0xDB,0xE6,0xEE,0xF6,0xFF,0x28,0xE0,0x20,0x3B,0x40,0x3D,0x30,0x84,0xC0}},//SET GAMMA 
#endif

    {0x05,0x11,1,{0x00}}, 
    {REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 200, {}},
    {0x05,0x29,1,{0x00}},
    {REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 50, {}},
};


static void init_lcm_registers(void)///lrz0810
{
    unsigned int data_array[40];
    //////////////////////////////////////////////lrzadd0821////for1222
    
    data_array[0] = 0x00043902;                          
    data_array[1] = 0x9483ffB9; 
    dsi_set_cmdq(data_array, 2, 1); 
    
    data_array[0] = 0x00033902;                          
    data_array[1] = 0x008333Ba; 
    dsi_set_cmdq(data_array, 2, 1); 
    
    data_array[0] = 0x00103902;                          
    data_array[1] = 0x12126cB1; 
    data_array[2] = 0xf1110423;
    data_array[3] = 0x2354ff80;
    data_array[4] = 0x58d2c080;
    dsi_set_cmdq(data_array, 5, 1); 
    
    data_array[0] = 0x000d3902;                          
    data_array[1] = 0x51ff00B4; 
    data_array[2] = 0x035a595a; 
    data_array[3] = 0x0170015a;                          
    data_array[4] = 0x00000070;  
    dsi_set_cmdq(data_array, 5, 1);
    
    data_array[0] = 0x000c3902;                          
    data_array[1] = 0x0e6400B2; 
    data_array[2] = 0x081c320d;
    data_array[3] = 0x004d1c08;
    dsi_set_cmdq(data_array, 4, 1);
    
    data_array[0] = 0x00263902;                          
    data_array[1] = 0x000700D3;
    data_array[2] = 0x00100740;
    data_array[3] = 0x00081008;
    data_array[4] = 0x0E155408;
    data_array[5] = 0x15020E05;
    data_array[6] = 0x47060506;
    data_array[7] = 0x4B0A0A44;
    data_array[8] = 0x08070710;
    data_array[9] = 0x0A000000;
    data_array[10] =0x00000100;
    dsi_set_cmdq(data_array, 11, 1); 
    
    data_array[0] = 0x002d3902;                          
    data_array[1] = 0x1B1A1AD5;
    data_array[2] = 0x0201001B;
    data_array[3] = 0x06050403;
    data_array[4] = 0x0A090807;
    data_array[5] = 0x1825240B;
    data_array[6] = 0x18272618;
    data_array[7] = 0x18181818;
    data_array[8] = 0x18181818;
    data_array[9] = 0x18181818;
    data_array[10] =0x20181818;
    data_array[11] =0x18181821;
    data_array[12] =0x00000018;
    dsi_set_cmdq(data_array, 13, 1); 
    
    data_array[0] = 0x002d3902;                          
    data_array[1] = 0x1B1A1AD6;
    data_array[2] = 0x090A0B1B;
    data_array[3] = 0x05060708;
    data_array[4] = 0x01020304;
    data_array[5] = 0x58202100;
    data_array[6] = 0x18262758;
    data_array[7] = 0x18181818;
    data_array[8] = 0x18181818;
    data_array[9] = 0x18181818;
    data_array[10] =0x25181818;
    data_array[11] =0x18181824;
    data_array[12] =0x00000018;
    dsi_set_cmdq(data_array, 13, 1); 
    
    data_array[0] = 0x00053902;                          
    data_array[1] = 0x00c000C7; 
    data_array[2] = 0x000000c0; 
    dsi_set_cmdq(data_array, 3, 1);
    
    data_array[0] = 0x09cc1500;
    dsi_set_cmdq(data_array, 1, 1);
    
    data_array[0]= 0x00033902;
    data_array[1]= 0x006c6cb6;
    dsi_set_cmdq(data_array,2, 1);
    
    data_array[0] = 0x00350500;
    dsi_set_cmdq(data_array, 1, 1);
    
    data_array[0] = 0xff511500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(1);
    data_array[0] = 0x24531500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(1);
    data_array[0] = 0x00551500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);//20
    
    
    data_array[0] = 0x00110500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(150);//200
    
    data_array[0] = 0x00290500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(30);
    
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));
    
    params->type   = LCM_TYPE_DSI;
    
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;
    
    params->physical_width = 62;
    params->physical_height = 110;
    
#if (LCM_DSI_CMD_MODE)
    params->dsi.mode   = CMD_MODE;
#else
    params->dsi.mode   = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 
#endif
    
    // DSI
    /* Command mode setting */
    //1 Three lane or Four lane
    params->dsi.LANE_NUM				= LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
    
    // Video mode setting		
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    
    params->dsi.vertical_sync_active				= 4;// 3    2
    params->dsi.vertical_backporch					= 12;//5; //0x0d;// 20   1
    params->dsi.vertical_frontporch					= 15;//0d;//0x08; // 1  12
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 
    
    params->dsi.horizontal_sync_active				= 40;//10;//12;// 50  2
    params->dsi.horizontal_backporch				= 50;//16;//5f;
    params->dsi.horizontal_frontporch				= 75;//16;//5f;
    params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
    
    //params->dsi.LPX=8; 
    
    // Bit rate calculation
    //params->dsi.PLL_CLOCK = 250;//250;
    //1 Every lane speed
    params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
    params->dsi.pll_div2=0;		// div2=0,1,2,3;div1_real=1,2,4,4	
    params->dsi.fbk_div =8;//9;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	

}


static void lcm_init(void)
{
    //SET_RESET_PIN(0);
    //MDELAY(20); 
    //SET_RESET_PIN(1);
    //MDELAY(20); 
    
    lcm_is_init = true;
//    dct_pmic_VGP2_enable(1);
//    MDELAY(10);
    mt_set_gpio_mode(GPIO_LCM_RST,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST,GPIO_OUT_ONE);
    MDELAY(5); 
    
    //SET_RESET_PIN(0);
    mt_set_gpio_out(GPIO_LCM_RST,GPIO_OUT_ZERO);
    MDELAY(50);
    
    //SET_RESET_PIN(1);
    mt_set_gpio_out(GPIO_LCM_RST,GPIO_OUT_ONE);
    MDELAY(180); 
    
    #if 1
    dsi_set_cmdq_V3(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);
    #else
    init_lcm_registers();
    #endif
}

static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0]=0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120); 

	#if 1
	DSI_Enter_ULPM(1); /* Enter low power mode  */

	MDELAY(60);
	//SET_RESET_PIN(0);
	mt_set_gpio_out(GPIO_LCM_RST,GPIO_OUT_ZERO);
	MDELAY(100);

//	dct_pmic_VGP2_enable(0); /* [Ted 5-28] Disable VCI power to prevent lcd polarization */
	lcm_is_init = false;
	
	#else
	SET_RESET_PIN(1);	
	SET_RESET_PIN(0);
	MDELAY(1); // 1ms
	
	SET_RESET_PIN(1);
	MDELAY(120);     
	lcm_util.set_gpio_out(GPIO_LCD_ENN, GPIO_OUT_ZERO);
	lcm_util.set_gpio_out(GPIO_LCD_ENP, GPIO_OUT_ZERO); 
	#endif
}


static void lcm_resume(void)
{
	#if 1
	if(!lcm_is_init)
		lcm_init();
	#else
	lcm_util.set_gpio_out(GPIO_LCD_ENN, GPIO_OUT_ONE);
	lcm_util.set_gpio_out(GPIO_LCD_ENP, GPIO_OUT_ONE);
	lcm_init();
	#endif
}
         
#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif


extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);

static unsigned int lcm_compare_id(void)
{
#if 0
  	unsigned int ret = 0;

	ret = mt_get_gpio_in(GPIO92);
#if defined(BUILD_LK)
	printf("%s, [jx]hx8394a GPIO92 = %d \n", __func__, ret);
#endif	

	return (ret == 0)?1:0; 
#else
	unsigned int id=0;
	unsigned char buffer[2] = {0,0};
	unsigned int array[16];  

//	dct_pmic_VGP2_enable(1);

	mt_set_gpio_mode(GPIO_LCM_RST,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_RST,GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_RST,GPIO_OUT_ONE);
	MDELAY(5); 
		    
	//SET_RESET_PIN(0);
	mt_set_gpio_out(GPIO_LCM_RST,GPIO_OUT_ZERO);
	MDELAY(50);
			
	//SET_RESET_PIN(1);
	mt_set_gpio_out(GPIO_LCM_RST,GPIO_OUT_ONE);
	MDELAY(105); 

	array[0] = 0x00043902;
	array[1] = 0x9483FFB9;// page enable
	dsi_set_cmdq(&array, 2, 1);
	MDELAY(10);

	array[0] = 0x00023902;
	array[1] = 0x000013BA;       	  
	dsi_set_cmdq(array, 2, 1);
	MDELAY(10);

	array[0] = 0x00013700;// return byte number
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xF4, buffer, 1);
	id = buffer[0]; 

	#ifdef BUILD_LK
//	printf("[LK]---cmd---hx8394d_hd720_dsi_vdo_truly----%s------[0x%x]\n",__func__,buffer[0]);
    #else
//	printk("[KERNEL]---cmd---hx8394d_hd720_dsi_vdo_truly----%s------[0x%x]\n",__func__,buffer[0]);
    #endif	
	if(id==HX8394_LCM_ID)
	{
		int adcdata[4];
		int lcmadc=0;
		IMM_GetOneChannelValue(0,adcdata,&lcmadc);
		lcmadc = lcmadc * 1500/4096; 
		#ifdef BUILD_LK
		printf("[LK]---cmd---hx8394d_hd720_dsi_vdo_truly---%s---adc[%d]\n",__func__,lcmadc);
		#else
		printk("[KERNEL]---cmd---hx8394d_hd720_dsi_vdo_truly---%s---adc[%d]\n",__func__,lcmadc);
		#endif	
		if(lcmadc > 1000 && lcmadc < 1500)
			return 1;
	}
	return 0;//(id == HX8394_LCM_ID)?1:0;
#endif
}


static unsigned int lcm_esd_check(void)
{
  #ifndef BUILD_LK
	char  buffer[3];
	int   array[4];

	if(lcm_esd_test)
	{
		lcm_esd_test = FALSE;
		return TRUE;
	}

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0a, buffer, 1);
	printk("[%s]buffer[0]=0x%x",__func__,buffer[0]);
	if(buffer[0]==0x1c)
	{
		return FALSE;
	}
	else
	{			 
		return TRUE;
	}
#else
	return FALSE;
#endif

}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();
	//lcm_resume();

	return TRUE;
}



LCM_DRIVER hx8394d_hd720_dsi_vdo_truly_lcm_drv = 
{
    .name			= "hx8394d_hd720_dsi_vdo_truly",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.esd_check = lcm_esd_check,
	.esd_recover = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
    };
