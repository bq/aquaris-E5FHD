/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
    #include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
    #include <mach/mt_gpio.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define LCD_DEBUG
#ifdef BUILD_LK
    #ifdef LCD_DEBUG
    #define LCM_DEBUG(format, ...)   printf("lk " format "\n", ## __VA_ARGS__)
    #else
    #define LCM_DEBUG(format, ...)
    #endif
#else
    #ifdef LCD_DEBUG
    #define LCM_DEBUG(format, ...)   printk("kernel " format "\n", ## __VA_ARGS__)
    #else
    #define LCM_DEBUG(format, ...)
    #endif
#endif

//#define LCM_ID_R63315  (0x00)

#define FRAME_WIDTH  (1080)
#define FRAME_HEIGHT (1920)

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

extern void TC358768_DCS_write_1A_1P(unsigned char cmd, unsigned char para);
extern void lcm_reset(void);
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)     lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                       lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                   lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                        lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)                lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define dsi_lcm_set_gpio_out(pin, out)                       lcm_util.set_gpio_out(pin, out)
#define dsi_lcm_set_gpio_mode(pin, mode)                     lcm_util.set_gpio_mode(pin, mode)
#define dsi_lcm_set_gpio_dir(pin, dir)                       lcm_util.set_gpio_dir(pin, dir)
#define dsi_lcm_set_gpio_pull_enable(pin, en)                lcm_util.set_gpio_pull_enable(pin, en)

#define   LCM_DSI_CMD_MODE  0
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
extern void dct_pmic_VGP2_enable(bool dctEnable);
#endif

#if 0
void TC358768_DCS_write_1A_1P(unsigned char cmd, unsigned char para)
{
	unsigned int data_array[16];
#if 0//ndef BUILD_LK
	do {
		data_array[0] =(0x00001500 | (para<<24) | (cmd<<16));
		dsi_set_cmdq(data_array, 1, 1);
		if (cmd == 0xFF)
			break;
		read_reg_v2(cmd, &buffer, 1);
		if(buffer != para)
			printk("%s, data_array = 0x%08x, (cmd, para, back) = (0x%02x, 0x%02x, 0x%02x)\n", __func__, data_array[0], cmd, para, buffer);	
		MDELAY(1);
	} while (buffer != para);
#else
	data_array[0] =(0x00001500 | (para<<24) | (cmd<<16));
	dsi_set_cmdq(data_array, 1, 1);
#endif
}
#define TC358768_DCS_write_1A_0P(cmd)							data_array[0]=(0x00000500 | (cmd<<16)); \
																dsi_set_cmdq(data_array, 1, 1);																									
#endif
static void init_lcm_registers(void)
{
    unsigned int data_array[16];

    data_array[0] = 0x00110500;
    dsi_set_cmdq(data_array, 1, 1);

    MDELAY(120);

    data_array[0] = 0x00022902;
    data_array[1] = 0x000004B0;
    dsi_set_cmdq(&data_array, 2, 1);

    data_array[0] = 0x00072902; //b3 6 para
    data_array[1] = 0x000014B3;
    data_array[2] = 0x00000000;
    dsi_set_cmdq(&data_array, 3, 1);

    data_array[0] = 0x00032902;
    data_array[1] = 0x00B33AB6;//B33AB6
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00022902; ///0x00032902;
    data_array[1] = 0x000000C0;
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00232902;
    data_array[1] = 0x106084C1;
    data_array[2] = 0xCE6FFFEB;
    data_array[3] = 0x0217FFFF;
    data_array[4] = 0xB1AE7358;
    data_array[5] = 0xFFFFC620;
    data_array[6] = 0x5FFFF31F;
    data_array[7] = 0x10101010;
    data_array[8] = 0x22010200;
    data_array[9] = 0x00010022;
    dsi_set_cmdq(&data_array, 10, 1);

    data_array[0] = 0x00082902;
    data_array[1] = 0x80F731C2;
    data_array[2] = 0x0000080A;
    dsi_set_cmdq(&data_array, 3, 1);

    data_array[0] = 0x00172902;
    data_array[1] = 0x000070C4;
    data_array[2] = 0x00040000;
    data_array[3] = 0x060C0000;
    data_array[4] = 0x00000000;
    data_array[5] = 0x00000400;
    data_array[6] = 0x00060C00;
    dsi_set_cmdq(&data_array, 7, 1);

    data_array[0] = 0x00292902;
    data_array[1] = 0x006900C6;
    data_array[2] = 0x00690069;
    data_array[3] = 0x00000000;
    data_array[4] = 0x00690069;
    data_array[5] = 0x07191069;
    data_array[6] = 0x69000100;
    data_array[7] = 0x69006900;
    data_array[8] = 0x00000000;
    data_array[9] = 0x69006900;
    data_array[10] = 0x19106900;
    data_array[11] = 0x00000007;
    dsi_set_cmdq(&data_array, 12, 1);

    data_array[0] = 0x000A2902;
    data_array[1] = 0x3FFC31CB;
    data_array[2] = 0x0000008C;
    data_array[3] = 0x0000C000;
    dsi_set_cmdq(&data_array, 4, 1);

    data_array[0] = 0x00022902;
    data_array[1] = 0x00000BCC;
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x000B2902;
    data_array[1] = 0xBB8111D0;
    data_array[2] = 0x194C1E1E;
    data_array[3] = 0x00000C19;
    dsi_set_cmdq(&data_array, 4, 1); //dsi_set_cmdq(&data_array, 3, 1);

    data_array[0] = 0x001A2902;
    data_array[1] = 0xBB331BD3;
    data_array[2] = 0x3333B3BB;
    data_array[3] = 0x00010033;
    data_array[4] = 0x0DA0D8A0;
    data_array[5] = 0x3B334E4E;
    data_array[6] = 0x3D077222;
    data_array[7] = 0x000033BF;
    dsi_set_cmdq(&data_array, 8, 1);

    data_array[0] = 0x00082902;
    data_array[1] = 0x000006D5;
    data_array[2] = 0x32015101;
    dsi_set_cmdq(&data_array, 3, 1); //dsi_set_cmdq(&data_array, 2, 1);

    data_array[0] = 0x001f2902;   //0x001e2902;
    data_array[1] = 0x110A01C7;
    data_array[2] = 0x3E332618;
    data_array[3] = 0x52423850;
    data_array[4] = 0x776E6760;
    data_array[5] = 0x18110A01;
    data_array[6] = 0x503E3326;
    data_array[7] = 0x60524238;
    data_array[8] = 0x00776E67;
    dsi_set_cmdq(&data_array, 9, 1);


    data_array[0] = 0x00142902;
    data_array[1] = 0x000001C8;
    data_array[2] = 0x00FC0000;
    data_array[3] = 0x00000000;
    data_array[4] = 0x000000FC;
    data_array[5] = 0x00FC0000;
    dsi_set_cmdq(&data_array, 6, 1);

    data_array[0] = 0x00351500;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00022902;
    data_array[1] = 0x000001B7;
    dsi_set_cmdq(&data_array, 2, 1);


    data_array[0] = 0x00290500;
    dsi_set_cmdq(data_array, 1, 1);

    MDELAY(10);
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

        // enable tearing-free
    //    params->dbi.te_mode                 = LCM_DBI_TE_MODE_VSYNC_ONLY;
    //    params->dbi.te_edge_polarity        = LCM_POLARITY_RISING;

        #if (LCM_DSI_CMD_MODE)
        params->dsi.mode   = CMD_MODE;
        #else
        params->dsi.mode   = BURST_VDO_MODE;
        #endif

        // DSI
        /* Command mode setting */
        //1 Three lane or Four lane
        params->dsi.LANE_NUM                = LCM_FOUR_LANE;
        //The following defined the fomat for data coming from LCD engine.
        params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

        // Highly depends on LCD driver capability.
        // Not support in MT6573

        // Video mode setting

        params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    //    params->dsi.word_count=720*3;

        params->dsi.vertical_sync_active      = 4;//2;
        params->dsi.vertical_backporch        = 4;//1;
        params->dsi.vertical_frontporch       = 4;//1;
        params->dsi.vertical_active_line      = FRAME_HEIGHT;

		params->dsi.horizontal_sync_active				=8; //0x12;// 50  2
		params->dsi.horizontal_backporch				=60; //0x5f;
		params->dsi.horizontal_frontporch				=100;// 0x5f;
        params->dsi.horizontal_active_pixel   = FRAME_WIDTH;

        // Bit rate calculation
        params->dsi.PLL_CLOCK = 370;//148;
        //1 Every lane speed
        params->dsi.pll_div1=0;        // div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
        params->dsi.pll_div2=0;        // div2=0,1,2,3;div1_real=1,2,4,4
        params->dsi.fbk_div =0x11;      // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)

}

static void lcm_init(void)
{
#ifdef BUILD_LK
	printf("%s, LK\n",__func__);
#else
	printk("%s, kernel\n",__func__);
#endif
	lcm_is_init = true;
	dct_pmic_VGP2_enable(1);
        MDELAY(10);
	mt_set_gpio_mode(GPIO112,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO112,GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
    MDELAY(5);

    //SET_RESET_PIN(1);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);
    MDELAY(5);
    //SET_RESET_PIN(0);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
    MDELAY(5);
    //SET_RESET_PIN(1);
        mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);  
    MDELAY(5);

        mt_set_gpio_out(GPIO112,GPIO_OUT_ONE); 
        MDELAY(20);
        TC358768_DCS_write_1A_1P(0xFF, 0xEE);
        TC358768_DCS_write_1A_1P(0xFB, 0x01);
        TC358768_DCS_write_1A_1P(0x18, 0x40);
        MDELAY(10);
        TC358768_DCS_write_1A_1P(0X18, 0X00);
        MDELAY(20);  
    init_lcm_registers();
}

#if 0
void lcm_reset(void)
{
	mt_set_gpio_mode(GPIO112,GPIO_MODE_00);
	mt_set_gpio_dir(GPIO112,GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);
	MDELAY(120);
}
#endif

extern void DSI_clk_ULP_mode(bool enter);
extern void DSI_lane0_ULP_mode(bool enter);

static void DSI_Enter_ULPM(bool enter)
{
	DSI_clk_ULP_mode(enter);  //enter ULPM
	DSI_lane0_ULP_mode(enter);
}

static void lcm_suspend(void)
{
    unsigned int data_array[16];

#ifdef BUILD_LK
	printf("%s, LK\n",__func__);
#else
	printk("%s, kernel\n",__func__);
#endif
    data_array[0]=0x00280500; // Display Off
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00100500; // Sleep In
    dsi_set_cmdq(data_array, 1, 1);
	DSI_Enter_ULPM(1); /* Enter low power mode  */

    //SET_RESET_PIN(0);
	MDELAY(60);
    //SET_RESET_PIN(1);
	mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);
	MDELAY(150);
	dct_pmic_VGP2_enable(0); /* [Ted 5-28] Disable VCI power to prevent lcd polarization */
	lcm_is_init = false;
}

static void lcm_resume(void)
{
//	unsigned int data_array[16];
	//unsigned char buffer[2];
	if(!lcm_is_init)
        lcm_init();

#ifdef BUILD_LK
	printf("%s, LK\n",__func__);
#else
	printk("%s, kernel\n",__func__);
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

//yangqingbing delete it for patch69 for CMD  mode
/*    data_array[0]= 0x00290508;//HW bug, so need send one HS packet
    dsi_set_cmdq(data_array, 1, 1);
*/

    data_array[0]= 0x002c3909;
    dsi_set_cmdq(data_array, 1, 0);

}
#endif

static unsigned int lcm_compare_id(void)
{
    unsigned char buffer[5] = {0};
    unsigned int data_array[16];

    dct_pmic_VGP2_enable(1);
    MDELAY(5);

    //SET_RESET_PIN(1);
    mt_set_gpio_mode(GPIO112,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO112,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
    MDELAY(5);
    //SET_RESET_PIN(0);
    mt_set_gpio_out(GPIO112,GPIO_OUT_ZERO);
    MDELAY(5);
    //SET_RESET_PIN(1);
    mt_set_gpio_out(GPIO112,GPIO_OUT_ONE);
    MDELAY(5);

    data_array[0] = 0x00053700;// read id return 1 byte
    dsi_set_cmdq(&data_array, 1, 1);
    read_reg_v2(0xBF, buffer, 5);
#ifdef BUILD_LK
	printf("r63315 compare_id buf[0]=0x%x,buf[1]=0x%x,buf[2]=0x%x,buf[3]=0x%x,buf[4]=0x%x\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4]);
#else
	printk("r63315 compare_id buf[0]=0x%x,buf[1]=0x%x,buf[2]=0x%x,buf[3]=0x%x,buf[4]=0x%x\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4]);
#endif	

    if ( (0x33 == buffer[2]) && (0x15 == buffer[3]) )
	{
	    return 1;
	}

	return 0;
}
#ifndef BUILD_LK
static unsigned int lcm_esd_check(void)
{
    unsigned int ret[3]={0};
    unsigned char buffer[3];
    unsigned int data_array[16];


    data_array[0] = 0x00013700;// read id return 1 byte
    dsi_set_cmdq(&data_array, 1, 1);

    read_reg_v2(0x05, buffer, 1);
    ret[0]= buffer[0];

    data_array[0] = 0x00013700;// read id return 1 byte
    dsi_set_cmdq(&data_array, 1, 1);

    read_reg_v2(0x0A, buffer, 1);
    ret[1]= buffer[0];

    data_array[0] = 0x00013700;// read id return 1 byte
    dsi_set_cmdq(&data_array, 1, 1);

    read_reg_v2(0x0E, buffer, 1);
    ret[2]= buffer[0];

    LCM_DEBUG("LCM REG 0x05,0x0A,0x0E is 0x%x,0x%x,0x%x\n",ret[0],ret[1],ret[2]);

    if(ret[0]!=0||ret[1]!=0x1c)
    {
        LCM_DEBUG("lCM esd\n");
        return 1;

    }
    else
        return 0;
}


static unsigned int lcm_esd_recover(void)
{
    LCM_DEBUG("start :%s\n",__func__);

    lcm_init();


    LCM_DEBUG("esd recover ends\n");

    return 1;
}
#endif

LCM_DRIVER r63315_fhd_dsi_vdo_truly_lcm_drv =
{
    .name            = "r63315_fhd_dsi_vdo_truly",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif

#ifndef BUILD_LK

    .esd_check      = lcm_esd_check,
    .esd_recover    = lcm_esd_recover,
#endif
};

