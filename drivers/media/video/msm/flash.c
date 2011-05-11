/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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
 *
 */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/pwm.h>
#include <linux/pmic8058-pwm.h>
#include <mach/pmic.h>
#include <mach/camera.h>
#ifdef CONFIG_HUAWEI_KERNEL

#include <linux/mfd/pmic8058.h>
#include <linux/gpio.h>

#ifdef CONFIG_HUAWEI_EVALUATE_POWER_CONSUMPTION 
#include <mach/msm_battery.h>
#define CAMERA_FLASH_CUR_DIV 10
#endif



static struct  pm8058_gpio camera_flash = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = 0,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_2,
		.inv_int_pol 	= 1,
	};
#endif


static int msm_camera_flash_pwm(
	struct msm_camera_sensor_flash_pwm *pwm,
	unsigned led_state)
{
	int rc = 0;
	int PWM_PERIOD = NSEC_PER_SEC / pwm->freq;
	
	/*description:pwm camera flash*/
	#ifdef CONFIG_HUAWEI_KERNEL
	static struct pwm_device *flash_pwm = NULL;
    #else 
    static struct pwm_device *flash_pwm;
    #endif
	/*If it is the first time to enter the function*/
  	if (!flash_pwm) {
        #ifdef CONFIG_HUAWEI_KERNEL
		 rc = pm8058_gpio_config( 23, &camera_flash);
  	 	 if (rc)  {
        	pr_err("%s PMIC GPIO 24 write failed\n", __func__);
     	 	return rc;
      	 }
        #endif
  		flash_pwm = pwm_request(pwm->channel, "camera-flash");
  		if (flash_pwm == NULL || IS_ERR(flash_pwm)) {
  			pr_err("%s: FAIL pwm_request(): flash_pwm=%p\n",
  			       __func__, flash_pwm);
  			flash_pwm = NULL;
  			return -ENXIO;
  		}
  	}

	switch (led_state) {
/* Switch the unit from ns to us */
    case MSM_CAMERA_LED_LOW:
		rc = pwm_config(flash_pwm,
			(PWM_PERIOD/pwm->max_load)*pwm->low_load/NSEC_PER_USEC,
			PWM_PERIOD/NSEC_PER_USEC);
		if (rc >= 0)
			rc = pwm_enable(flash_pwm);
		break;

	case MSM_CAMERA_LED_HIGH:
		rc = pwm_config(flash_pwm,
			(PWM_PERIOD/pwm->max_load)*pwm->high_load/NSEC_PER_USEC,
			PWM_PERIOD/NSEC_PER_USEC);
		if (rc >= 0)
			rc = pwm_enable(flash_pwm);
		break;
	case MSM_CAMERA_LED_OFF:
		pwm_disable(flash_pwm);
		break;

	default:
		rc = -EFAULT;
		break;
	}

	return rc;
}

int msm_camera_flash_pmic(
	struct msm_camera_sensor_flash_pmic *pmic,
	unsigned led_state)
{
	int rc = 0;
	switch (led_state) {
	case MSM_CAMERA_LED_OFF:
		rc = pmic_flash_led_set_current(0);
		break;

	case MSM_CAMERA_LED_LOW:
		rc = pmic_flash_led_set_current(pmic->low_current);
		break;

	case MSM_CAMERA_LED_HIGH:
		rc = pmic_flash_led_set_current(pmic->high_current);
		break;

	default:
		rc = -EFAULT;
		break;
	}

	CDBG("flash_set_led_state: return %d\n", rc);

	return rc;
}

int32_t msm_camera_flash_set_led_state(
	struct msm_camera_sensor_flash_data *fdata, unsigned led_state)
{
	int32_t rc;

	CDBG("flash_set_led_state: %d flash_sr_type=%d\n", led_state,
	    fdata->flash_src->flash_sr_type);

	if (fdata->flash_type != MSM_CAMERA_FLASH_LED)
		return -ENODEV;

	switch (fdata->flash_src->flash_sr_type) {
	case MSM_CAMERA_FLASH_SRC_PMIC:
		rc = msm_camera_flash_pmic(&fdata->flash_src->_fsrc.pmic_src,
			led_state);
		break;

	case MSM_CAMERA_FLASH_SRC_PWM:
		rc = msm_camera_flash_pwm(&fdata->flash_src->_fsrc.pwm_src,
			led_state);
		break;

	default:
		rc = -ENODEV;
		break;
	}

#ifdef CONFIG_HUAWEI_EVALUATE_POWER_CONSUMPTION 
    /* start calculate flash consume */
	switch (led_state) {
	case MSM_CAMERA_LED_OFF:
        huawei_rpc_current_consuem_notify(EVENT_CAMERA_FLASH_STATE, 0);
		break;

	case MSM_CAMERA_LED_LOW:
        /* the consume depend on low_current */
        huawei_rpc_current_consuem_notify(EVENT_CAMERA_FLASH_STATE, fdata->flash_src->_fsrc.pmic_src.low_current);
		break;

	case MSM_CAMERA_LED_HIGH:
        /* the consume depend on high_current */
        huawei_rpc_current_consuem_notify(EVENT_CAMERA_FLASH_STATE, fdata->flash_src->_fsrc.pmic_src.high_current);
		break;

	default:
		break;
	}
#endif


	return rc;
}
