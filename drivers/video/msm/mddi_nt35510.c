/* drivers\video\msm\mddi_nt35510.c
 * NT35510 LCD driver for 7x30 platform
 *
 * Copyright (C) 2010 HUAWEI Technology Co., ltd.
 * 
 * Date: 2010/12/07
 * 
 */

#include "msm_fb.h"
#include "mddihost.h"
#include "mddihosti.h"
#include <linux/mfd/pmic8058.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/hardware_self_adapt.h>
#include <linux/pwm.h>
#include <mach/pmic.h>
#include "hw_backlight.h"
#include "hw_mddi_lcd.h"
#define LCD_DEBUG
#ifdef  LCD_DEBUG
#define MDDI_LCD_DEBUG(fmt, args...) printk(KERN_ERR fmt, ##args)
#else
#define MDDI_LCD_DEBUG(fmt, args...)
#endif
static int nt35510_lcd_on(struct platform_device *pdev);
static int nt35510_lcd_off(struct platform_device *pdev);
static int nt35510_lcd_on(struct platform_device *pdev)
{
    int ret = 0;
    /*exit sleep mode*/
    ret = mddi_queue_register_write(0x1100,0,TRUE,0);
    mdelay(50);
    MDDI_LCD_DEBUG("%s: nt35510_lcd exit sleep mode ,on_ret=%d\n",__func__,ret);
       
	return ret;
}
static int nt35510_lcd_off(struct platform_device *pdev)
{
    int ret = 0;
    /*enter sleep mode*/
    ret = mddi_queue_register_write(0x1000,0,TRUE,0);
    mdelay(50);
    MDDI_LCD_DEBUG("%s: nt35510_lcd enter sleep mode ,off_ret=%d\n",__func__,ret);
	return ret;
}

static int __init nt35510_probe(struct platform_device *pdev)
{
	msm_fb_add_device(pdev);
 	return 0;
}

static struct platform_driver this_driver = {
	.probe  = nt35510_probe,
	.driver = {
		.name   = "mddi_nt35510_wvga",
	},
};

static struct msm_fb_panel_data nt35510_panel_data = {
	.on = nt35510_lcd_on,
	.off = nt35510_lcd_off,
	.set_backlight = pwm_set_backlight,
};

static struct platform_device this_device = {
	.name   = "mddi_nt35510_wvga",
	.id	= 0,
	.dev	= {
		.platform_data = &nt35510_panel_data,
	}
};
static int __init nt35510_init(void)
{
	int ret = 0;
	struct msm_panel_info *pinfo = NULL;
	lcd_panel_type lcd_panel = LCD_NONE;
	bpp_type bpp = MDDI_OUT_16BPP;		
	mddi_type mddi_port_type = mddi_port_type_probe();

	lcd_panel=lcd_panel_probe();
	
	if(LCD_NT35510_ALPHA_SI_WVGA != lcd_panel)
	{
		return 0;
	}
	MDDI_LCD_DEBUG("%s:------nt35510_init------\n",__func__);
	/* Select which bpp accroding MDDI port type */
	if(MDDI_TYPE1 == mddi_port_type)
	{
		bpp = MDDI_OUT_16BPP;
	}
	else if(MDDI_TYPE2 == mddi_port_type)
	{
		bpp = MDDI_OUT_24BPP;
	}
	else
	{
		bpp = MDDI_OUT_16BPP;
	}
	
	ret = platform_driver_register(&this_driver);
	if (!ret) 
	{
		pinfo = &nt35510_panel_data.panel_info;
		pinfo->xres = 480;
		pinfo->yres = 800;
		pinfo->type = MDDI_PANEL;
		pinfo->pdest = DISPLAY_1;
		pinfo->mddi.vdopkt = MDDI_DEFAULT_PRIM_PIX_ATTR;
		pinfo->wait_cycle = 0;
		pinfo->bpp = (uint32)bpp;
		pinfo->fb_num = 2;
        pinfo->clk_rate = 192000000;
	    pinfo->clk_min = 192000000;
	    pinfo->clk_max = 192000000;
        pinfo->lcd.vsync_enable = TRUE;
        pinfo->lcd.refx100 = 5500;
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
		}
	}

	return ret;
}
module_init(nt35510_init);
