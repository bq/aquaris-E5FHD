CKT_AUTO_ADD_GLOBAL_DEFINE_BY_NAME = CKT_INTERPOLATION CKT_SUPPORT_AUTOTEST_MODE CKT_LOW_POWER_SUPPORT CKT_MUTE_CLOSE_PA RESPIRATION_LAMP
CKT_AUTO_ADD_GLOBAL_DEFINE_BY_NAME_VALUE =PROJ_NAME CUST_NAME SOFTCODE USB_MANUFACTURER_STRING USB_PRODUCT_STRING USB_STRING_SERIAL_IDX CUSTOM_EXIF_STRING_MAKE CUSTOM_EXIF_STRING_MODEL CUSTOM_EXIF_STRING_SOFTWARE CUSTOM_BTMTK_ANDROID_DEFAULT_REMOTE_NAME CUSTOM_BTMTK_ANDROID_DEFAULT_LOCAL_NAME
CKT_AUTO_ADD_GLOBAL_DEFINE_BY_VALUE = 
#############################
#############################
#############################

#项目的相关定义
PROJ_NAME = VEGETA01A
CUST_NAME = CKT
SOFTCODE = S0A
BASEVERNO =100
#############################
#会用他设置ro.product.model
CKT_PRODUCT_MODEL=CKT_$(strip $(PROJ_NAME) )
#会用他设置缺省时区persist.sys.timezone
TIMEZONE=Asia/Shanghai


############usb相关#################
USB_MANUFACTURER_STRING=$(strip $(CUST_NAME) )
USB_PRODUCT_STRING=$(strip $(CKT_PRODUCT_MODEL) )
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
#CKT_BUILD_VERNO = PANDORA-S0A_CKT_L2EN_111_111111
#CKT_BUILD_INTERNAL_VERNO =PANDORA-S0A_CKT_L2EN_111_111111111111

#############################
#摄像头软件插值
CKT_INTERPOLATION = no
#工厂模式自动测试开关
CKT_SUPPORT_AUTOTEST_MODE = no
#MTK的low power 测试
CKT_LOW_POWER_SUPPORT = no
#ckt liutao 20140409 for close pa when volume is 0
CKT_MUTE_CLOSE_PA = no


CKT_APP_FLASHLIGHT = yes

#打开自动版本号切换功能
CKT_VERSION_AUTO_SWITCH=no
export CKT_VERSION_AUTO_SWITCH


#RGB LED feature control
RESPIRATION_LAMP = yes









































###########以下为产生的东西,一般不需要理会
_CKT_BUILD_VERNO  = $(strip $(PROJ_NAME) )-$(strip $(SOFTCODE) )_$(strip $(CUST_NAME) )_L$(words $(subst hdpi, ,$(strip $(MTK_PRODUCT_LOCALES))))$(word 1,$(subst _, ,$(subst zh_TW,TR,$(subst zh_CN,SM,$(strip $(MTK_PRODUCT_LOCALES))))))_$(strip $(BASEVERNO))

DATA_FOR_VERO=$(shell date +%y%m%d)
DATA_FOR_INTERNAL_VERO=$(shell date +%y%m%d%H%M%S)

CKT_BUILD_VERNO  ?= $(call uc, $(_CKT_BUILD_VERNO)_$(strip $(DATA_FOR_VERO)))
CKT_BUILD_INTERNAL_VERNO  ?= $(call uc, $(_CKT_BUILD_VERNO)_$(strip $(DATA_FOR_INTERNAL_VERO)))
#############################

