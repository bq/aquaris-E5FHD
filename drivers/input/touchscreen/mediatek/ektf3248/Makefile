include $(srctree)/drivers/misc/mediatek/Makefile.custom

# Linux driver folder
ccflags-y += -I$(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/EKTF3248/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/EKTF3248/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/

obj-y	+=  ektf3248_tpd_driver.o

