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

/*******************************************************************************
 *
 * Filename:
 * ---------
 * aud_hd_record_custom.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 * This file is the header of audio customization related function or definition.
 *
 * Author:
 * -------
 * Charlie Lu.
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#ifndef AUDIO_HD_RECORD_CUSTOM_H
#define AUDIO_HD_RECORD_CUSTOM_H

//hd_rec_fir
/* 0: Input FIR coefficients for Normal mode */
/* 1: Input FIR coefficients for Headset mode */
/* 2: Input FIR coefficients for Handfree mode */
/* 3: Input FIR coefficients for BT mode */
/* 4: Input FIR coefficients for Normal mode */
/* 5: Input FIR coefficients for Handfree mode */
/* 6: Input FIR coefficients for Normal mode 2nd Mic*/
/* 7: Input FIR coefficients for Voice Recognition*/
/* 8: Input FIR coefficients for Voice Unlock Main Mic */
/* 9: Input FIR coefficients for Voice Unlock 2nd Mic */
/* 10: Input FIR coefficients for 1st customization Main Mic*/
/* 11: Input FIR coefficients for 1st customization 2nd Mic*/
/* 12: Input FIR coefficients for 2nd customization Main Mic*/
/* 13: Input FIR coefficients for 2nd customization 2nd Mic*/
/* 14: Input FIR coefficients for 3rd customization Main Mic*/
/* 15: Input FIR coefficients for 3rd customization Ref Mic*/

#define DEFAULT_HD_RECORD_FIR_Coeff \
   -224,   -354,   -358,   -384,   -229,     80,    -15,     42,     58,    -93, \
   -402,     -2,   -271,   -131,    121,     92,   -262,     30,   -509,   -623, \
   -472,   -558,   -347,    -15,     39,    170,    407,   -238,    313,   -744, \
   -605,   -984,   -472,  -1671,    716,  -1266,    718,   -422,   1466,  -2082, \
   2964,  -3664,   3374,  -7274,  20675,  20675,  -7274,   3374,  -3664,   2964, \
  -2082,   1466,   -422,    718,  -1266,    716,  -1671,   -472,   -984,   -605, \
   -744,    313,   -238,    407,    170,     39,    -15,   -347,   -558,   -472, \
   -623,   -509,     30,   -262,     92,    121,   -131,   -271,     -2,   -402, \
    -93,     58,     42,    -15,     80,   -229,   -384,   -358,   -354,   -224, \
\
  32767,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
\
  32767,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
\
  32767,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
\
  32767,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
\
  32767,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
\
    -53,    -74,   -102,    -12,    -11,      4,      3,      4,    -75,    -85, \
   -124,    -16,   -234,     28,   -184,   -250,   -165,   -283,   -235,   -158, \
   -575,   -487,   -511,   -549,   -254,   -111,   -282,   -685,      3,   -151, \
   -272,   -480,    585,   -296,    696,   -552,    563,   -158,   2645,  -1611, \
   -427,   -915,   4985,  -4762,  13126,  13126,  -4762,   4985,   -915,   -427, \
  -1611,   2645,   -158,    563,   -552,    696,   -296,    585,   -480,   -272, \
   -151,      3,   -685,   -282,   -111,   -254,   -549,   -511,   -487,   -575, \
   -158,   -235,   -283,   -165,   -250,   -184,     28,   -234,    -16,   -124, \
    -85,    -75,      4,      3,      4,    -11,    -12,   -102,    -74,    -53, \
\
    486,   -236,   -231,   -214,   -146,   -182,     11,     44,     92,    -45, \
   -474,     49,   -123,    413,    182,    713,     90,    440,   -915,   -267, \
   -432,    556,   -201,   -293,    126,   -546,   -109,   -401,    947,   -526, \
    964,  -1829,    992,  -1942,   1141,  -2652,   1108,  -2736,   3534,  -3878, \
    814,  -5622,   7550,  -8747,  26027,  26027,  -8747,   7550,  -5622,    814, \
  -3878,   3534,  -2736,   1108,  -2652,   1141,  -1942,    992,  -1829,    964, \
   -526,    947,   -401,   -109,   -546,    126,   -293,   -201,    556,   -432, \
   -267,   -915,    440,     90,    713,    182,    413,   -123,     49,   -474, \
    -45,     92,     44,     11,   -182,   -146,   -214,   -231,   -236,    486, \
\
  32767,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
\
  32767,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
\
  32767,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
\
  32767,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
\
  32767,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
\
  32767,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
\
  32767,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
\
  32767,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0, \
      0,      0,      0,      0,      0,      0,      0,      0,      0,      0

#define DEFAULT_HD_RECORD_MODE_FIR_MAPPING_CH1 \
 7, 4, 0, \
 1, 0, 1, \
 0, 1, 0, \
 1, 0, 1, \
 8, 1,10, \
 1,12, 1, \
14, 1, 0, \
 1, 0, 1, \
 0, 1, 0, \
 1, 0, 3

#define DEFAULT_HD_RECORD_MODE_FIR_MAPPING_CH2 \
 6, 6, 6, \
 6, 6, 6, \
 6, 6, 6, \
 6, 6, 6, \
 9, 6,11, \
 6,13, 6, \
15, 6, 6, \
 6, 6, 6, \
 6, 6, 6, \
 6, 6, 3

#endif