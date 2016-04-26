
include $(srctree)/drivers/misc/mediatek/Makefile.custom

# Module project related part

ifeq ($(CONFIG_MTK_RPR410),y)
obj-y	+=  alsps/rpr410/
endif
ifeq ($(CONFIG_MTK_BMA056),y)
obj-y	+=  accelerometer/bma056/
endif
ifeq ($(CONFIG_MTK_BMM056_NEW),y)
obj-y	+=  magnetometer/bmm056-new/
endif

ifneq ($(CONFIG_ARCH_MTK_PROJECT),)
ARCH_MTK_PROJECT := $(shell echo $(CONFIG_ARCH_MTK_PROJECT))
obj-y	+= core/
obj-y	+= dct/dct/
endif

obj-y	+=  camera/camera/
obj-y   +=  flashlight/constant_flashlight/
obj-y	+=  vibrator/
obj-y	+=  leds/
obj-y	+=  keypad/
obj-y	+=  sound/
obj-y	+=  accdet/

obj-y	+=  hdmi/

ifeq ($(CONFIG_MTK_TMD2772_AUTO),y)
obj-y +=  alsps/tmd2772_auto/
endif

ifeq ($(CONFIG_MTK_MPU6880G),y)
obj-y +=  accelerometer/mpu6880g/
endif

ifeq ($(CONFIG_MTK_MPU6880GY),y)
obj-y +=  gyroscope/mpu6880gy/
endif

ifeq ($(CONFIG_MTK_LSM330),y)
obj-y +=  accelerometer/lsm330/
endif

ifeq ($(CONFIG_MTK_L3GD20),y)
obj-y +=  gyroscope/l3gd20/
endif

ifeq ($(CONFIG_MTK_MMC3516X_AUTO),y)
obj-y +=  magnetometer/mmc3516x_auto/
endif

ifeq ($(CONFIG_MTK_ST480_AUTO),y)
obj-y +=  magnetometer/st480_auto/
endif

ifeq ($(CONFIG_MTK_LTR553_AUTO),y)
obj-y +=  alsps/ltr553_auto/
endif

