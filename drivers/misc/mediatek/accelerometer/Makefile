include $(srctree)/drivers/misc/mediatek/Makefile.custom

# In case the platform does NOT support this type of sensors

obj-y   +=  accel.o accel_factory.o

ifeq ($(CONFIG_MTK_K2DH),y)
obj-y   +=  k2dh/
endif

ifeq ($(CONFIG_MTK_BMA050),y)
obj-y   +=  bma050/
endif
ifeq ($(CONFIG_MTK_BMA2XX),y)
obj-y   +=  bma2xx/
endif
ifeq ($(CONFIG_MTK_BMA050_NEW),y)
obj-y   +=  bma050-new/
endif
ifeq ($(CONFIG_MTK_BMA222E),y)
obj-y   +=  bma222E/
endif
ifeq ($(CONFIG_MTK_BMA222E_NEW),y)
obj-y   +=  bma222E-new/
endif
ifeq ($(CONFIG_MTK_MPU6050G),y)
obj-y   +=  mpu6050g/
endif
ifeq ($(CONFIG_MTK_MPU6050G_NEW),y)
obj-y   +=  mpu6050g-new/
endif
ifeq ($(CONFIG_MTK_BMA250),y)
obj-y   +=  bma250/
endif
ifeq ($(CONFIG_MTK_KXTIK1004),y)
obj-y   +=  kxtik1004/
endif

ifeq ($(CONFIG_MTK_MPU6515A),y)
obj-y   +=  mpu6515/
endif

ifeq ($(CONFIG_MTK_KXTJ2_1009),y)
obj-y   +=  KXTJ2_1009/
endif

ifeq ($(CONFIG_MTK_KXTJ2_1009_NEW),y)
obj-y   +=  KXTJ2_1009-new/
endif

ifeq ($(CONFIG_MTK_BMA250E),y)
obj-y   +=  bma250e/
endif

ifeq ($(CONFIG_MTK_BMA250E_NKT),y)
obj-y   +=  bma250e_nkt/
endif

ifeq ($(CONFIG_MTK_MPU60X0),y)
obj-y   +=  mpu60x0/
endif

ifeq ($(CONFIG_MTK_BMA255_SDO0),y)
obj-y   +=  bma255-sdo0/
endif

ifeq ($(CONFIG_MTK_BMA255_SDO1),y)
obj-y   +=  bma255-sdo1/
endif
ifeq ($(CONFIG_MTK_BMA056),y)
obj-y   +=  bma056/
endif

ifeq ($(CONFIG_MTK_LSM6DS3),y)
obj-y   +=  lsm6ds3/
endif

ifeq ($(CONFIG_MTK_LSM330),y)
obj-y   +=  lsm330/
endif

ifeq ($(CONFIG_MTK_LSM303D),y)
obj-y   +=  lsm303d/
endif

ifeq ($(CONFIG_MTK_GMA303),y)
obj-y   +=  gma303/
endif

ifeq ($(CONFIG_MTK_MPU6880G),y)
obj-y   +=  mpu6880g/
endif

