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
#define FRAME_WIDTH  (1080)
#define FRAME_HEIGHT (1920)

#define LCM_ID_NT35596 (0x96)
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)      lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                        lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                    lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                            lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)                   lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define dsi_lcm_set_gpio_out(pin, out)                                        lcm_util.set_gpio_out(pin, out)
#define dsi_lcm_set_gpio_mode(pin, mode)                                    lcm_util.set_gpio_mode(pin, mode)
#define dsi_lcm_set_gpio_dir(pin, dir)                                        lcm_util.set_gpio_dir(pin, dir)
#define dsi_lcm_set_gpio_pull_enable(pin, en)                                lcm_util.set_gpio_pull_enable(pin, en)

#define   LCM_DSI_CMD_MODE  0

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

extern void DSI_clk_ULP_mode(bool enter);
extern void DSI_lane0_ULP_mode(bool enter);

static void DSI_Enter_ULPM(bool enter)
{
	DSI_clk_ULP_mode(enter);  //enter ULPM
	DSI_lane0_ULP_mode(enter);
}

static bool lcm_is_init = false;

extern void TC358768_DCS_write_1A_1P(unsigned char cmd, unsigned char para);

#define TC358768_DCS_write_1A_0P(cmd)    data_array[0]=(0x00000500 | (cmd<<16));\
             dsi_set_cmdq(data_array, 1, 1);

static void init_lcm_registers(void)
{
    unsigned int data_array[16];

    TC358768_DCS_write_1A_1P(0xFF,0x05);
    TC358768_DCS_write_1A_1P(0xFB,0x01);
    TC358768_DCS_write_1A_1P(0xC5,0x01);
    MDELAY(150);

//CMD2 Page 0  
    TC358768_DCS_write_1A_1P(0xFF, 0x01);
    MDELAY(10);
    TC358768_DCS_write_1A_1P(0XFB    ,0x01);
//Power-Related  
    TC358768_DCS_write_1A_1P(0X00    ,0x01);
    TC358768_DCS_write_1A_1P(0X01    ,0x55);
    TC358768_DCS_write_1A_1P(0X02    ,0x45);
    TC358768_DCS_write_1A_1P(0X03    ,0x55);
    TC358768_DCS_write_1A_1P(0X05    ,0x50);

//VGH/VGL w/Clamp +/- 9V 
    TC358768_DCS_write_1A_1P(0X14    ,0x9E);
    TC358768_DCS_write_1A_1P(0X07    ,0xA8);
    TC358768_DCS_write_1A_1P(0X08    ,0x0C);

//Gamma Voltage +/-4.35V
    TC358768_DCS_write_1A_1P(0X0B    ,0x96);
    TC358768_DCS_write_1A_1P(0X0C    ,0x96);

//VGHO/VGLO Disable
    TC358768_DCS_write_1A_1P(0X0E    ,0x00);
    TC358768_DCS_write_1A_1P(0X0F    ,0x00);

    TC358768_DCS_write_1A_1P(0X11    ,0x20); // 0x27
    TC358768_DCS_write_1A_1P(0X12    ,0x20); // 0x27
    TC358768_DCS_write_1A_1P(0X13    ,0x03);
    TC358768_DCS_write_1A_1P(0X06    ,0x0A);
    TC358768_DCS_write_1A_1P(0X15    ,0x99);
    TC358768_DCS_write_1A_1P(0X16    ,0x99);
    TC358768_DCS_write_1A_1P(0X1B    ,0x39);
    TC358768_DCS_write_1A_1P(0X1C    ,0x39);
    TC358768_DCS_write_1A_1P(0X1D    ,0x47);

//ID code  
    TC358768_DCS_write_1A_1P(0X44    ,0xC1);
    TC358768_DCS_write_1A_1P(0X45    ,0x86);
    TC358768_DCS_write_1A_1P(0X46    ,0xC4);

//Gate EQ 
    TC358768_DCS_write_1A_1P(0X58    ,0x05);
    TC358768_DCS_write_1A_1P(0X59    ,0x05);
    TC358768_DCS_write_1A_1P(0X5A    ,0x05);
    TC358768_DCS_write_1A_1P(0X5B    ,0x05);
    TC358768_DCS_write_1A_1P(0X5C    ,0x00);
    TC358768_DCS_write_1A_1P(0X5D    ,0x00);
    TC358768_DCS_write_1A_1P(0X5E    ,0x00);
    TC358768_DCS_write_1A_1P(0X5F    ,0x00);

//ISOP P/N (Normal Mode)   
    TC358768_DCS_write_1A_1P(0X6D    ,0x44);

//Gamma R +/-
    TC358768_DCS_write_1A_1P( 0x75,0x00);
    TC358768_DCS_write_1A_1P( 0x76,0x00);
    TC358768_DCS_write_1A_1P( 0x77,0x00);
    TC358768_DCS_write_1A_1P( 0x78,0x22);
    TC358768_DCS_write_1A_1P( 0x79,0x00);
    TC358768_DCS_write_1A_1P( 0x7A,0x46);
    TC358768_DCS_write_1A_1P( 0x7B,0x00);
    TC358768_DCS_write_1A_1P( 0x7C,0x5C);
    TC358768_DCS_write_1A_1P( 0x7D,0x00);
    TC358768_DCS_write_1A_1P( 0x7E,0x76);
    TC358768_DCS_write_1A_1P( 0x7F,0x00);
    TC358768_DCS_write_1A_1P( 0x80,0x8D);
    TC358768_DCS_write_1A_1P( 0x81,0x00);
    TC358768_DCS_write_1A_1P( 0x82,0xA6);
    TC358768_DCS_write_1A_1P( 0x83,0x00);
    TC358768_DCS_write_1A_1P( 0x84,0xB8);
    TC358768_DCS_write_1A_1P( 0x85,0x00);
    TC358768_DCS_write_1A_1P( 0x86,0xC7);
    TC358768_DCS_write_1A_1P( 0x87,0x00);
    TC358768_DCS_write_1A_1P( 0x88,0xF6);
    TC358768_DCS_write_1A_1P( 0x89,0x01);
    TC358768_DCS_write_1A_1P( 0x8A,0x1D);
    TC358768_DCS_write_1A_1P( 0x8B,0x01);
    TC358768_DCS_write_1A_1P( 0x8C,0x54);
    TC358768_DCS_write_1A_1P( 0x8D,0x01);
    TC358768_DCS_write_1A_1P( 0x8E,0x81);
    TC358768_DCS_write_1A_1P( 0x8F,0x01);
    TC358768_DCS_write_1A_1P( 0x90,0xCB);
    TC358768_DCS_write_1A_1P( 0x91,0x02);
    TC358768_DCS_write_1A_1P( 0x92,0x05);
    TC358768_DCS_write_1A_1P( 0x93,0x02);
    TC358768_DCS_write_1A_1P( 0x94,0x07);
    TC358768_DCS_write_1A_1P( 0x95,0x02);
    TC358768_DCS_write_1A_1P( 0x96,0x47);
    TC358768_DCS_write_1A_1P( 0x97,0x02);
    TC358768_DCS_write_1A_1P( 0x98,0x82);
    TC358768_DCS_write_1A_1P( 0x99,0x02);
    TC358768_DCS_write_1A_1P( 0x9A,0xAB);
    TC358768_DCS_write_1A_1P( 0x9B,0x02);
    TC358768_DCS_write_1A_1P( 0x9C,0xDC);
    TC358768_DCS_write_1A_1P( 0x9D,0x03);
    TC358768_DCS_write_1A_1P( 0x9E,0x01);
    TC358768_DCS_write_1A_1P( 0x9F,0x03);
    TC358768_DCS_write_1A_1P( 0xA0,0x3A);
    TC358768_DCS_write_1A_1P( 0xA2,0x03);
    TC358768_DCS_write_1A_1P( 0xA3,0x56);
    TC358768_DCS_write_1A_1P( 0xA4,0x03);
    TC358768_DCS_write_1A_1P( 0xA5,0x6D);
    TC358768_DCS_write_1A_1P( 0xA6,0x03);
    TC358768_DCS_write_1A_1P( 0xA7,0x89);
    TC358768_DCS_write_1A_1P( 0xA9,0x03);
    TC358768_DCS_write_1A_1P( 0xAA,0xA3);
    TC358768_DCS_write_1A_1P( 0xAB,0x03);
    TC358768_DCS_write_1A_1P( 0xAC,0xC9);
    TC358768_DCS_write_1A_1P( 0xAD,0x03);
    TC358768_DCS_write_1A_1P( 0xAE,0xDD);
    TC358768_DCS_write_1A_1P( 0xAF,0x03);
    TC358768_DCS_write_1A_1P( 0xB0,0xF5);
    TC358768_DCS_write_1A_1P( 0xB1,0x03);
    TC358768_DCS_write_1A_1P( 0xB2,0xFF);
    TC358768_DCS_write_1A_1P( 0xB3,0x00);
    TC358768_DCS_write_1A_1P( 0xB4,0x00);
    TC358768_DCS_write_1A_1P( 0xB5,0x00);
    TC358768_DCS_write_1A_1P( 0xB6,0x22);
    TC358768_DCS_write_1A_1P( 0xB7,0x00);
    TC358768_DCS_write_1A_1P( 0xB8,0x46);
    TC358768_DCS_write_1A_1P( 0xB9,0x00);
    TC358768_DCS_write_1A_1P( 0xBA,0x5C);
    TC358768_DCS_write_1A_1P( 0xBB,0x00);
    TC358768_DCS_write_1A_1P( 0xBC,0x76);
    TC358768_DCS_write_1A_1P( 0xBD,0x00);
    TC358768_DCS_write_1A_1P( 0xBE,0x8D);
    TC358768_DCS_write_1A_1P( 0xBF,0x00);
    TC358768_DCS_write_1A_1P( 0xC0,0xA6);
    TC358768_DCS_write_1A_1P( 0xC1,0x00);
    TC358768_DCS_write_1A_1P( 0xC2,0xB8);
    TC358768_DCS_write_1A_1P( 0xC3,0x00);
    TC358768_DCS_write_1A_1P( 0xC4,0xC7);
    TC358768_DCS_write_1A_1P( 0xC5,0x00);
    TC358768_DCS_write_1A_1P( 0xC6,0xF6);
    TC358768_DCS_write_1A_1P( 0xC7,0x01);
    TC358768_DCS_write_1A_1P( 0xC8,0x1D);
    TC358768_DCS_write_1A_1P( 0xC9,0x01);
    TC358768_DCS_write_1A_1P( 0xCA,0x54);
    TC358768_DCS_write_1A_1P( 0xCB,0x01);
    TC358768_DCS_write_1A_1P( 0xCC,0x81);
    TC358768_DCS_write_1A_1P( 0xCD,0x01);
    TC358768_DCS_write_1A_1P( 0xCE,0xCB);
    TC358768_DCS_write_1A_1P( 0xCF,0x02);
    TC358768_DCS_write_1A_1P( 0xD0,0x05);
    TC358768_DCS_write_1A_1P( 0xD1,0x02);
    TC358768_DCS_write_1A_1P( 0xD2,0x07);
    TC358768_DCS_write_1A_1P( 0xD3,0x02);
    TC358768_DCS_write_1A_1P( 0xD4,0x47);
    TC358768_DCS_write_1A_1P( 0xD5,0x02);
    TC358768_DCS_write_1A_1P( 0xD6,0x82);
    TC358768_DCS_write_1A_1P( 0xD7,0x02);
    TC358768_DCS_write_1A_1P( 0xD8,0xAB);
    TC358768_DCS_write_1A_1P( 0xD9,0x02);
    TC358768_DCS_write_1A_1P( 0xDA,0xDC);
    TC358768_DCS_write_1A_1P( 0xDB,0x03);
    TC358768_DCS_write_1A_1P( 0xDC,0x01);
    TC358768_DCS_write_1A_1P( 0xDD,0x03);
    TC358768_DCS_write_1A_1P( 0xDE,0x3A);
    TC358768_DCS_write_1A_1P( 0xDF,0x03);
    TC358768_DCS_write_1A_1P( 0xE0,0x56);
    TC358768_DCS_write_1A_1P( 0xE1,0x03);
    TC358768_DCS_write_1A_1P( 0xE2,0x6D);
    TC358768_DCS_write_1A_1P( 0xE3,0x03);
    TC358768_DCS_write_1A_1P( 0xE4,0x89);
    TC358768_DCS_write_1A_1P( 0xE5,0x03);
    TC358768_DCS_write_1A_1P( 0xE6,0xA3);
    TC358768_DCS_write_1A_1P( 0xE7,0x03);
    TC358768_DCS_write_1A_1P( 0xE8,0xC9);
    TC358768_DCS_write_1A_1P( 0xE9,0x03);
    TC358768_DCS_write_1A_1P( 0xEA,0xDD);
    TC358768_DCS_write_1A_1P( 0xEB,0x03);
    TC358768_DCS_write_1A_1P( 0xEC,0xF5);
    TC358768_DCS_write_1A_1P( 0xED,0x03);
    TC358768_DCS_write_1A_1P( 0xEE,0xFF);

//Gamma G+
    TC358768_DCS_write_1A_1P( 0xEF,0x00);
    TC358768_DCS_write_1A_1P( 0xF0,0x00);
    TC358768_DCS_write_1A_1P( 0xF1,0x00);
    TC358768_DCS_write_1A_1P( 0xF2,0x22);
    TC358768_DCS_write_1A_1P( 0xF3,0x00);
    TC358768_DCS_write_1A_1P( 0xF4,0x46);
    TC358768_DCS_write_1A_1P( 0xF5,0x00);
    TC358768_DCS_write_1A_1P( 0xF6,0x5C);
    TC358768_DCS_write_1A_1P( 0xF7,0x00);
    TC358768_DCS_write_1A_1P( 0xF8,0x76);
    TC358768_DCS_write_1A_1P( 0xF9,0x00);
    TC358768_DCS_write_1A_1P( 0xFA,0x8D);

//CMD2 Page 1
    TC358768_DCS_write_1A_1P( 0xFF,0x02);
    MDELAY(10);
    TC358768_DCS_write_1A_1P( 0xFB,0x01);

//Gamma G+/-
    TC358768_DCS_write_1A_1P( 0x00,0x00);
    TC358768_DCS_write_1A_1P( 0x01,0xA6);
    TC358768_DCS_write_1A_1P( 0x02,0x00);
    TC358768_DCS_write_1A_1P( 0x03,0xB8);
    TC358768_DCS_write_1A_1P( 0x04,0x00);
    TC358768_DCS_write_1A_1P( 0x05,0xC7);
    TC358768_DCS_write_1A_1P( 0x06,0x00);
    TC358768_DCS_write_1A_1P( 0x07,0xF6);
    TC358768_DCS_write_1A_1P( 0x08,0x01);
    TC358768_DCS_write_1A_1P( 0x09,0x1D);
    TC358768_DCS_write_1A_1P( 0x0A,0x01);
    TC358768_DCS_write_1A_1P( 0x0B,0x54);
    TC358768_DCS_write_1A_1P( 0x0C,0x01);
    TC358768_DCS_write_1A_1P( 0x0D,0x81);
    TC358768_DCS_write_1A_1P( 0x0E,0x01);
    TC358768_DCS_write_1A_1P( 0x0F,0xCB);
    TC358768_DCS_write_1A_1P( 0x10,0x02);
    TC358768_DCS_write_1A_1P( 0x11,0x05);
    TC358768_DCS_write_1A_1P( 0x12,0x02);
    TC358768_DCS_write_1A_1P( 0x13,0x07);
    TC358768_DCS_write_1A_1P( 0x14,0x02);
    TC358768_DCS_write_1A_1P( 0x15,0x47);
    TC358768_DCS_write_1A_1P( 0x16,0x02);
    TC358768_DCS_write_1A_1P( 0x17,0x82);
    TC358768_DCS_write_1A_1P( 0x18,0x02);
    TC358768_DCS_write_1A_1P( 0x19,0xAB);
    TC358768_DCS_write_1A_1P( 0x1A,0x02);
    TC358768_DCS_write_1A_1P( 0x1B,0xDC);
    TC358768_DCS_write_1A_1P( 0x1C,0x03);
    TC358768_DCS_write_1A_1P( 0x1D,0x01);
    TC358768_DCS_write_1A_1P( 0x1E,0x03);
    TC358768_DCS_write_1A_1P( 0x1F,0x3A);
    TC358768_DCS_write_1A_1P( 0x20,0x03);
    TC358768_DCS_write_1A_1P( 0x21,0x56);
    TC358768_DCS_write_1A_1P( 0x22,0x03);
    TC358768_DCS_write_1A_1P( 0x23,0x6D);
    TC358768_DCS_write_1A_1P( 0x24,0x03);
    TC358768_DCS_write_1A_1P( 0x25,0x89);
    TC358768_DCS_write_1A_1P( 0x26,0x03);
    TC358768_DCS_write_1A_1P( 0x27,0xA3);
    TC358768_DCS_write_1A_1P( 0x28,0x03);
    TC358768_DCS_write_1A_1P( 0x29,0xC9);
    TC358768_DCS_write_1A_1P( 0x2A,0x03);
    TC358768_DCS_write_1A_1P( 0x2B,0xDD);
    TC358768_DCS_write_1A_1P( 0x2D,0x03);
    TC358768_DCS_write_1A_1P( 0x2F,0xF5);
    TC358768_DCS_write_1A_1P( 0x30,0x03);
    TC358768_DCS_write_1A_1P( 0x31,0xFF);
    TC358768_DCS_write_1A_1P( 0x32,0x00);
    TC358768_DCS_write_1A_1P( 0x33,0x00);
    TC358768_DCS_write_1A_1P( 0x34,0x00);
    TC358768_DCS_write_1A_1P( 0x35,0x22);
    TC358768_DCS_write_1A_1P( 0x36,0x00);
    TC358768_DCS_write_1A_1P( 0x37,0x46);
    TC358768_DCS_write_1A_1P( 0x38,0x00);
    TC358768_DCS_write_1A_1P( 0x39,0x5C);
    TC358768_DCS_write_1A_1P( 0x3A,0x00);
    TC358768_DCS_write_1A_1P( 0x3B,0x76);
    TC358768_DCS_write_1A_1P( 0x3D,0x00);
    TC358768_DCS_write_1A_1P( 0x3F,0x8D);
    TC358768_DCS_write_1A_1P( 0x40,0x00);
    TC358768_DCS_write_1A_1P( 0x41,0xA6);
    TC358768_DCS_write_1A_1P( 0x42,0x00);
    TC358768_DCS_write_1A_1P( 0x43,0xB8);
    TC358768_DCS_write_1A_1P( 0x44,0x00);
    TC358768_DCS_write_1A_1P( 0x45,0xC7);
    TC358768_DCS_write_1A_1P( 0x46,0x00);
    TC358768_DCS_write_1A_1P( 0x47,0xF6);
    TC358768_DCS_write_1A_1P( 0x48,0x01);
    TC358768_DCS_write_1A_1P( 0x49,0x1D);
    TC358768_DCS_write_1A_1P( 0x4A,0x01);
    TC358768_DCS_write_1A_1P( 0x4B,0x54);
    TC358768_DCS_write_1A_1P( 0x4C,0x01);
    TC358768_DCS_write_1A_1P( 0x4D,0x81);
    TC358768_DCS_write_1A_1P( 0x4E,0x01);
    TC358768_DCS_write_1A_1P( 0x4F,0xCB);
    TC358768_DCS_write_1A_1P( 0x50,0x02);
    TC358768_DCS_write_1A_1P( 0x51,0x05);
    TC358768_DCS_write_1A_1P( 0x52,0x02);
    TC358768_DCS_write_1A_1P( 0x53,0x07);
    TC358768_DCS_write_1A_1P( 0x54,0x02);
    TC358768_DCS_write_1A_1P( 0x55,0x47);
    TC358768_DCS_write_1A_1P( 0x56,0x02);
    TC358768_DCS_write_1A_1P( 0x58,0x82);
    TC358768_DCS_write_1A_1P( 0x59,0x02);
    TC358768_DCS_write_1A_1P( 0x5A,0xAB);
    TC358768_DCS_write_1A_1P( 0x5B,0x02);
    TC358768_DCS_write_1A_1P( 0x5C,0xDC);
    TC358768_DCS_write_1A_1P( 0x5D,0x03);
    TC358768_DCS_write_1A_1P( 0x5E,0x01);
    TC358768_DCS_write_1A_1P( 0x5F,0x03);
    TC358768_DCS_write_1A_1P( 0x60,0x3A);
    TC358768_DCS_write_1A_1P( 0x61,0x03);
    TC358768_DCS_write_1A_1P( 0x62,0x56);
    TC358768_DCS_write_1A_1P( 0x63,0x03);
    TC358768_DCS_write_1A_1P( 0x64,0x6D);
    TC358768_DCS_write_1A_1P( 0x65,0x03);
    TC358768_DCS_write_1A_1P( 0x66,0x89);
    TC358768_DCS_write_1A_1P( 0x67,0x03);
    TC358768_DCS_write_1A_1P( 0x68,0xA3);
    TC358768_DCS_write_1A_1P( 0x69,0x03);
    TC358768_DCS_write_1A_1P( 0x6A,0xC9);
    TC358768_DCS_write_1A_1P( 0x6B,0x03);
    TC358768_DCS_write_1A_1P( 0x6C,0xDD);
    TC358768_DCS_write_1A_1P( 0x6D,0x03);
    TC358768_DCS_write_1A_1P( 0x6E,0xF5);
    TC358768_DCS_write_1A_1P( 0x6F,0x03);
    TC358768_DCS_write_1A_1P( 0x70,0xFF);

//Gamma B+/-
    TC358768_DCS_write_1A_1P( 0x71,0x00);
    TC358768_DCS_write_1A_1P( 0x72,0x00);
    TC358768_DCS_write_1A_1P( 0x73,0x00);
    TC358768_DCS_write_1A_1P( 0x74,0x22);
    TC358768_DCS_write_1A_1P( 0x75,0x00);
    TC358768_DCS_write_1A_1P( 0x76,0x46);
    TC358768_DCS_write_1A_1P( 0x77,0x00);
    TC358768_DCS_write_1A_1P( 0x78,0x5C);
    TC358768_DCS_write_1A_1P( 0x79,0x00);
    TC358768_DCS_write_1A_1P( 0x7A,0x76);
    TC358768_DCS_write_1A_1P( 0x7B,0x00);
    TC358768_DCS_write_1A_1P( 0x7C,0x8D);
    TC358768_DCS_write_1A_1P( 0x7D,0x00);
    TC358768_DCS_write_1A_1P( 0x7E,0xA6);
    TC358768_DCS_write_1A_1P( 0x7F,0x00);
    TC358768_DCS_write_1A_1P( 0x80,0xB8);
    TC358768_DCS_write_1A_1P( 0x81,0x00);
    TC358768_DCS_write_1A_1P( 0x82,0xC7);
    TC358768_DCS_write_1A_1P( 0x83,0x00);
    TC358768_DCS_write_1A_1P( 0x84,0xF6);
    TC358768_DCS_write_1A_1P( 0x85,0x01);
    TC358768_DCS_write_1A_1P( 0x86,0x1D);
    TC358768_DCS_write_1A_1P( 0x87,0x01);
    TC358768_DCS_write_1A_1P( 0x88,0x54);
    TC358768_DCS_write_1A_1P( 0x89,0x01);
    TC358768_DCS_write_1A_1P( 0x8A,0x81);
    TC358768_DCS_write_1A_1P( 0x8B,0x01);
    TC358768_DCS_write_1A_1P( 0x8C,0xCB);
    TC358768_DCS_write_1A_1P( 0x8D,0x02);
    TC358768_DCS_write_1A_1P( 0x8E,0x05);
    TC358768_DCS_write_1A_1P( 0x8F,0x02);
    TC358768_DCS_write_1A_1P( 0x90,0x07);
    TC358768_DCS_write_1A_1P( 0x91,0x02);
    TC358768_DCS_write_1A_1P( 0x92,0x47);
    TC358768_DCS_write_1A_1P( 0x93,0x02);
    TC358768_DCS_write_1A_1P( 0x94,0x82);
    TC358768_DCS_write_1A_1P( 0x95,0x02);
    TC358768_DCS_write_1A_1P( 0x96,0xAB);
    TC358768_DCS_write_1A_1P( 0x97,0x02);
    TC358768_DCS_write_1A_1P( 0x98,0xDC);
    TC358768_DCS_write_1A_1P( 0x99,0x03);
    TC358768_DCS_write_1A_1P( 0x9A,0x01);
    TC358768_DCS_write_1A_1P( 0x9B,0x03);
    TC358768_DCS_write_1A_1P( 0x9C,0x3A);
    TC358768_DCS_write_1A_1P( 0x9D,0x03);
    TC358768_DCS_write_1A_1P( 0x9E,0x56);
    TC358768_DCS_write_1A_1P( 0x9F,0x03);
    TC358768_DCS_write_1A_1P( 0xA0,0x6D);
    TC358768_DCS_write_1A_1P( 0xA2,0x03);
    TC358768_DCS_write_1A_1P( 0xA3,0x89);
    TC358768_DCS_write_1A_1P( 0xA4,0x03);
    TC358768_DCS_write_1A_1P( 0xA5,0xA3);
    TC358768_DCS_write_1A_1P( 0xA6,0x03);
    TC358768_DCS_write_1A_1P( 0xA7,0xC9);
    TC358768_DCS_write_1A_1P( 0xA9,0x03);
    TC358768_DCS_write_1A_1P( 0xAA,0xDD);
    TC358768_DCS_write_1A_1P( 0xAB,0x03);
    TC358768_DCS_write_1A_1P( 0xAC,0xF5);
    TC358768_DCS_write_1A_1P( 0xAD,0x03);
    TC358768_DCS_write_1A_1P( 0xAE,0xFF);
    TC358768_DCS_write_1A_1P( 0xAF,0x00);
    TC358768_DCS_write_1A_1P( 0xB0,0x00);
    TC358768_DCS_write_1A_1P( 0xB1,0x00);
    TC358768_DCS_write_1A_1P( 0xB2,0x22);
    TC358768_DCS_write_1A_1P( 0xB3,0x00);
    TC358768_DCS_write_1A_1P( 0xB4,0x46);
    TC358768_DCS_write_1A_1P( 0xB5,0x00);
    TC358768_DCS_write_1A_1P( 0xB6,0x5C);
    TC358768_DCS_write_1A_1P( 0xB7,0x00);
    TC358768_DCS_write_1A_1P( 0xB8,0x76);
    TC358768_DCS_write_1A_1P( 0xB9,0x00);
    TC358768_DCS_write_1A_1P( 0xBA,0x8D);
    TC358768_DCS_write_1A_1P( 0xBB,0x00);
    TC358768_DCS_write_1A_1P( 0xBC,0xA6);
    TC358768_DCS_write_1A_1P( 0xBD,0x00);
    TC358768_DCS_write_1A_1P( 0xBE,0xB8);
    TC358768_DCS_write_1A_1P( 0xBF,0x00);
    TC358768_DCS_write_1A_1P( 0xC0,0xC7);
    TC358768_DCS_write_1A_1P( 0xC1,0x00);
    TC358768_DCS_write_1A_1P( 0xC2,0xF6);
    TC358768_DCS_write_1A_1P( 0xC3,0x01);
    TC358768_DCS_write_1A_1P( 0xC4,0x1D);
    TC358768_DCS_write_1A_1P( 0xC5,0x01);
    TC358768_DCS_write_1A_1P( 0xC6,0x54);
    TC358768_DCS_write_1A_1P( 0xC7,0x01);
    TC358768_DCS_write_1A_1P( 0xC8,0x81);
    TC358768_DCS_write_1A_1P( 0xC9,0x01);
    TC358768_DCS_write_1A_1P( 0xCA,0xCB);
    TC358768_DCS_write_1A_1P( 0xCB,0x02);
    TC358768_DCS_write_1A_1P( 0xCC,0x05);
    TC358768_DCS_write_1A_1P( 0xCD,0x02);
    TC358768_DCS_write_1A_1P( 0xCE,0x07);
    TC358768_DCS_write_1A_1P( 0xCF,0x02);
    TC358768_DCS_write_1A_1P( 0xD0,0x47);
    TC358768_DCS_write_1A_1P( 0xD1,0x02);
    TC358768_DCS_write_1A_1P( 0xD2,0x82);
    TC358768_DCS_write_1A_1P( 0xD3,0x02);
    TC358768_DCS_write_1A_1P( 0xD4,0xAB);
    TC358768_DCS_write_1A_1P( 0xD5,0x02);
    TC358768_DCS_write_1A_1P( 0xD6,0xDC);
    TC358768_DCS_write_1A_1P( 0xD7,0x03);
    TC358768_DCS_write_1A_1P( 0xD8,0x01);
    TC358768_DCS_write_1A_1P( 0xD9,0x03);
    TC358768_DCS_write_1A_1P( 0xDA,0x3A);
    TC358768_DCS_write_1A_1P( 0xDB,0x03);
    TC358768_DCS_write_1A_1P( 0xDC,0x56);
    TC358768_DCS_write_1A_1P( 0xDD,0x03);
    TC358768_DCS_write_1A_1P( 0xDE,0x6D);
    TC358768_DCS_write_1A_1P( 0xDF,0x03);
    TC358768_DCS_write_1A_1P( 0xE0,0x89);
    TC358768_DCS_write_1A_1P( 0xE1,0x03);
    TC358768_DCS_write_1A_1P( 0xE2,0xA3);
    TC358768_DCS_write_1A_1P( 0xE3,0x03);
    TC358768_DCS_write_1A_1P( 0xE4,0xC9);
    TC358768_DCS_write_1A_1P( 0xE5,0x03);
    TC358768_DCS_write_1A_1P( 0xE6,0xDD);
    TC358768_DCS_write_1A_1P( 0xE7,0x03);
    TC358768_DCS_write_1A_1P( 0xE8,0xF5);
    TC358768_DCS_write_1A_1P( 0xE9,0x03);
    TC358768_DCS_write_1A_1P( 0xEA,0xFF);

    TC358768_DCS_write_1A_1P( 0xEB,0x30);
    TC358768_DCS_write_1A_1P( 0xEC,0x17);
    TC358768_DCS_write_1A_1P( 0xED,0x20);
    TC358768_DCS_write_1A_1P( 0xEE,0x0F);
    TC358768_DCS_write_1A_1P( 0xEF,0x1F);
    TC358768_DCS_write_1A_1P( 0xF0,0x0F);
    TC358768_DCS_write_1A_1P( 0xF1,0x0F);
    TC358768_DCS_write_1A_1P( 0xF2,0x07);

//CMD2 Page 4
    TC358768_DCS_write_1A_1P( 0xFF, 0x05);
    MDELAY(10);
    TC358768_DCS_write_1A_1P( 0xFB, 0x01);

//CGOUT Mapping
    TC358768_DCS_write_1A_1P( 0x00,0x0F);
    TC358768_DCS_write_1A_1P( 0x01,0x00);
    TC358768_DCS_write_1A_1P( 0x02,0x00);
    TC358768_DCS_write_1A_1P( 0x03,0x00);
    TC358768_DCS_write_1A_1P( 0x04,0x0B);
    TC358768_DCS_write_1A_1P( 0x05,0x0C);
    TC358768_DCS_write_1A_1P( 0x06,0x00);
    TC358768_DCS_write_1A_1P( 0x07,0x00);
    TC358768_DCS_write_1A_1P( 0x08,0x00);
    TC358768_DCS_write_1A_1P( 0x09,0x00);
    TC358768_DCS_write_1A_1P( 0x0A,0x03);
    TC358768_DCS_write_1A_1P( 0x0B,0x04);
    TC358768_DCS_write_1A_1P( 0x0C,0x01);
    TC358768_DCS_write_1A_1P( 0x0D,0x13);
    TC358768_DCS_write_1A_1P( 0x0E,0x15);
    TC358768_DCS_write_1A_1P( 0x0F,0x17);
    TC358768_DCS_write_1A_1P( 0x10,0x0F);
    TC358768_DCS_write_1A_1P( 0x11,0x00);
    TC358768_DCS_write_1A_1P( 0x12,0x00);
    TC358768_DCS_write_1A_1P( 0x13,0x00);
    TC358768_DCS_write_1A_1P( 0x14,0x0B);
    TC358768_DCS_write_1A_1P( 0x15,0x0C);
    TC358768_DCS_write_1A_1P( 0x16,0x00);
    TC358768_DCS_write_1A_1P( 0x17,0x00);
    TC358768_DCS_write_1A_1P( 0x18,0x00);
    TC358768_DCS_write_1A_1P( 0x19,0x00);
    TC358768_DCS_write_1A_1P( 0x1A,0x03);
    TC358768_DCS_write_1A_1P( 0x1B,0x04);
    TC358768_DCS_write_1A_1P( 0x1C,0x01);
    TC358768_DCS_write_1A_1P( 0x1D,0x13);
    TC358768_DCS_write_1A_1P( 0x1E,0x15);
    TC358768_DCS_write_1A_1P( 0x1F,0x17);

//STV
    TC358768_DCS_write_1A_1P( 0x20,0x09);
    TC358768_DCS_write_1A_1P( 0x21,0x01);
    TC358768_DCS_write_1A_1P( 0x22,0x00);
    TC358768_DCS_write_1A_1P( 0x23,0x00);
    TC358768_DCS_write_1A_1P( 0x24,0x00);
    TC358768_DCS_write_1A_1P( 0x25,0xED);

//U2D/D2U
    TC358768_DCS_write_1A_1P( 0x29,0x58);
    TC358768_DCS_write_1A_1P( 0x2A,0x16);
    TC358768_DCS_write_1A_1P( 0x2B,0x05);

//GCK1/2
    TC358768_DCS_write_1A_1P( 0x2F,0x02);
    TC358768_DCS_write_1A_1P( 0x30,0x04);
    TC358768_DCS_write_1A_1P( 0x31,0x49);
    TC358768_DCS_write_1A_1P( 0x32,0x23);
    TC358768_DCS_write_1A_1P( 0x33,0x01);
    TC358768_DCS_write_1A_1P( 0x34,0x00);
    TC358768_DCS_write_1A_1P( 0x35,0x69);
    TC358768_DCS_write_1A_1P( 0x36,0x00);
    TC358768_DCS_write_1A_1P( 0x37,0x2D);
    TC358768_DCS_write_1A_1P( 0x38,0x18);

//APO
    TC358768_DCS_write_1A_1P( 0x5B,0x00);
    TC358768_DCS_write_1A_1P( 0x5F,0x75);
    TC358768_DCS_write_1A_1P( 0x63,0x00);
    TC358768_DCS_write_1A_1P( 0x67,0x04);
    TC358768_DCS_write_1A_1P( 0x6C, 0x00);

//Resolution
    TC358768_DCS_write_1A_1P( 0x90,0x00);

//MUX
    TC358768_DCS_write_1A_1P( 0x74,0x10);
    TC358768_DCS_write_1A_1P( 0x75,0x19);
    TC358768_DCS_write_1A_1P( 0x76,0x06);
    TC358768_DCS_write_1A_1P( 0x77,0x03);
    TC358768_DCS_write_1A_1P( 0x78,0x00);
    TC358768_DCS_write_1A_1P( 0x79,0x00);
    TC358768_DCS_write_1A_1P( 0x7B,0x80);
    TC358768_DCS_write_1A_1P( 0x7C,0xD8);
    TC358768_DCS_write_1A_1P( 0x7D,0x60);
    TC358768_DCS_write_1A_1P( 0x7E,0x10);
    TC358768_DCS_write_1A_1P( 0x7F,0x19);
    TC358768_DCS_write_1A_1P( 0x80,0x00);
    TC358768_DCS_write_1A_1P( 0x81,0x06);
    TC358768_DCS_write_1A_1P( 0x82,0x03);
    TC358768_DCS_write_1A_1P( 0x83,0x00);
    TC358768_DCS_write_1A_1P( 0x84,0x03);
    TC358768_DCS_write_1A_1P( 0x85,0x07);

//MUX and LTF setting
    TC358768_DCS_write_1A_1P( 0x86,0x1B);
    TC358768_DCS_write_1A_1P( 0x87,0x39);
    TC358768_DCS_write_1A_1P( 0x88,0x1B);
    TC358768_DCS_write_1A_1P( 0x89,0x39);
    TC358768_DCS_write_1A_1P( 0x8A,0x33);
//TC358768_DCS_write_1A_1P( 0xB5,0x20);
    TC358768_DCS_write_1A_1P( 0x8C,0x01);

//RTN, BP, PF  (Normal/Idle Mode)
    TC358768_DCS_write_1A_1P( 0x91,0x4C);
    TC358768_DCS_write_1A_1P( 0x92,0x79);
    TC358768_DCS_write_1A_1P( 0x93,0x04);
    TC358768_DCS_write_1A_1P( 0x94,0x04);
    TC358768_DCS_write_1A_1P( 0x95,0xE4);

    TC358768_DCS_write_1A_1P( 0x98,0x00);
    TC358768_DCS_write_1A_1P( 0x99,0x33);
    TC358768_DCS_write_1A_1P( 0x9B,0x0F);
    TC358768_DCS_write_1A_1P( 0xA4,0x0F);
    TC358768_DCS_write_1A_1P( 0x9D,0xB0);


//GPIO output
    TC358768_DCS_write_1A_1P( 0xC4,0x24);
    TC358768_DCS_write_1A_1P( 0xC6,0x00);

//PWM Frequency
    TC358768_DCS_write_1A_1P( 0xFF,0x23);
    MDELAY(10);
    TC358768_DCS_write_1A_1P( 0x08,0x04);
    TC358768_DCS_write_1A_1P( 0xFB,0x01);

//FRM
//TC358768_DCS_write_1A_1P( 0xFF,0x05);
//DELAY 1
//TC358768_DCS_write_1A_1P( 0xEC,0x01);

//CMD1
    TC358768_DCS_write_1A_1P( 0xFF,0x00);
    MDELAY(10);
    TC358768_DCS_write_1A_1P( 0xD3,0x09);
    TC358768_DCS_write_1A_1P( 0xD4,0x0A);

    TC358768_DCS_write_1A_0P(0x11);
    MDELAY(120);
    TC358768_DCS_write_1A_0P(0x29);
    MDELAY(20);
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

        params->dsi.vertical_sync_active      = 2;
        params->dsi.vertical_backporch        = 1;
        params->dsi.vertical_frontporch       = 1;
        params->dsi.vertical_active_line      = FRAME_HEIGHT;

		params->dsi.horizontal_sync_active				= 0x12;// 50  2
		params->dsi.horizontal_backporch				= 0x5f;
		params->dsi.horizontal_frontporch				= 0x5f;
        params->dsi.horizontal_active_pixel   = FRAME_WIDTH;

        // Bit rate calculation
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

    MDELAY(5);

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

static void lcm_suspend(void)
{
    unsigned int data_array[16];
    //unsigned char buffer[2];

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
    unsigned int data_array[16];
    unsigned char buffer[2];
    if(!lcm_is_init)
    {
#ifdef BUILD_LK
        printf("%s, LK\n",__func__);
#else
        printk("%s, kernel\n",__func__);
#endif
        lcm_init();
    }
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

    data_array[0]= 0x00290508;//HW bug, so need send one HS packet
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0]= 0x002c3909;
    dsi_set_cmdq(data_array, 1, 0);

}
#endif

static unsigned int lcm_esd_check(void)
{
#ifdef BUILD_LK
    printf("%s, LK\n",__func__);
#else
    //printk("%s, kernel\n",__func__);
#endif
    unsigned char buffer_1[12];
    unsigned int array[16];
    int i;
    unsigned char fResult;

    for(i = 0;i < 11;i++)
    buffer_1[i] = 0x00;

    //---------------------------------
    // Set Maximum Return Size
    //---------------------------------
    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1, 1);

    //---------------------------------
    // Read [9Ch, 00h, ECC] + Error Report(4 Bytes)
    //---------------------------------
    read_reg_v2(0x0A, buffer_1, 7);
#if 0//ndef BUILD_LK
    printk(KERN_EMERG "lcm_esd_check: read(0x0A)==========\n");
    for(i = 0;i < 7;i++)
    printk(KERN_EMERG "buffer_1[%d]:0x%x \n",i,buffer_1[i]);
#endif

    if ( 0x9c == buffer_1[0] && 0 == buffer_1[3])
    return false;
    else
    return true;

}

static unsigned int lcm_esd_recover(void)
{
#ifdef BUILD_LK
    printf("%s, LK\n",__func__);
#else
    printk("%s, kernel\n",__func__);
#endif

    lcm_init();

    return true;
}

#if 1
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
static int GN_IMM_GetOneChannelValue(int dwChannel, int deCount)
{
    int data[4], i;
    unsigned int ret = 0,ret_value1=0,ret_value2=0;

    i = deCount;
    while(i--){
    IMM_GetOneChannelValue(dwChannel, data, 0);
    ret_value1 += data[0];
    ret_value2 += data[1];
    }

    ret = ret_value1*1000/deCount + ret_value2*10/deCount;
    return ret;
}

static unsigned int lcm_compare_id(void)
{
    unsigned int id_vol3 = 0xffffffff;

    dct_pmic_VGP2_enable(1);
    MDELAY(10);
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

    id_vol3 = GN_IMM_GetOneChannelValue(0, 20);
    LCM_DEBUG("trust_nt35596 lcm_compare_id(id_vol)= %d. \n", id_vol3);

  if((id_vol3>700) && (id_vol3<900)) //Trust id pin voltage is about 0.8v
  {
      return 1;
  }
  else
  {
      return 0;
  }

}
#endif

LCM_DRIVER nt35596_fhd_dsi_vdo_trust_lcm_drv =
{
    .name           = "nt35596_fhd_dsi_vdo_trust",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    //.esd_check      = lcm_esd_check,
    //.esd_recover    = lcm_esd_recover,
    .compare_id     = lcm_compare_id,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
};
