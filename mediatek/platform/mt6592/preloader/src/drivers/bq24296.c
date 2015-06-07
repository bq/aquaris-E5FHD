#include "platform.h"
#include "i2c.h"
#include "bq24296.h"

int g_bq24296_log_en=0;

/**********************************************************
  *
  *   [I2C Slave Setting] 
  *
  *********************************************************/
#define bq24296_SLAVE_ADDR_WRITE   0xD6
#define bq24296_SLAVE_ADDR_Read    0xD7

/**********************************************************
  *
  *   [Global Variable] 
  *
  *********************************************************/
#define bq24296_REG_NUM 11
kal_uint8 bq24296_reg[bq24296_REG_NUM] = {0};

// yong.su 2013-10-30 20:14:12#define bq24296_I2C_ID	I2C1
#define bq24296_I2C_ID	I2C0
static struct mt_i2c_t bq24296_i2c;

/**********************************************************
  *
  *   [I2C Function For Read/Write bq24296] 
  *
  *********************************************************/
kal_uint32 bq24296_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    bq24296_i2c.id = bq24296_I2C_ID;
    /* Since i2c will left shift 1 bit, we need to set bq24296 I2C address to >>1 */
    bq24296_i2c.addr = (bq24296_SLAVE_ADDR_WRITE >> 1);
    bq24296_i2c.mode = ST_MODE;
    bq24296_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&bq24296_i2c, write_data, len);
    printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code);

    return ret_code;
}

kal_uint32 bq24296_read_byte (kal_uint8 addr, kal_uint8 *dataBuffer) 
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint16 len;
    *dataBuffer = addr;

    bq24296_i2c.id = bq24296_I2C_ID;
    /* Since i2c will left shift 1 bit, we need to set bq24296 I2C address to >>1 */
    bq24296_i2c.addr = (bq24296_SLAVE_ADDR_WRITE >> 1);
    bq24296_i2c.mode = ST_MODE;
    bq24296_i2c.speed = 100;
    len = 1;

    ret_code = i2c_write_read(&bq24296_i2c, dataBuffer, len, len);
    printf("%s: i2c_read: ret_code: %d\n", __func__, ret_code);

    return ret_code;
}

/**********************************************************
  *
  *   [Read / Write Function] 
  *
  *********************************************************/
kal_uint32 bq24296_read_interface (kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT)
{
    kal_uint8 bq24296_reg = 0;
    int ret = 0;
    
    printf("--------------------------------------------------PL\n");

    ret = bq24296_read_byte(RegNum, &bq24296_reg);
    printf("[bq24296_read_interface] Reg[%x]=0x%x\n", RegNum, bq24296_reg);
    
    bq24296_reg &= (MASK << SHIFT);
    *val = (bq24296_reg >> SHIFT);    
    printf("[bq24296_read_interface] val=0x%x\n", *val);

    return ret;
}

kal_uint32 bq24296_config_interface (kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT)
{
    kal_uint8 bq24296_reg = 0;
    int ret = 0;

    printf("--------------------------------------------------PL\n");

    ret = bq24296_read_byte(RegNum, &bq24296_reg);
    printf("[bq24296_config_interface] Reg[%x]=0x%x\n", RegNum, bq24296_reg);
    
    bq24296_reg &= ~(MASK << SHIFT);
    bq24296_reg |= (val << SHIFT);

    ret = bq24296_write_byte(RegNum, bq24296_reg);
    printf("[bq24296_config_interface] write Reg[%x]=0x%x\n", RegNum, bq24296_reg);

    // Check
    //bq24296_read_byte(RegNum, &bq24296_reg);
    //printf("[bq24296_config_interface] Check Reg[%x]=0x%x\n", RegNum, bq24296_reg);

    return ret;
}

/**********************************************************
  *
  *   [Internal Function] 
  *
  *********************************************************/
//CON0----------------------------------------------------

void bq24296_set_en_hiz(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON0), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON0_EN_HIZ_MASK),
                                    (kal_uint8)(CON0_EN_HIZ_SHIFT)
                                    );
  
}

void bq24296_set_vindpm(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON0), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON0_VINDPM_MASK),
                                    (kal_uint8)(CON0_VINDPM_SHIFT)
                                    );
   		
}

void bq24296_set_iinlim(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON0), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON0_IINLIM_MASK),
                                    (kal_uint8)(CON0_IINLIM_SHIFT)
                                    );
    	
}

//CON1----------------------------------------------------

void bq24296_set_reg_rst(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_REG_RST_MASK),
                                    (kal_uint8)(CON1_REG_RST_SHIFT)
                                    );
   		
}

void bq24296_set_wdt_rst(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_WDT_RST_MASK),
                                    (kal_uint8)(CON1_WDT_RST_SHIFT)
                                    );
   	
}

void bq24296_set_otg_config(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_OTG_CONFIG_MASK),
                                    (kal_uint8)(CON1_OTG_CONFIG_SHIFT)
                                    );
}

void bq24296_set_chg_config(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_CHG_CONFIG_MASK),
                                    (kal_uint8)(CON1_CHG_CONFIG_SHIFT)
                                    );
   	
}

void bq24296_set_sys_min(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_SYS_MIN_MASK),
                                    (kal_uint8)(CON1_SYS_MIN_SHIFT)
                                    );
  	
}

void bq24296_set_boost_lim(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_BOOST_LIM_MASK),
                                    (kal_uint8)(CON1_BOOST_LIM_SHIFT)
                                    );
  	
}

//CON2----------------------------------------------------

void bq24296_set_ichg(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON2), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON2_ICHG_MASK),
                                    (kal_uint8)(CON2_ICHG_SHIFT)
                                    );
  		
}

//CON3----------------------------------------------------

void bq24296_set_iprechg(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON3), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON3_IPRECHG_MASK),
                                    (kal_uint8)(CON3_IPRECHG_SHIFT)
                                    );
   		
}

void bq24296_set_iterm(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON3), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON3_ITERM_MASK),
                                    (kal_uint8)(CON3_ITERM_SHIFT)
                                    );
   	
}

//CON4----------------------------------------------------

void bq24296_set_vreg(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON4), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON4_VREG_MASK),
                                    (kal_uint8)(CON4_VREG_SHIFT)
                                    );
   		
}

void bq24296_set_batlowv(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON4), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON4_BATLOWV_MASK),
                                    (kal_uint8)(CON4_BATLOWV_SHIFT)
                                    );
  		
}

void bq24296_set_vrechg(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON4), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON4_VRECHG_MASK),
                                    (kal_uint8)(CON4_VRECHG_SHIFT)
                                    );
    	
}

//CON5----------------------------------------------------

void bq24296_set_en_term(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON5), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON5_EN_TERM_MASK),
                                    (kal_uint8)(CON5_EN_TERM_SHIFT)
                                    );
  	
}

void bq24296_set_term_stat(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON5), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON5_TERM_STAT_MASK),
                                    (kal_uint8)(CON5_TERM_STAT_SHIFT)
                                    );
	
}

void bq24296_set_watchdog(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON5), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON5_WATCHDOG_MASK),
                                    (kal_uint8)(CON5_WATCHDOG_SHIFT)
                                    );
  		
}

void bq24296_set_en_timer(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON5), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON5_EN_TIMER_MASK),
                                    (kal_uint8)(CON5_EN_TIMER_SHIFT)
                                    );
   	
}

void bq24296_set_chg_timer(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON5), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON5_CHG_TIMER_MASK),
                                    (kal_uint8)(CON5_CHG_TIMER_SHIFT)
                                    );
    	
}

//CON6----------------------------------------------------

void bq24296_set_treg(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON6), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON6_TREG_MASK),
                                    (kal_uint8)(CON6_TREG_SHIFT)
                                    );
	
}

//CON7----------------------------------------------------


void bq24296_set_DPDM_en(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON7), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON7_DPDM_EN_MASK),
                                    (kal_uint8)(CON7_DPDM_EN_SHIFT)
                                    );
  	
}

void bq24296_set_tmr2x_en(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON7), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON7_TMR2X_EN_MASK),
                                    (kal_uint8)(CON7_TMR2X_EN_SHIFT)
                                    );
  	
}

void bq24296_set_batfet_disable(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON7), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON7_BATFET_Disable_MASK),
                                    (kal_uint8)(CON7_BATFET_Disable_SHIFT)
                                    );
   	
}

void bq24296_set_int_mask(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=bq24296_config_interface(   (kal_uint8)(bq24296_CON7), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON7_INT_MASK_MASK),
                                    (kal_uint8)(CON7_INT_MASK_SHIFT)
                                    );
   	
}

//CON8----------------------------------------------------

kal_uint32 bq24296_get_system_status(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=bq24296_read_interface(     (kal_uint8)(bq24296_CON8), 
                                    (&val),
                                    (kal_uint8)(0xFF),
                                    (kal_uint8)(0x0)
                                    );
   
	
    return val;
}

kal_uint32 bq24296_get_vbus_stat(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=bq24296_read_interface(     (kal_uint8)(bq24296_CON8), 
                                    (&val),
                                    (kal_uint8)(CON8_VBUS_STAT_MASK),
                                    (kal_uint8)(CON8_VBUS_STAT_SHIFT)
                                    );
 
	
    return val;
}

kal_uint32 bq24296_get_chrg_stat(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=bq24296_read_interface(     (kal_uint8)(bq24296_CON8), 
                                    (&val),
                                    (kal_uint8)(CON8_CHRG_STAT_MASK),
                                    (kal_uint8)(CON8_CHRG_STAT_SHIFT)
                                    );
   
	
    return val;
}

kal_uint32 bq24296_get_vsys_stat(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=bq24296_read_interface(     (kal_uint8)(bq24296_CON8), 
                                    (&val),
                                    (kal_uint8)(CON8_VSYS_STAT_MASK),
                                    (kal_uint8)(CON8_VSYS_STAT_SHIFT)
                                    );
  
	
    return val;
}

/**********************************************************
  *
  *   [Internal Function] 
  *
  *********************************************************/
unsigned int bq24296_reg_config_interface (unsigned char RegNum, unsigned char val)
{
    int ret = 0;
    
    ret = bq24296_write_byte(RegNum, val);

    if(g_bq24296_log_en>1)        
        printf("%d\n", ret);

    return ret;
}

void bq24296_dump_register(void)
{
    int i=0;

    for (i=0;i<bq24296_REG_NUM;i++)
    {
        bq24296_read_byte(i, &bq24296_reg[i]);
        printf("[0x%x]=0x%x\n", i, bq24296_reg[i]);        
    }
}

void bq24296_read_register(int i)
{
    bq24296_read_byte(i, &bq24296_reg[i]);
    printf("[0x%x]=0x%x\n", i, bq24296_reg[i]); 
}

void bq24296_hw_init(void)
{
//    upmu_set_rg_bc11_bb_ctrl(1);    //BC11_BB_CTRL    
    //upmu_set_rg_bc11_rst(1);        //BC11_RST


    //TODO: pull PSEL low
    //TODO: pull CE low
     printf("[bq24296 preload init --------------------------------------------\n"); 
   

    bq24296_set_en_hiz(0x0);
    bq24296_set_vindpm(0x8); //VIN DPM check 4.68V
    bq24296_set_reg_rst(0x0);
    bq24296_set_wdt_rst(0x1); //kick watchdog	
    bq24296_set_sys_min(0x5); //Minimum system voltage 3.5V	
    bq24296_set_iprechg(0x3); //preCharge Current limit 512mA for bq24296 and 384mA for bq24296M
    bq24296_set_iterm(0x01); //Termination Current limit 256mA
    bq24296_set_BHot(0x2);//boost mode thermal protection 65 degrees
    bq24296_set_vreg(0x35);//Charge Voltage Limit 4.35V  
    bq24296_set_batlowv(0x1); //BATLOWV 3.0V
    bq24296_set_vrechg(0x0); //VRECHG 0.1V 
    bq24296_set_en_term(0x1); //Enable termination
    bq24296_set_watchdog(0x1); //WDT 40s
    bq24296_set_en_timer(0x1); //Enable charge timer
    bq24296_set_chg_timer(0x2);//Fast Chare Timer Setting 12hrs
    bq24296_set_int_mask(0x0); //Disable fault interrupt
   
}

