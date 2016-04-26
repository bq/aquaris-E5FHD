include $(srctree)/drivers/misc/mediatek/Makefile.custom

# Linux driver folder
ccflags-y += -I$(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/hx8527/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/hx8527/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/

obj-y	+=  hx8527_driver.o

