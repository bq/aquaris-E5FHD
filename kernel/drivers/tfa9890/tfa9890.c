/*
 * drivers/misc/tfa9887.c
 *
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/sysfs.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <sound/initval.h>
//#include <tfa9880/tfa9880.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
 
/***************** DEBUG ****************/
//#define pr_debug printk
//#define pr_warning printk
 
//#define TEST_DEBUG
 
//#define TFA9887_DEBUG
 
//#define TFA9887_STEREO
 
/***************** DEBUG ****************/
 
#ifdef TFA9887_DEBUG
#define PRINT_LOG printk
#else
#define PRINT_LOG(...) 
#endif
#define TFA9887_REVISIONNUMBER 0x03
#define TFA9887_REV 0x80
#ifdef TFA9887_DEBUG
#define assert(expr) \
if (!(expr)) { \
printk( "Assertion failed! %s,%s,%s,line=%d\n", \
#expr,__FILE__,__func__,__LINE__); \
}
#else
#define assert(expr) do {} while (0)
#endif /* TFA9887_DEBUG */
struct tfa9887_dev {
     struct mutex lock;
     struct i2c_client *client;
     struct miscdevice tfa9887_device;
     bool deviceInit;
};
 
#ifdef TFA9887_STEREO
static struct tfa9887_dev *tfa9887R;
#endif
static struct tfa9887_dev *tfa9887L; //, *tfa9887L_byte;
static u8 tfa9890_isfound=0;
u8 Tfa9890_IsFound(void)
{
    return tfa9890_isfound;
}

#define I2C_BUS_NUM_STATIC_ALLOC
#ifdef TFA9887_STEREO
#define TFA9887R_I2C_NAME   "i2c_tfa9887R"
#define TFA9887R_I2C_ADDR    0x36
#endif
#define TFA9887L_I2C_NAME   "i2c_tfa9887L"
#define TFA9887L_I2C_ADDR    0x34
#define I2C_STATIC_BUS_NUM        (2)
 
#define MAX_BUFFER_SIZE 512
#define _MATV_HIGH_SPEED_DMA_
 
#ifdef _MATV_HIGH_SPEED_DMA_
static u8 *gpDMABuf_va = NULL;
static u32 gpDMABuf_pa = NULL;
#endif
 
#ifdef I2C_BUS_NUM_STATIC_ALLOC
 
#ifdef TFA9887_STEREO
static struct i2c_board_info  tfa9887R_i2c_boardinfo = {
        I2C_BOARD_INFO(TFA9887R_I2C_NAME, TFA9887R_I2C_ADDR), 
};
#endif
 
static struct i2c_board_info  __initdata tfa9887L_i2c_boardinfo = {
        I2C_BOARD_INFO(TFA9887L_I2C_NAME, TFA9887L_I2C_ADDR)
};
#endif
 
#ifdef I2C_BUS_NUM_STATIC_ALLOC
int i2c_static_add_device(struct i2c_board_info *info)
{
struct i2c_adapter *adapter;
struct i2c_client *client;
int err;
 
adapter = i2c_get_adapter(I2C_STATIC_BUS_NUM);
if (!adapter) {
printk("tfa9887 %s: can't get i2c adapter\n", __FUNCTION__);
err = -ENODEV;
goto i2c_err;
}
 
client = i2c_new_device(adapter, info);
if (!client) {
pr_err("tfa9887 %s:  can't add i2c device at 0x%x\n",
__FUNCTION__, (unsigned int)info->addr);
err = -ENODEV;
goto i2c_err;
}
 
i2c_put_adapter(adapter);
 
return 0;
 
i2c_err:
return err;
}
 
#endif /*I2C_BUS_NUM_STATIC_ALLOC*/
 
//////////////////////// i2c R/W ////////////////////////////
#if 0
 
int Tfa9887_WriteRegister(struct tfa9887_dev *tfa9887, unsigned int subaddress, unsigned int value)
{
union i2c_smbus_data data;
data.word = ((value&0x00FF)<< 8)|((value&0xFF00)>> 8);;
return i2c_smbus_xfer(tfa9887->client->adapter, tfa9887->client->addr, tfa9887->client->flags,
      I2C_SMBUS_WRITE, subaddress,
      I2C_SMBUS_WORD_DATA, &data); 
}
 
int Tfa9887_ReadRegister(struct tfa9887_dev *tfa9887, unsigned int subaddress, unsigned int* pValue)
{
union i2c_smbus_data data;
int status;
 
        assert(pValue != (unsigned short *)0);
status = i2c_smbus_xfer(tfa9887->client->adapter, tfa9887->client->addr, tfa9887->client->flags,
I2C_SMBUS_READ, subaddress,
I2C_SMBUS_WORD_DATA, &data);  
  if((data.word > 0)||(data.word == 0))
  {
  *pValue=((data.word&0x00FF)<< 8)|((data.word&0xFF00)>> 8);
  }
         return status;
 
}
static int regmap_raw_read(struct tfa9887_dev *tfa9887, unsigned char  address, unsigned char *bytes, int size)
{
//int ret = 0;
int i = 0;
if (size > MAX_BUFFER_SIZE)
size = MAX_BUFFER_SIZE;
 
//PRINT_LOG("%s : reading %zu bytes.\n", __func__, size);
 
//mutex_lock(&tfa9887->lock);
 
//ret = i2c_smbus_read_i2c_block_data(tfa9887->client, address, size, bytes);
for( i = 0 ; i <size; i ++) {
bytes[i] = i2c_smbus_read_byte_data(tfa9887->client, address);
if(bytes[i] < 0) {
return -EIO;
}
}
//mutex_unlock(&tfa9887->lock);
return 0; //Tfa9887_Error_Ok
#if 0
/* Write sub address */
ret = i2c_master_send(tfa9887->client, &address, 1);
if (ret != 1) 
{
printk("%s : i2c_master_send returned %d\n", __func__, ret);
ret = -EIO;
} 
/* Read data */
ret = i2c_master_recv(tfa9887->client, bytes, size);
#endif
 
#if 0
if (ret < 0) 
{
printk("%s: i2c_master_recv returned %d\n", __func__, ret);
return ret;
}
if (ret > size) 
{
printk("%s: received too many bytes from i2c (%d)\n", __func__, ret);
return -EIO;
}
return ret;
#endif
}
 
static int regmap_raw_write(struct tfa9887_dev *tfa9887, unsigned char subaddress, unsigned char *data, int length)
{
int ret = 0;
int i = 0;
if (length > MAX_BUFFER_SIZE)
{
length = MAX_BUFFER_SIZE;
}
 
//PRINT_LOG("%s : writing %zu bytes.\n", __func__, length);
#if 0
/* Write data */
ret = i2c_master_send(tfa9887->client, buf, count);
#endif
for( i = 0 ; i < length; i ++)
{
ret = i2c_smbus_write_byte_data(tfa9887->client, subaddress, data[i]);
if (ret < 0) 
{
printk("%s : i2c_master_send returned %d\n", __func__, ret);
//ret = -EIO;
return -EIO;
}
}
return 0; //Tfa9887_Error_Ok
#if 0
int error = 0; //, i = 0; 
while(i < length)
{
error = i2c_smbus_write_byte_data(client, subaddress, data[i++]);
if(error < 0)
{
PRINT_LOG("[Tfa9887_I2CWriteData] Write data ERROR! error = %d.\n", error);
return error;
}
}
PRINT_LOG("[Tfa9887_I2CWriteData] Write %d bytes.\n", i);
return error; //error > 0 is OK!
#endif
}
#endif
 
static ssize_t tfa9887L_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *offset)
{
struct tfa9887_dev *tfa9887_dev = filp->private_data;
char tmp[MAX_BUFFER_SIZE];
int ret;
        char *p =NULL;
#ifdef TEST_DEBUG
int i;
#endif
int ii = 0;
 
if (count > MAX_BUFFER_SIZE)
count = MAX_BUFFER_SIZE;
 
PRINT_LOG("%s : reading %zu bytes.\n", __func__, count);
 
mutex_lock(&tfa9887_dev->lock);
#if defined(_MATV_HIGH_SPEED_DMA_)
ret = i2c_master_recv(tfa9887L->client, gpDMABuf_pa, count ); 
#else
/* Read data */
ret = i2c_master_recv(tfa9887L->client, tmp, count);
#endif
/////////////////////////////
/*p =  tmp;
      if(count > 8)
       {
         while(ii <= count)
         {
     ret = i2c_master_recv(tfa9887L->client, p, 8);
         if (ret != 8) 
{
printk("%s : i2c_master_send returned1 %d\n", __func__, ret);
ret = -EIO;
}
             p=p+8;
             ii+=8; 
             if(ii > count)
             {
               ret = i2c_master_recv(tfa9887L->client, p, 8 -(ii - count));
              if (ret != 8 -(ii - count)) 
      {
printk("%s : i2c_master_send returned2 %d\n", __func__, ret);
ret = -EIO;
       }
                break;
             }
         }
       }
       else
{
         ret = i2c_master_recv(tfa9887L->client, tmp, count);
     if (ret != count) 
{
printk("%s : i2c_master_send returned3 %d\n", __func__, ret);
ret = -EIO;
}    
}*/
/////////////////////////
mutex_unlock(&tfa9887_dev->lock);
 
if (ret < 0) 
{
printk("%s: i2c_master_recv returned %d\n", __func__, ret);
return ret;
}
if (ret > count) 
{
printk("%s: received too many bytes from i2c (%d)\n", __func__, ret);
return -EIO;
}
#if defined(_MATV_HIGH_SPEED_DMA_)
if (copy_to_user(buf, gpDMABuf_va, ret)) 
{
pr_warning("%s : failed to copy to user space\n", __func__);
return -EFAULT;
}
#else
if (copy_to_user(buf, tmp, ret)) 
{
pr_warning("%s : failed to copy to user space\n", __func__);
return -EFAULT;
}
#endif
#ifdef TEST_DEBUG
PRINT_LOG("[%s] Read from TFA9887:", __func__);
#if defined(_MATV_HIGH_SPEED_DMA_)
for(i = 0; i < ret; i++)
{
PRINT_LOG(" %02X", gpDMABuf_va[i]);
}
PRINT_LOG("\n");
#else
for(i = 0; i < ret; i++)
{
PRINT_LOG(" %02X", temp[i]);
}
PRINT_LOG("\n");
#endif
#endif
return ret;
}
 
static ssize_t tfa9887L_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
struct tfa9887_dev  *tfa9887_dev;
char tmp[MAX_BUFFER_SIZE];
        char *p =NULL;
int ret;
#ifdef TEST_DEBUG
int i;
#endif
        int ii = 0;
tfa9887_dev = filp->private_data;
        //printk("tfa9887_dev is 0x%x\n",tfa9887_dev);
if (count > MAX_BUFFER_SIZE)
{
count = MAX_BUFFER_SIZE;
}
if (copy_from_user(tmp, buf, count)) 
{
printk("%s : failed to copy from user space\n", __func__);
return -EFAULT;
}
#if defined(_MATV_HIGH_SPEED_DMA_)
if (copy_from_user(gpDMABuf_va, buf, count))
{
printk("%s :gpDMABuf_va  failed to copy from user space\n", __func__);
return -EFAULT;
}
#endif
      //  p =  tmp;
PRINT_LOG("%s : writing %zu bytes.\n", __func__, count);
/* Write data */
#if defined(_MATV_HIGH_SPEED_DMA_)
ret = i2c_master_send(tfa9887L->client, gpDMABuf_pa, count);
#else
ret = i2c_master_send(tfa9887L->client, tmp, count);
#endif
     /*  if(count > 8)
       {
         while(ii <= count)
         {
     ret = i2c_master_send(tfa9887L->client, p, 8);
         if (ret != 8) 
{
printk("%s : i2c_master_send returned1 %d\n", __func__, ret);
ret = -EIO;
}
             p=p+8;
             ii+=8; 
             if(ii > count)
             {
               ret = i2c_master_send(tfa9887L->client, p, 8 -(ii - count));
              if (ret != 8 -(ii - count)) 
      {
printk("%s : i2c_master_send returned2 %d\n", __func__, ret);
ret = -EIO;
       }
                break;
             }
         }
       }
       else
{
         ret = i2c_master_send(tfa9887L->client, tmp, count);
     if (ret != count) 
{
printk("%s : i2c_master_send returned3 %d\n", __func__, ret);
ret = -EIO;
}    
}*/ 
#ifdef TEST_DEBUG
PRINT_LOG("[%s] Write to TFA9887:", __func__);
        p = tmp;
for(i = 0; i < count; i++)
{
PRINT_LOG(" %02X , p = %02X \n", gpDMABuf_va[i] , *p);
                p++;  
}
PRINT_LOG("\n");
#endif 
return ret;
}
 
static int tfa9887L_dev_open(struct inode *inode, struct file *filp)
{
 
struct tfa9887_dev *tfa9887_dev = container_of(filp->private_data, struct tfa9887_dev, tfa9887_device);
filp->private_data = tfa9887_dev;
#ifdef _MATV_HIGH_SPEED_DMA_    
    gpDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &gpDMABuf_pa, GFP_KERNEL);
    if(!gpDMABuf_va){
        pr_debug("[tfa9887L][Error] Allocate DMA I2C Buffer failed!\n");
    }
#endif
 
pr_debug("%s : %d,%d\n", __func__, imajor(inode), iminor(inode));
 
return 0;
}
 
static int i2cdev_check(struct device *dev, void *addrp)
{
struct i2c_client *client = i2c_verify_client(dev);
 
if (!client || client->addr != *(unsigned int *)addrp)
return 0;
 
return dev->driver ? -EBUSY : 0;
}
 
/* This address checking function differs from the one in i2c-core
   in that it considers an address with a registered device, but no
   driver bound to it, as NOT busy. */
static int i2cdev_check_addr(struct i2c_adapter *adapter, unsigned int addr)
{
return device_for_each_child(&adapter->dev, &addr, i2cdev_check);
}
 
static int tfa9887L_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
struct i2c_client *client = filp->private_data;
void __user *argp = (void __user *)arg;
int mode = 0;
 
switch (cmd) 
{
case I2C_SLAVE:
case I2C_SLAVE_FORCE:
if ((arg > 0x3ff) || (((client->flags & I2C_M_TEN) == 0) && arg > 0x7f))
return -EINVAL;
if (cmd == I2C_SLAVE && i2cdev_check_addr(client->adapter, arg))
return -EBUSY;
/* REVISIT: address could become busy later */
client->addr = arg;
 
#if 0
case TFA9887_IOCTL_SET_MODE:
if (copy_from_user(&mode ,argp, sizeof(mode))) 
{
return -EFAULT;
}
printk("Set tfa9887 Mode = %d.\n", mode);
#if 1  //for test
eq_mode = mode;
SetMute(tfa9887_dev, Tfa9887_Mute_Digital);
SetPreset(tfa9887_dev);
SetEq(tfa9887_dev);
SetMute(tfa9887_dev, Tfa9887_Mute_Off);
#endif
break;
#endif
default:
printk("%s bad ioctl %u\n", __func__, cmd);
return -EINVAL;
}
 
return 0;
}
 
#ifdef TFA9887_STEREO
static const struct file_operations tfa9887R_dev_fops = 
{
.owner = THIS_MODULE,
.open = tfa9887L_dev_open,
.unlocked_ioctl = tfa9887L_dev_ioctl,
.llseek = no_llseek,
.read = tfa9887L_dev_read,
.write = tfa9887L_dev_write,
};
#endif
 
static const struct file_operations tfa9887L_dev_fops = 
{
.owner = THIS_MODULE,
.open = tfa9887L_dev_open,
.unlocked_ioctl = tfa9887L_dev_ioctl,
.llseek = no_llseek,
.read = tfa9887L_dev_read,
.write = tfa9887L_dev_write,
};
 
#ifdef TFA9887_STEREO
static int tfa9887R_i2c_probe(struct i2c_client *client,
      const struct i2c_device_id *id)
{
int ret = 0;
//struct tfa9887_dev *tfa9887L;
//Tfa9887_handle_t handle;
int tempvalue1=0;
int rev_value=0;
        printk("%s : Smart Audio tfa9887R probe start \n", __func__);
 
if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
{
printk("%s : need I2C_FUNC_I2C\n", __func__);
return  -ENODEV;
}
 
tfa9887R = kzalloc(sizeof(*tfa9887R), GFP_KERNEL);
if (tfa9887R == NULL) 
{
dev_err(&client->dev,
"failed to allocate memory for module data\n");
ret = -ENOMEM;
goto err_exit;
}
i2c_set_clientdata(client, tfa9887R);
tfa9887R->client   = client;
/* init mutex and queues */
mutex_init(&tfa9887R->lock);
 
      tempvalue1 =i2c_smbus_read_word_data(client, TFA9887_REVISIONNUMBER);
  rev_value=((tempvalue1&0x00FF)<< 8)|((tempvalue1&0xFF00)>> 8);
 
      PRINT_LOG("tfa9887R_i2c_probe:rev_value=0x%x\n",rev_value);
 
         if ((rev_value&0xFFFF) == TFA9887_REV) {
                 printk( "NXP Device detected!\n" \
                                 "TFA9887R registered I2C driver!\n");
         } else{
                 printk( "NXP Device not found, \
                                 i2c error %d \n", (rev_value&0xFFFF));
                 //error = -1;
                 goto i2c_error;
 
         }  
 
 
tfa9887R->tfa9887_device.minor = MISC_DYNAMIC_MINOR;
tfa9887R->tfa9887_device.name = "i2c_tfa9887R";
tfa9887R->tfa9887_device.fops = &tfa9887R_dev_fops;
 
ret = misc_register(&tfa9887R->tfa9887_device);
if (ret) 
{
printk("%s : misc_register failed\n", __FILE__);
goto err_misc_register;
}
if (tfa9887R) {
mutex_lock(&tfa9887R->lock);
tfa9887R->deviceInit = true;
mutex_unlock(&tfa9887R->lock);
}
 
printk("Tfa9887R probe success!\n");
return 0;
 
err_misc_register:
misc_deregister(&tfa9887R->tfa9887_device);
i2c_error:
mutex_destroy(&tfa9887R->lock);
kfree(tfa9887R);
tfa9887R = NULL;
err_exit:
return ret;
}
 
static int tfa9887R_i2c_remove(struct i2c_client *client)
{
struct tfa9887_dev *tfa9887_dev;
 
PRINT_LOG("Enter %s.  %d\n", __FUNCTION__, __LINE__);
tfa9887_dev = i2c_get_clientdata(client);
misc_deregister(&tfa9887_dev->tfa9887_device);
mutex_destroy(&tfa9887_dev->lock);
kfree(tfa9887_dev);
return 0;
}
 
static void tfa9887R_i2c_shutdown(struct i2c_client *i2c)
{
if (tfa9887R) {
mutex_lock(&tfa9887R->lock);
tfa9887R->deviceInit = false;
mutex_unlock(&tfa9887R->lock);
        }
}
 
static const struct of_device_id tfa9887R_of_match[] = {
{ .compatible = "nxp,i2c_tfa9887R", },
{},
};
MODULE_DEVICE_TABLE(of, tfa9887R_of_match);
 
static const struct i2c_device_id tfa9887R_i2c_id[] = {
{ "i2c_tfa9887R", 0 },
{ }
};
MODULE_DEVICE_TABLE(i2c, tfa9887R_i2c_id);
 
static struct i2c_driver tfa9887R_i2c_driver = {
 .driver = {
 .name = "i2c_tfa9887R",
 .owner = THIS_MODULE,
 .of_match_table = tfa9887R_of_match,
 },
 .probe =    tfa9887R_i2c_probe,
 .remove =   tfa9887R_i2c_remove,
 .id_table = tfa9887R_i2c_id,
 .shutdown = tfa9887R_i2c_shutdown,
};
#endif
 
static int tfa9887L_i2c_probe(struct i2c_client *client,
      const struct i2c_device_id *id)
{
int ret = 0;
//struct tfa9887_dev *tfa9887L;
//Tfa9887_handle_t handle;
int tempvalue1=0;
int rev_value=0;
        printk("%s : Smart Audio tfa9887L probe start \n", __func__);
 
if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
{
printk("%s : need I2C_FUNC_I2C\n", __func__);
return  -ENODEV;
}
 
tfa9887L = kzalloc(sizeof(*tfa9887L), GFP_KERNEL);
if (tfa9887L == NULL) 
{
dev_err(&client->dev,
"failed to allocate memory for module data\n");
ret = -ENOMEM;
goto err_exit;
}
i2c_set_clientdata(client, tfa9887L);
tfa9887L->client   = client;

/* init mutex and queues */
mutex_init(&tfa9887L->lock);
 
      tempvalue1 =i2c_smbus_read_word_data(client, TFA9887_REVISIONNUMBER);
  rev_value=((tempvalue1&0x00FF)<< 8)|((tempvalue1&0xFF00)>> 8);
 
      PRINT_LOG("tfa9887_i2c_probe:rev_value=0x%x\n",rev_value);
 
         if ((rev_value&0xFFFF) == TFA9887_REV) {
                 printk( "NXP Device detected!\n" \
                                 "TFA9887 registered I2C driver!\n");
         } else{
                 printk( "NXP Device not found, \
                                 i2c error %d \n", (rev_value&0xFFFF));
                 //error = -1;
                 goto i2c_error;
 
         } 


#if defined(_MATV_HIGH_SPEED_DMA_)
    tfa9887L->client->addr = tfa9887L->client->addr | I2C_DMA_FLAG;
    tfa9887L->client->timing = 400;
#endif 
#if 1 //for test V1.1
tfa9887L->tfa9887_device.minor = MISC_DYNAMIC_MINOR;
tfa9887L->tfa9887_device.name = "i2c_tfa9887L";
tfa9887L->tfa9887_device.fops = &tfa9887L_dev_fops;
 
ret = misc_register(&tfa9887L->tfa9887_device);
if (ret) 
{
printk("%s : misc_register failed\n", __FILE__);
goto err_misc_register;
}
#endif
 
if (tfa9887L) {
mutex_lock(&tfa9887L->lock);
tfa9887L->deviceInit = true;
mutex_unlock(&tfa9887L->lock);
}
tfa9890_isfound = 1;
printk("Tfa9887L probe success!\n");
return 0;
 
err_misc_register:
misc_deregister(&tfa9887L->tfa9887_device);
i2c_error:
mutex_destroy(&tfa9887L->lock);
kfree(tfa9887L);
tfa9887L = NULL;
err_exit:
tfa9890_isfound = 0;
return ret;
}
 
 
static int tfa9887L_i2c_remove(struct i2c_client *client)
{
struct tfa9887_dev *tfa9887_dev;
 
PRINT_LOG("Enter %s.  %d\n", __FUNCTION__, __LINE__);
tfa9887_dev = i2c_get_clientdata(client);
misc_deregister(&tfa9887_dev->tfa9887_device);
mutex_destroy(&tfa9887_dev->lock);
kfree(tfa9887_dev);
#ifdef _MATV_HIGH_SPEED_DMA_    
    if(gpDMABuf_va){
        dma_free_coherent(NULL, 4096, gpDMABuf_va, gpDMABuf_pa);
        gpDMABuf_va = NULL;
        gpDMABuf_pa = NULL;
    }
#endif
return 0;
}
 
static void tfa9887L_i2c_shutdown(struct i2c_client *i2c)
{
#ifdef _MATV_HIGH_SPEED_DMA_    
    if(gpDMABuf_va){
        dma_free_coherent(NULL, 4096, gpDMABuf_va, gpDMABuf_pa);
        gpDMABuf_va = NULL;
        gpDMABuf_pa = NULL;
    }
#endif
 

PRINT_LOG("Enter %s. +  %4d\n", __FUNCTION__, __LINE__);
}
 
#if 1
static const struct of_device_id tfa9887L_of_match[] = {
{ .compatible = "nxp,i2c_tfa9887L", },
{},
};
MODULE_DEVICE_TABLE(of, tfa9887L_of_match);
#endif
 
static const struct i2c_device_id tfa9887L_i2c_id[] = {
{ "i2c_tfa9887L", 0 },
{ }
};
MODULE_DEVICE_TABLE(i2c, tfa9887L_i2c_id);
 
static struct i2c_driver tfa9887L_i2c_driver = {
 .driver = {
 .name = "i2c_tfa9887L",
 .owner = THIS_MODULE,
 .of_match_table = tfa9887L_of_match,
 },
 .probe =    tfa9887L_i2c_probe,
 .remove =   tfa9887L_i2c_remove,
 .id_table = tfa9887L_i2c_id,
 .shutdown = tfa9887L_i2c_shutdown,
};
 
static int __init tfa9887_modinit(void)
{
 int ret = 0;
#ifdef TFA9887_STEREO
#ifdef I2C_BUS_NUM_STATIC_ALLOC
        ret = i2c_static_add_device(&tfa9887R_i2c_boardinfo);
        if (ret < 0) {
            printk("%s: add i2c device tfa9887R error %d\n", __FUNCTION__, ret);
        }
        #endif
 ret = i2c_add_driver(&tfa9887R_i2c_driver);
 if (ret != 0) {
 printk(KERN_ERR "Failed to register tfa9887R I2C driver: %d\n",
    ret);
 }
#endif
 
PRINT_LOG("Loading tfa9887L driver\n");
#ifdef I2C_BUS_NUM_STATIC_ALLOC
i2c_register_board_info(2, &tfa9887L_i2c_boardinfo, 1);
        /*
        ret = i2c_static_add_device(&tfa9887L_i2c_boardinfo);
        if (ret < 0) {
            printk("%s: add i2c device tfa9887L error %d\n", __FUNCTION__, ret);
        }*/
        #endif
 ret = i2c_add_driver(&tfa9887L_i2c_driver);
 if (ret != 0) {
 printk(KERN_ERR "Failed to register tfa9887L I2C driver: %d\n",
    ret);
 }
 return ret;
}
module_init(tfa9887_modinit);
 
static void __exit tfa9887_exit(void)
{
#ifdef TFA9887_STEREO
 i2c_del_driver(&tfa9887R_i2c_driver);
#endif
 i2c_del_driver(&tfa9887L_i2c_driver);
#ifdef _MATV_HIGH_SPEED_DMA_    
    if(gpDMABuf_va){
        dma_free_coherent(NULL, 4096, gpDMABuf_va, gpDMABuf_pa);
        gpDMABuf_va = NULL;
        gpDMABuf_pa = NULL;
    }
#endif
 
}
module_exit(tfa9887_exit);
 
MODULE_AUTHOR("Vinod Subbarayalu <vsubbarayalu@nvidia.com>, Scott Peterson <speterson@nvidia.com>");
MODULE_DESCRIPTION("TFA9887 Audio Codec driver");
MODULE_LICENSE("GPL");
 
 
