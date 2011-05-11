/* Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*creat by lijuan 00152865 2010/03/30*/


#include "msm_fb.h"
#include "mddihost.h"
#include "mddihosti.h"
#include "mddi_nt35582.h"
#include <linux/mfd/pmic8058.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/hardware_self_adapt.h>
#include "hw_backlight.h"
#define LCD_DEBUG
#ifdef  LCD_DEBUG
#define MDDI_LCD_DEBUG(fmt, args...) printk(KERN_ERR fmt, ##args)
#else
#define MDDI_LCD_DEBUG(fmt, args...)
#endif
static int nt35582_lcd_on(struct platform_device *pdev);
static int nt35582_lcd_off(struct platform_device *pdev);
static int nt35582_lcd_on(struct platform_device *pdev)
{
    int ret;
    /*exit sleep mode*/
    ret = mddi_queue_register_write(0x1100,0,TRUE,0);
    mdelay(50);
    MDDI_LCD_DEBUG("%s: nt35582_lcd exit sleep mode ,on_ret=%d\n",__func__,ret);
       
	return ret;
}
static int nt35582_lcd_off(struct platform_device *pdev)
{
    int ret;
    /*enter sleep mode*/
    ret = mddi_queue_register_write(0x1000,0,TRUE,0);
    mdelay(50);
    MDDI_LCD_DEBUG("%s: nt35582_lcd enter sleep mode ,off_ret=%d\n",__func__,ret);
	return ret;

}

/* move the backlight code to hw_backlight.c as common */

static int __init nt35582_probe(struct platform_device *pdev)
{
	msm_fb_add_device(pdev);
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = nt35582_probe,
	.driver = {
		.name   = "mddi_nt35582_wvga",
	},
};
static struct msm_fb_panel_data nt35582_panel_data = {
	.on = nt35582_lcd_on,
	.off = nt35582_lcd_off,
	.set_backlight = pwm_set_backlight,
};
static struct platform_device this_device = {
	.name   = "mddi_nt35582_wvga",
	.id	= 0,
	.dev	= {
		.platform_data = &nt35582_panel_data,
	}
};
static int __init nt35582_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;
	lcd_panel_type lcd_panel ;
	lcd_panel=lcd_panel_probe();
	if(LCD_NT35582_TRULY_WVGA!=lcd_panel&&LCD_NT35582_BYD_WVGA!=lcd_panel)
		return 0;
	MDDI_LCD_DEBUG("------nt35582_init------\n");
	
	ret = platform_driver_register(&this_driver);
	if (!ret) {
		pinfo = &nt35582_panel_data.panel_info;
		pinfo->xres = 480;
		pinfo->yres = 800;
		pinfo->type = MDDI_PANEL;
		pinfo->pdest = DISPLAY_1;
		pinfo->mddi.vdopkt = MDDI_DEFAULT_PRIM_PIX_ATTR;
		pinfo->wait_cycle = 0;
/* Set MDDI clk 192MHz,set 24bit per pixel,
 * adjust the start of data to sync with vsync signal
 */
/* change 24bit into 16bit */
		pinfo->bpp = 16;
		pinfo->fb_num = 2;
        pinfo->clk_rate = 192000000;
	    pinfo->clk_min = 192000000;
	    pinfo->clk_max = 192000000;
        MDDI_LCD_DEBUG("%s: BYD LCD and Truly LCD,set MDDI_CLK=%d \n",__func__, pinfo->clk_rate);
		pinfo->lcd.vsync_enable = TRUE;
/* Reduce the fps,sync depend on the vsync signal*/
        pinfo->lcd.refx100 = 4000;
		pinfo->lcd.v_back_porch = 0;
		pinfo->lcd.v_front_porch = 0;
		pinfo->lcd.v_pulse_width = 22;
		pinfo->lcd.hw_vsync_mode = TRUE;
		pinfo->lcd.vsync_notifier_period = 0;
		pinfo->bl_max = 255;


		ret = platform_device_register(&this_device);
		if (ret)
			{
			platform_driver_unregister(&this_driver);
			MDDI_LCD_DEBUG("%s: Failed on platform_device_register(): rc=%d \n",__func__, ret);
			}
		}

	return ret;
}
module_init(nt35582_init);
