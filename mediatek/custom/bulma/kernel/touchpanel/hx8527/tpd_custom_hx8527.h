#ifndef TOUCHPANEL_H__
#define TOUCHPANEL_H__

//------------------------------------------
// Support Function Enable :
#define Android4_0
#define HX_MTK_DMA                             // Support the MTK DMA Function.           Default is open.
#define MT6592
//#define HX_CLOSE_POWER_IN_SLEEP              // Close power when enter suspend.         Default is close.

//TODO START : Select the function you need!
//------------------------------------------
// Support Function Enable :
#define HX_TP_SYS_DIAG                         // Support Sys : Diag function             ,default is open
#define HX_TP_SYS_REGISTER                     // Support Sys : Register function         ,default is open
#define HX_TP_SYS_DEBUG_LEVEL                  // Support Sys : Debug Level function      ,default is open
#define HX_TP_SYS_FLASH_DUMP                   // Support Sys : Flash dump function       ,default is open
#define HX_TP_SYS_SELF_TEST                    // Support Sys : Self Test Function        ,default is open

//#define TPD_PROXIMITY  //wangli_20140428
//#define HX_EN_SEL_BUTTON		       // Support Self Virtual key		,default is close
#define HX_EN_MUT_BUTTON		       // Support Mutual Virtual Key		,default is close
#define HX_RST_PIN_FUNC                        // Support HW Reset                      ,default is open
//#define HX_LOADIN_CONFIG                     // Support Common FW,load in config
//#define HX_PORTING_DEB_MSG                   // Support Driver Porting Message        ,default is close
#define HX_FW_UPDATE_BY_I_FILE                 // Support Update FW by i file           ,default is close //wangli_20140504
//TODO END

#ifdef HX_RST_PIN_FUNC
//#define HX_ESD_WORKAROUND                // Support ESD Workaround                  ,default is close
#endif	

//#define PT_NUM_LOG 
//#define BUTTON_CHECK
#define Himax_Gesture	//wangli_20140530

//------------------------------------------// Virtual key
#define TPD_BUTTON_HEIGH        (90)//(100)
#define TPD_BUTTON_WIDTH        (150)//(120) //wangli_20140625
//#define TPD_KEY_COUNT  4 //wangli_20140430
#define TPD_KEY_COUNT  3
#define key_1           190,1961               //auto define
#define key_2           540,1961
#define key_3           920,1961	//wangli_20140625
//#define key_4           1640,1961 //wangli_20140430
#define TPD_KEYS        {KEY_APP_SWITCH,KEY_HOMEPAGE,KEY_BACK}
#define TPD_KEYS_DIM    {{key_1,TPD_BUTTON_WIDTH,TPD_BUTTON_HEIGH},{key_2,TPD_BUTTON_WIDTH,TPD_BUTTON_HEIGH},{key_3,TPD_BUTTON_WIDTH,TPD_BUTTON_HEIGH}}


// *************************** kick dog start ************************************
// Android 4.1 wangli_20140505
enum wk_wdt_type {
	WK_WDT_LOC_TYPE,
	WK_WDT_EXT_TYPE,
	WK_WDT_LOC_TYPE_NOLOCK,
	WK_WDT_EXT_TYPE_NOLOCK,
};
extern void mpcore_wdt_restart(enum wk_wdt_type type);
extern void mtk_wdt_restart(enum wk_wdt_type type);
// *************************** kick dog end **************************************

#endif /* TOUCHPANEL_H__ */
