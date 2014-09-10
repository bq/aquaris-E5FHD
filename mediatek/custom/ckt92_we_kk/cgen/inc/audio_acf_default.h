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
 * audio_acf_default.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 * This file is the header of audio customization related parameters or definition.
 *
 * Author:
 * -------
 * Tina Tsai
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
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#ifndef AUDIO_ACF_DEFAULT_H
#define AUDIO_ACF_DEFAULT_H

/* Compensation Filter HSF coeffs: default all pass filter       */
/* BesLoudness also uses this coeffs    */
#define BES_LOUDNESS_HSF_COEFF \
0x7893c56,   0xf0ed8754,   0x7893c56,   0x7872c74b,   0x0,     \
0x77f32f5,   0xf1019a16,   0x77f32f5,   0x77ccc7e5,   0x0,     \
0x7516d25,   0xf15d25b6,   0x7516d25,   0x74cecaa0,   0x0,     \
0x71bcf5c,   0xf1c86148,   0x71bcf5c,   0x7140cdc6,   0x0,     \
0x7095837,   0xf1ed4f91,   0x7095837,   0x7003ced8,   0x0,     \
0x6b6a8b9,   0xf292ae8d,   0x6b6a8b9,   0x6a61d38c,   0x0,     \
0x658e294,   0xf34e3ad7,   0x658e294,   0x63d0d8b4,   0x0,     \
0x639551b,   0xf38d55c9,   0x639551b,   0x618fda65,   0x0,     \
0x5b0840c,   0xf49ef7e8,   0x5b0840c,   0x5785e175,   0x0,     \
    \
0x7cbdafc,   0xf0684a08,   0x7cbdafc,   0x7c9bc320,   0x0,     \
0x7c72b1e,   0xf071a9c4,   0x7c72b1e,   0x7c4ac364,   0x0,     \
0x7b13668,   0xf09d9330,   0x7b13668,   0x7ac7c4a0,   0x0,     \
0x7963f61,   0xf0d3813e,   0x7963f61,   0x78dec616,   0x0,     \
0x78ca3d2,   0xf0e6b85c,   0x78ca3d2,   0x782dc698,   0x0,     \
0x75f85af,   0xf140f4a1,   0x75f85af,   0x74d5c8e4,   0x0,     \
0x72802d3,   0xf1affa59,   0x72802d3,   0x7089cb89,   0x0,     \
0x7143ee8,   0xf1d78230,   0x7143ee8,   0x6ef7cc6f,   0x0,     \
0x6b7dc5a,   0xf290474c,   0x6b7dc5a,   0x6758d05d,   0x0

#define BES_LOUDNESS_BPF_COEFF \
    0x3f7e83f3,   0x3d3c7c0c,   0xc3440000,     \
    0x3f73845b,   0x3d007ba4,   0xc38c0000,     \
    0x3f40865c,   0x3be879a3,   0xc4d60000,     \
    0x3f04890b,   0x3a9c76f4,   0xc65f0000,     \
    0x3eef8a0e,   0x3a2875f1,   0xc6e80000,     \
    0x3e8f8f2c,   0x381a70d3,   0xc9560000,     \
    \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0,     \
    0x0,   0x0,   0x0

#define BES_LOUDNESS_LPF_COEFF \
    0xe171c2f,   0xe1713af,   0xf3f20000,     \
    0x1016202d,   0x10160af5,   0xf4af0000,     \
    0x1ac33586,   0x1ac3e25f,   0xf2940000,     \
    0x2c1d583b,   0x2c1dae1a,   0xe16f0000,     \
    0x340c6818,   0x340c9a28,   0xd5a70000,     \
    0x0,   0x0,   0x0
#define BES_LOUDNESS_WS_GAIN_MAX  0x0
#define BES_LOUDNESS_WS_GAIN_MIN  0x0
#define BES_LOUDNESS_FILTER_FIRST  0x0
#define BES_LOUDNESS_ATT_TIME  0x10e50054
#define BES_LOUDNESS_REL_TIME  0x263
#define BES_LOUDNESS_GAIN_MAP_IN \
0xd5, 0xda, 0xe8, 0xfb, 0x6
#define BES_LOUDNESS_GAIN_MAP_OUT \
0x6, 0xe, 0xe, 0x1, 0xfffffffd


#endif
