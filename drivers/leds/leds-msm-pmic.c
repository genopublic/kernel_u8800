/*
 * leds-msm-pmic.c - MSM PMIC LEDs driver.
 *
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/pwm.h>
#include <linux/pmic8058-pwm.h>
#include <linux/mfd/pmic8058.h>

#include <mach/pmic.h>

#ifdef CONFIG_HUAWEI_LEDS_PMIC
#include <linux/hardware_self_adapt.h>
#include <asm/mach-types.h>
#endif
#define MAX_KEYPAD_BL_LEVEL	16

static void msm_keypad_bl_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
#ifdef CONFIG_HUAWEI_LEDS_PMIC
    int ret = 0;
    if(machine_is_msm7x30_u8800()) 
    {

      ret = pmic_set_led_intensity(LED_KEYPAD, value / LED_FULL);
    }
        
    if(machine_is_msm7x30_u8820())
    {   
      ret = pmic_set_mpp6_led_intensity(value / LED_FULL);
    }
    if (ret)
		dev_err(led_cdev->dev, "can't set keypad backlight\n");
#else
	int ret;

	ret = pmic_set_led_intensity(LED_KEYPAD, value / MAX_KEYPAD_BL_LEVEL);
	if (ret)
		dev_err(led_cdev->dev, "can't set keypad backlight\n");
#endif
}
static struct  pm8058_gpio camera_flash = {
                .direction      = PM_GPIO_DIR_OUT,
                .output_buffer  = PM_GPIO_OUT_BUF_CMOS,
                .output_value   = 0,
                .pull           = PM_GPIO_PULL_NO,
                .vin_sel        = 0,
                .out_strength   = PM_GPIO_STRENGTH_HIGH,
                .function       = PM_GPIO_FUNC_2,
                .inv_int_pol    = 1,
        };

static void msm_flashlight_led_set(struct led_classdev *led_cdev,
        enum led_brightness value)
{
	int rc = 0;
        int PWM_PERIOD = NSEC_PER_SEC / 500;
        static struct pwm_device *flash_pwm = NULL;

	if(value==1) // some apps expect the flashlight to be either on or off
		value=255;

        if (!flash_pwm) {
                rc = pm8058_gpio_config( 23, &camera_flash);
                if (rc)  {
                	pr_err("%s PMIC GPIO 23 write failed\n", __func__);
                	return;
        	}
                flash_pwm = pwm_request(0, "camera-flash");
                if (flash_pwm == NULL) {
                        pr_err("%s: FAIL pwm_request(): flash_pwm=%p\n",
                               __func__, flash_pwm);
                        flash_pwm = NULL;
                        return;
                }
        }
	if(value==0) {
		pwm_disable(flash_pwm);
	} else {
		rc = pwm_config(flash_pwm,(PWM_PERIOD/255)*value/NSEC_PER_USEC,PWM_PERIOD/NSEC_PER_USEC);
	        if (rc >= 0)
        		rc = pwm_enable(flash_pwm);
	}
}



static struct led_classdev msm_kp_bl_led = {
#ifdef CONFIG_HUAWEI_LEDS_PMIC
	.name			= "button-backlight",
#else
	.name			= "keyboard-backlight",
#endif
	.brightness_set		= msm_keypad_bl_led_set,
	.brightness		= LED_OFF,
};

static struct led_classdev msm_flashlight_led = {
	.name			= "flashlight",
	.brightness_set		= msm_flashlight_led_set,
	.brightness		= LED_OFF,
};

static int msm_pmic_led_probe(struct platform_device *pdev)
{
	int rc;

	rc = led_classdev_register(&pdev->dev, &msm_kp_bl_led);
	if (rc) {
		dev_err(&pdev->dev, "unable to register led class driver\n");
		return rc;
	}
	rc = led_classdev_register(&pdev->dev, &msm_flashlight_led);
        if (rc) {
                dev_err(&pdev->dev, "unable to register led class driver\n");
                return rc;
        }
	msm_keypad_bl_led_set(&msm_kp_bl_led, LED_OFF);
	return rc;
}

static int __devexit msm_pmic_led_remove(struct platform_device *pdev)
{
	led_classdev_unregister(&msm_kp_bl_led);

	return 0;
}

#ifdef CONFIG_PM
static int msm_pmic_led_suspend(struct platform_device *dev,
		pm_message_t state)
{
	led_classdev_suspend(&msm_kp_bl_led);

	return 0;
}

static int msm_pmic_led_resume(struct platform_device *dev)
{
	led_classdev_resume(&msm_kp_bl_led);

	return 0;
}
#else
#define msm_pmic_led_suspend NULL
#define msm_pmic_led_resume NULL
#endif

static struct platform_driver msm_pmic_led_driver = {
	.probe		= msm_pmic_led_probe,
	.remove		= __devexit_p(msm_pmic_led_remove),
	.suspend	= msm_pmic_led_suspend,
	.resume		= msm_pmic_led_resume,
	.driver		= {
		.name	= "pmic-leds",
		.owner	= THIS_MODULE,
	},
};

static int __init msm_pmic_led_init(void)
{
	return platform_driver_register(&msm_pmic_led_driver);
}
module_init(msm_pmic_led_init);

static void __exit msm_pmic_led_exit(void)
{
	platform_driver_unregister(&msm_pmic_led_driver);
}
module_exit(msm_pmic_led_exit);

MODULE_DESCRIPTION("MSM PMIC LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:pmic-leds");
