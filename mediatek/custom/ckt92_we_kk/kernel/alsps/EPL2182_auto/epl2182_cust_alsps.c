#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_alsps.h>

static struct alsps_hw cust_alsps_hw = {
    .i2c_num    = 0,
	.polling_mode_ps =0,
	.polling_mode_als =1,
    .power_id   = MT65XX_POWER_NONE,    /*LDO is not used*/
    .power_vol  = VOL_DEFAULT,          /*LDO is not used*/
// 苏 勇 2014年01月07日 09:53:35    .als_level  = { 0,  1,  1,   7,  15,  15,  100, 1000, 2000,  3000,  6000, 10000, 14000, 18000, 20000},
// 苏 勇 2014年01月07日 09:53:35    .als_value  = {40, 40, 90,  90, 160, 160,  225,  320,  640,  1280,  1280,  2600,  2600, 2600,  10240, 10240},
    .als_level  = { 15,  20,  35,   55,  75,  90,  100, 1000, 2000,  3000,  6000, 10000, 14000, 18000, 20000},
    .als_value  = {20, 40, 90,  100, 160, 180,  225,  320,  640,  1280,  1280,  2600,  2600, 2600,  10240, 10240},
	.ps_threshold_high = 500,
    .ps_threshold_low = 450,
};
struct alsps_hw *EPL2182_get_cust_alsps_hw(void) {
    return &cust_alsps_hw;
}

