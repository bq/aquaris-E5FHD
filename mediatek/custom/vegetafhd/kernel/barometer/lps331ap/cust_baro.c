#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_baro.h>

/*---------------------------------------------------------------------------*/
static struct baro_hw cust_baro_hw = {
    .i2c_num = 0,
    .direction = 5,
    .power_id = -1,//MT65XX_POWER_NONE,  /*!< LDO is not used */
    .power_vol= 0,//VOL_DEFAULT,        /*!< LDO is not used */
    .firlen = 16,                   /*!< don't enable low pass fileter */
//    .power = cust_gyro_power,
};
/*---------------------------------------------------------------------------*/
struct baro_hw* get_cust_baro_hw(void)
{
    return &cust_baro_hw;
}