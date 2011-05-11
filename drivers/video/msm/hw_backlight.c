/* drivers\video\msm\hw_backlight.c
 * backlight driver for 7x30 platform
 *
 * Copyright (C) 2010 HUAWEI Technology Co., ltd.
 * 
 * Date: 2010/12/07
 * 
 */

#include "msm_fb.h"
#include <linux/mfd/pmic8058.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include <mach/pmic.h>
#define PWM_PERIOD ( NSEC_PER_SEC / ( 22 * 1000 ) )	/* ns, period of 22Khz */
#define PWM_LEVEL 255
#define PWM_DUTY_LEVEL (PWM_PERIOD / PWM_LEVEL)
#define PM_GPIO25_PWM_ID  1
#define PM_GPIO26_PWM_ID  2
#define ADD_VALUE			4
#define PWM_LEVEL_ADJUST	226
#define BL_MIN_LEVEL 	    30
static struct pwm_device *bl_pwm;
boolean first_set_bl = TRUE;

int backlight_pwm_gpio_config(void)
{
    int rc;
	struct pm8058_gpio backlight_drv = 
	{
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = 0,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_2,
		.inv_int_pol 	= 1,
	};
	/* U8800 use PM_GPIO25 as backlight's PWM,but U8820 use PM_GPIO26 */
    if(machine_is_msm7x30_u8800())
	{
        rc = pm8058_gpio_config( 24, &backlight_drv);
    }
    else if(machine_is_msm7x30_u8820()) 
    {
    	rc = pm8058_gpio_config( 25, &backlight_drv);
    }
	else
	{
    	rc = -1;
	}
	
    if (rc) 
	{
		pr_err("%s LCD backlight GPIO config failed\n", __func__);
		return rc;
	}
    return 0;
}
#ifndef CONFIG_HUAWEI_LEDS_PMIC
void touchkey_setbacklight(int level)
{
    if(machine_is_msm7x30_u8800()) 
	{
		pmic_set_led_intensity(LED_KEYPAD, level);
	}

    if(machine_is_msm7x30_u8820())
    {   
		/*if the machine is U8820 use the mpp6 for touchkey backlight*/
        pmic_set_mpp6_led_intensity(level);
    }

}
#endif
void pwm_set_backlight (struct msm_fb_data_type * mfd)
{
	int bl_level = mfd->bl_level;
	/*config PM GPIO25 as PWM and request PWM*/
	if(TRUE == first_set_bl)
	{
	    backlight_pwm_gpio_config();
		/* U8800 use PM_GPIO25 as backlight's PWM,but U8820 use PM_GPIO26 */
		if(machine_is_msm7x30_u8800() )
		{
			bl_pwm = pwm_request(PM_GPIO25_PWM_ID, "backlight");
		}
		else if(machine_is_msm7x30_u8820())
		{
			bl_pwm = pwm_request(PM_GPIO26_PWM_ID, "backlight");
		}
		else
		{
			bl_pwm = NULL;
		}
		
	    if (NULL == bl_pwm || IS_ERR(bl_pwm)) 
		{
	    	pr_err("%s: pwm_request() failed\n", __func__);
	    	bl_pwm = NULL;
	    }
	    first_set_bl = FALSE;

	}
	if (bl_pwm) 
	{
		/* keep duty 10% < level < 90% */
		if (bl_level)
		{
			bl_level = ((bl_level * PWM_LEVEL_ADJUST) / PWM_LEVEL + ADD_VALUE); 
			if (bl_level < BL_MIN_LEVEL)
			{
				bl_level = BL_MIN_LEVEL;
			}
		}
		pwm_config(bl_pwm, PWM_DUTY_LEVEL*bl_level/NSEC_PER_USEC, PWM_PERIOD/NSEC_PER_USEC);
		pwm_enable(bl_pwm);
	}
#ifndef CONFIG_HUAWEI_LEDS_PMIC
	touchkey_setbacklight(!!bl_level);
#endif

}
