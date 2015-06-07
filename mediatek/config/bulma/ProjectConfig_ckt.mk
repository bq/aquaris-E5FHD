CKT_AUTO_ADD_GLOBAL_DEFINE_BY_NAME = CKT_INTERPOLATION CKT_SUPPORT_AUTOTEST_MODE CKT_LOW_POWER_SUPPORT CKT_MUTE_CLOSE_PA USE_FOR_BULMA_HE TEMPERATURE_CONTROL_CHARGING BULMA_PROJECT CKT_USER_INEMOENGINE  RESPIRATION_LAMP USING_TFA9890_EXTAMP
CKT_AUTO_ADD_GLOBAL_DEFINE_BY_NAME_VALUE =PROJ_NAME CUST_NAME SOFTCODE USB_MANUFACTURER_STRING USB_PRODUCT_STRING USB_STRING_SERIAL_IDX CUSTOM_EXIF_STRING_MAKE CUSTOM_EXIF_STRING_MODEL CUSTOM_EXIF_STRING_SOFTWARE CUSTOM_BTMTK_ANDROID_DEFAULT_REMOTE_NAME CUSTOM_BTMTK_ANDROID_DEFAULT_LOCAL_NAME
CKT_AUTO_ADD_GLOBAL_DEFINE_BY_VALUE = 
#############################
#############################
#############################

#项目的相关定义
PROJ_NAME = BULMA01A
CUST_NAME = BQ
SOFTCODE = S0A
BASEVERNO =210
#############################
#会用他设置ro.product.model
CKT_PRODUCT_MODEL=CKT_$(strip $(PROJ_NAME) )
#会用他设置缺省时区persist.sys.timezone
TIMEZONE=Europe/Amsterdam


############usb相关#################
USB_MANUFACTURER_STRING=$(strip $(CUST_NAME) )
USB_PRODUCT_STRING=0123456789ABCDEF
USB_STRING_SERIAL_IDX=$(strip $(USB_PRODUCT_STRING) )

############exif相关#################
CUSTOM_EXIF_STRING_MAKE=$(strip $(CUST_NAME) )
CUSTOM_EXIF_STRING_MODEL=$(strip $(PROJ_NAME) )
CUSTOM_EXIF_STRING_SOFTWARE=""

############bt相关#################
CUSTOM_BTMTK_ANDROID_DEFAULT_LOCAL_NAME =$(strip $(PROJ_NAME) )_BT
CUSTOM_BTMTK_ANDROID_DEFAULT_REMOTE_NAME=$(strip $(PROJ_NAME) )_DEVICE

#############################
#功能的开关,会导入到mediatek/source/frameworks/featureoption/java/com/mediatek/featureoption/FeatureOption.java
#修改的时候注意,在 mediatek/build/tools/javaoption.pm中添加相关模块
#另外注意如果enable只可以用yes,不可以用其他
TESTA = yes
TESTB = no
TESTC = testc_none

#############################

#如果要固定版本号,请在这设置,否则注释调它,而不是留空!!!
CKT_BUILD_VERNO = BULMA01A-S10A_BQ_L41ES_210_140901
CKT_BUILD_INTERNAL_VERNO =BULMA01A-S10A_BQ_L41ES_210_140901104550

#############################
#摄像头软件插值
CKT_INTERPOLATION = yes
#工厂模式自动测试开关
CKT_SUPPORT_AUTOTEST_MODE = no
#MTK的low power 测试
CKT_LOW_POWER_SUPPORT = no

#LED三色灯亮度渐变,
RESPIRATION_LAMP = yes

#ckt helin 20131210 add FlashLight apk to System
CKT_APP_FLASHLIGHT = no
export CKT_APP_FLASHLIGHT
#ckt liutao 20140409 for close pa when volume is 0
CKT_MUTE_CLOSE_PA = no
USE_FOR_BULMA_HE = yes

#add by he for amp
USING_TFA9890_EXTAMP=yes

#added for bulma project by kolery 20140722
BULMA_PROJECT = yes

# 4.35V电池——
# 1. 0℃以下，不允许充电；
# 2. 0℃-10℃，充电电流0.1C且≤400mA，充电限制电压4.2V；
# 3. 10℃-45℃，充电电流＜0.5C，充电限制电压4.35V；
# 4. 45℃以上，不允许充电。
#  
# 4.2V电池——（引用华为项目的要求）
# 1. -10℃~0℃，充电电流＜200mA，截止电压4.0V
# 2. 0℃~50℃，正常充电（doro项目客户要求45℃停止充电）
# 3. 50℃以上停止充电（doro项目客户要求45℃停止充电）
#  
# 整机加在电池两端的充电电压不能超过规格电压，即4.2V电池充电电压不能高于4.2V，4.35V电池充电电压不能高于4.35V。
#  
# 55℃以上高温告警提示
# -20℃以下低温告警提示
#  
# 提示语中不要出现“电池过热”，过热提示用“您的手机过热”；——此项针对所有机型，包括内置电池、外置电池的 
# 不能出现要求用户“移除电池”的提示；——此项针对内置电池的机型
TEMPERATURE_CONTROL_CHARGING = yes

#打开自动版本号切换功能
CKT_VERSION_AUTO_SWITCH=no
export CKT_VERSION_AUTO_SWITCH
#使用st的fussiion引擎
CKT_USER_INEMOENGINE=yes
export CKT_USER_INEMOENGINE











































###########以下为产生的东西,一般不需要理会
_CKT_BUILD_VERNO  = $(strip $(PROJ_NAME) )-$(strip $(SOFTCODE) )_$(strip $(CUST_NAME) )_L$(words $(subst hdpi, ,$(strip $(MTK_PRODUCT_LOCALES))))$(word 1,$(subst _, ,$(subst zh_TW,TR,$(subst zh_CN,SM,$(strip $(MTK_PRODUCT_LOCALES))))))_$(strip $(BASEVERNO))

DATA_FOR_VERO=$(shell date +%y%m%d)
DATA_FOR_INTERNAL_VERO=$(shell date +%y%m%d%H%M%S)

CKT_BUILD_VERNO  ?= $(call uc, $(_CKT_BUILD_VERNO)_$(strip $(DATA_FOR_VERO)))
CKT_BUILD_INTERNAL_VERNO  ?= $(call uc, $(_CKT_BUILD_VERNO)_$(strip $(DATA_FOR_INTERNAL_VERO)))
#############################

