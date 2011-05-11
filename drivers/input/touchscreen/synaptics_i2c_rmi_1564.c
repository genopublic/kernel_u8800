
/* drivers/input/keyboard/synaptics_i2c_rmi.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (C) 2008 Texas Instrument Inc.
 * Copyright (C) 2009 Synaptics, Inc.
 *
 * provides device files /dev/input/event#
 * for named device files, use udev
 * 2D sensors report ABS_X_FINGER(0), ABS_Y_FINGER(0) through ABS_X_FINGER(7), ABS_Y_FINGER(7)
 * NOTE: requires updated input.h, which should be included with this driver
 * 1D/Buttons report BTN_0 through BTN_0 + button_count
 * TODO: report REL_X, REL_Y for flick, BTN_TOUCH for tap (on 1D/0D; done for 2D)
 * TODO: check ioctl (EVIOCGABS) to query 2D max X & Y, 1D button count
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

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/synaptics_i2c_rmi_1564.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <linux/device.h>
#include <mach/vreg.h>
#include <mach/gpio.h>
#include <asm/mach-types.h>
#define BTN_F19 BTN_0
#define BTN_F30 BTN_0
#define SCROLL_ORIENTATION REL_Y


//#define TS_RMI_DEBUG
#undef TS_RMI_DEBUG 
#ifdef TS_RMI_DEBUG
#define TS_DEBUG_RMI(fmt, args...) printk(KERN_INFO fmt, ##args)
#else
#define TS_DEBUG_RMI(fmt, args...)
#endif

/*use this to contrl the debug message*/
static int synaptics_debug_mask;
module_param_named(synaptics_debug, synaptics_debug_mask, int,
		S_IRUGO | S_IWUSR | S_IWGRP);

#define DBG_MASK(x...) do {\
	if (synaptics_debug_mask) \
		printk(KERN_DEBUG x);\
	} while (0)


static struct workqueue_struct *synaptics_wq;

/* Register: EGR_0 */
#define EGR_PINCH_REG		0
#define EGR_PINCH 		(1 << 6)
#define EGR_PRESS_REG 		0
#define EGR_PRESS 		(1 << 5)
#define EGR_FLICK_REG 		0
#define EGR_FLICK 		(1 << 4)
#define EGR_EARLY_TAP_REG	0
#define EGR_EARLY_TAP		(1 << 3)
#define EGR_DOUBLE_TAP_REG	0
#define EGR_DOUBLE_TAP		(1 << 2)
#define EGR_TAP_AND_HOLD_REG	0
#define EGR_TAP_AND_HOLD	(1 << 1)
#define EGR_SINGLE_TAP_REG	0
#define EGR_SINGLE_TAP		(1 << 0)
/* Register: EGR_1 */
#define EGR_PALM_DETECT_REG	1
#define EGR_PALM_DETECT		(1 << 0)

#define GPIO_TOUCH_INT 148
#define F01_RMI_CTRL00 0x25
/* kernel29 -> kernel32 driver modify*/

/* delete some lines which is not needed anymore*/
/* Past to beginning of Relative axes events just past Relative axes comment. */
#define FINGER_MAX 9
#define FINGER_CNT (FINGER_MAX+1)
#define SYNAPTICS_I2C_RMI_NAME "Synaptics_RMI4"
#define F11_2D_CTRL14 0x0065
#define SENSITIVE 8
#define F11_2D_CTRL00 0x0027
#define REPORTING_MODE 0x00


/* Past at end of the Absolute axes events - replace ABS_MAX with this one. */

#define ABS_XF			0
#define ABS_YF			1
#define ABS_ZF			2
#define EVENTS_PER_FINGER	3
#define ABS_FINGER(f)		(0x29 + EVENTS_PER_FINGER * f)

#define ABS_X_FINGER(f)		(ABS_FINGER(f) + ABS_XF)
#define ABS_Y_FINGER(f)		(ABS_FINGER(f) + ABS_YF)
#define ABS_Z_FINGER(f)		(ABS_FINGER(f) + ABS_ZF)
#define ABS_CNT			(ABS_MAX+1)


struct synaptics_function_descriptor {
	__u8 queryBase;
	__u8 commandBase;
	__u8 controlBase;
	__u8 dataBase;
/* delete some lines which is not needed anymore*/
	__u8 intSrc;
#define FUNCTION_VERSION(x) ((x >> 5) & 3)
#define INTERRUPT_SOURCE_COUNT(x) (x & 7)

	__u8 functionNumber;
};
#define FD_ADDR_MAX 0xE9
#define FD_ADDR_MIN 0x05
#define FD_BYTE_COUNT 6

#define MIN_ACTIVE_SPEED 5
 
#define TS_X_MAX     1011
#define TS_Y_MAX     1825
#define TS_X_OFFSET  3*(TS_X_MAX/TS_Y_MAX)
#define TS_Y_OFFSET  TS_X_OFFSET
#ifdef CONFIG_HUAWEI_TOUCHSCREEN_EXTRA_KEY

/*max y for the key region. total height of the touchscreen is 91.5mm,
and the height of the key region is 8.5mm, TS_Y_MAX * 8.5 /91.5 */
#define TS_KEY_Y_MAX 168 


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef min
#define min(a,b) ((a)>(b)?(b):(a))
#endif
#ifndef max
#define max(a,b) ((b)>(a)?(b):(a))
#endif
#ifndef abs
#define abs(a)  ((0 < (a)) ? (a) : -(a))
#endif


#define X_START    (0)
#define X_END      (TS_X_MAX) 
#define Y_START    (TS_Y_MAX-TS_KEY_Y_MAX)
#define Y_END      (TS_Y_MAX)

#define EXTRA_MAX_TOUCH_KEY    4
#define TS_KEY_DEBOUNCE_TIMER_MS 60


/* to define a region of touch panel */
typedef struct
{
    u16 touch_x_start;
    u16 touch_x_end;
    u16 touch_y_start;
    u16 touch_y_end;
} touch_region;

/* to define virt button of touch panel */
typedef struct 
{
    u16  center_x;
    u16  center_y;
    u16  x_width;
    u16  y_width;
    u32   touch_keycode;
} button_region;

/* to define extra touch region and virt key region */
typedef struct
{
    touch_region   extra_touch_region;
    button_region  extra_key[EXTRA_MAX_TOUCH_KEY];
} extra_key_region;

/* to record keycode */
typedef struct {
	u32                 record_extra_key;             /*key value*/   
	bool                bRelease;                     /*be released?*/   
	bool                bSentPress;                  
	bool                touch_region_first;           /* to record first touch event*/
} RECORD_EXTRA_KEYCODE;
/*modify the value of HOME key*/ 
/* to init extra region and touch virt key region */
static extra_key_region   touch_extra_key_region =
{
    {X_START, X_END,Y_START,Y_END},								/* extra region */
    {
/* the value 24 (the gap between touch region and key region)maybe need to modify*/
       {(TS_X_MAX*1/8),   (TS_Y_MAX-TS_KEY_Y_MAX/2+59), TS_X_MAX/10, TS_KEY_Y_MAX/2, KEY_BACK},  /*back key */
       {(TS_X_MAX*3/8),   (TS_Y_MAX-TS_KEY_Y_MAX/2+59), TS_X_MAX/10, TS_KEY_Y_MAX/2, KEY_MENU},  /* menu key */
       {(TS_X_MAX*5/8),   (TS_Y_MAX-TS_KEY_Y_MAX/2+59), TS_X_MAX/10, TS_KEY_Y_MAX/2, KEY_HOME },  /* KEY_F2,KEY_HOME home key */
       {(TS_X_MAX*7/8),   (TS_Y_MAX-TS_KEY_Y_MAX/2+59), TS_X_MAX/10, TS_KEY_Y_MAX/2, KEY_SEARCH},  /* Search key */
    },
};

/* to record the key pressed */
/* delete some lines which is not needed anymore*/
#endif 

/* define in platform/board file(s) */
extern struct i2c_device_id synaptics_rmi4_id[];

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_rmi4_early_suspend(struct early_suspend *h);
static void synaptics_rmi4_late_resume(struct early_suspend *h);
#endif

static int synaptics_rmi4_read_pdt(struct synaptics_rmi4 *ts)
{
	int ret = 0;
	int nFd = 0;
	int interruptCount = 0;
	__u8 data_length;

	struct i2c_msg fd_i2c_msg[2];
	__u8 fd_reg;
	struct synaptics_function_descriptor fd;

	struct i2c_msg query_i2c_msg[2];
	__u8 query[14];
	__u8 *egr;

	fd_i2c_msg[0].addr = ts->client->addr;
	fd_i2c_msg[0].flags = 0;
	fd_i2c_msg[0].buf = &fd_reg;
	fd_i2c_msg[0].len = 1;

	fd_i2c_msg[1].addr = ts->client->addr;
	fd_i2c_msg[1].flags = I2C_M_RD;
	fd_i2c_msg[1].buf = (__u8 *)(&fd);
	fd_i2c_msg[1].len = FD_BYTE_COUNT;

	query_i2c_msg[0].addr = ts->client->addr;
	query_i2c_msg[0].flags = 0;
	query_i2c_msg[0].buf = &fd.queryBase;
	query_i2c_msg[0].len = 1;

	query_i2c_msg[1].addr = ts->client->addr;
	query_i2c_msg[1].flags = I2C_M_RD;
	query_i2c_msg[1].buf = query;
	query_i2c_msg[1].len = sizeof(query);


	ts->hasF11 = false;
	ts->hasF19 = false;
	ts->hasF30 = false;
	ts->data_reg = 0xff;
	ts->data_length = 0;


	for (fd_reg = FD_ADDR_MAX; fd_reg >= FD_ADDR_MIN; fd_reg -= FD_BYTE_COUNT)     
    {
		ret = i2c_transfer(ts->client->adapter, fd_i2c_msg, 2);
		if (ret < 0) {
			printk(KERN_ERR "I2C read failed querying RMI4 $%02X capabilities\n", ts->client->addr);
			return ret;
		}
/* delete some lines which is not needed anymore*/
		if (!fd.functionNumber) 
        {
			/* End of PDT */
			ret = nFd;
			TS_DEBUG_RMI("Read %d functions from PDT\n", fd.functionNumber);
			break;
		}

		++nFd;

		switch (fd.functionNumber) {
			case 0x01: /* Interrupt */
				ts->f01.data_offset = fd.dataBase;
				/*
				 * Can't determine data_length
				 * until whole PDT has been read to count interrupt sources
				 * and calculate number of interrupt status registers.
				 * Setting to 0 safely "ignores" for now.
				 */
				data_length = 0;
				break;
			case 0x11: /* 2D */
				ts->hasF11 = true;

				ts->f11.data_offset = fd.dataBase;
				ts->f11.interrupt_offset = interruptCount / 8;
				ts->f11.interrupt_mask = ((1 << INTERRUPT_SOURCE_COUNT(fd.intSrc)) - 1) << (interruptCount % 8);

				ret = i2c_transfer(ts->client->adapter, query_i2c_msg, 2);
				if (ret < 0)
					printk(KERN_ERR "Error reading F11 query registers\n");

				ts->f11.points_supported = (query[1] & 7) + 1;
				if (ts->f11.points_supported == 6)
					ts->f11.points_supported = 10;

				ts->f11_fingers = kcalloc(ts->f11.points_supported,
				                          sizeof(*ts->f11_fingers), 0);

				TS_DEBUG_RMI("%d fingers\n", ts->f11.points_supported);

				ts->f11_has_gestures = (query[1] >> 5) & 1;
				ts->f11_has_relative = (query[1] >> 3) & 1;
				/* if the sensitivity adjust exist */
                ts->f11_has_Sensitivity_Adjust = (query[1] >> 6) & 1;
				egr = &query[7];

 
				TS_DEBUG_RMI("EGR features:\n");
				ts->hasEgrPinch = egr[EGR_PINCH_REG] & EGR_PINCH;
				TS_DEBUG_RMI("\tpinch: %u\n", ts->hasEgrPinch);
				ts->hasEgrPress = egr[EGR_PRESS_REG] & EGR_PRESS;
				TS_DEBUG_RMI("\tpress: %u\n", ts->hasEgrPress);
				ts->hasEgrFlick = egr[EGR_FLICK_REG] & EGR_FLICK;
				TS_DEBUG_RMI("\tflick: %u\n", ts->hasEgrFlick);
				ts->hasEgrEarlyTap = egr[EGR_EARLY_TAP_REG] & EGR_EARLY_TAP;
				TS_DEBUG_RMI("\tearly tap: %u\n", ts->hasEgrEarlyTap);
				ts->hasEgrDoubleTap = egr[EGR_DOUBLE_TAP_REG] & EGR_DOUBLE_TAP;
				TS_DEBUG_RMI("\tdouble tap: %u\n", ts->hasEgrDoubleTap);
				ts->hasEgrTapAndHold = egr[EGR_TAP_AND_HOLD_REG] & EGR_TAP_AND_HOLD;
				TS_DEBUG_RMI("\ttap and hold: %u\n", ts->hasEgrTapAndHold);
				ts->hasEgrSingleTap = egr[EGR_SINGLE_TAP_REG] & EGR_SINGLE_TAP;
				TS_DEBUG_RMI("\tsingle tap: %u\n", ts->hasEgrSingleTap);
				ts->hasEgrPalmDetect = egr[EGR_PALM_DETECT_REG] & EGR_PALM_DETECT;
				TS_DEBUG_RMI("\tpalm detect: %u\n", ts->hasEgrPalmDetect);


				query_i2c_msg[0].buf = &fd.controlBase;
				ret = i2c_transfer(ts->client->adapter, query_i2c_msg, 2);
				if (ret < 0)
					printk(KERN_ERR "Error reading F11 control registers\n");

				query_i2c_msg[0].buf = &fd.queryBase;

				ts->f11_max_x = ((query[7] & 0x0f) * 0x100) | query[6];
				ts->f11_max_y = ((query[9] & 0x0f) * 0x100) | query[8];

				TS_DEBUG_RMI("max X: %d; max Y: %d\n", ts->f11_max_x, ts->f11_max_y);

				ts->f11.data_length = data_length =
					/* finger status, four fingers per register */
					((ts->f11.points_supported + 3) / 4)
					/* absolute data, 5 per finger */
					+ 5 * ts->f11.points_supported
					/* two relative registers */
					+ (ts->f11_has_relative ? 2 : 0)
					/* F11_2D_Data8 is only present if the egr_0 register is non-zero. */
					+ (egr[0] ? 1 : 0)
					/* F11_2D_Data9 is only present if either egr_0 or egr_1 registers are non-zero. */
					+ ((egr[0] || egr[1]) ? 1 : 0)
					/* F11_2D_Data10 is only present if EGR_PINCH or EGR_FLICK of egr_0 reports as 1. */
					+ ((ts->hasEgrPinch || ts->hasEgrFlick) ? 1 : 0)
					/* F11_2D_Data11 and F11_2D_Data12 are only present if EGR_FLICK of egr_0 reports as 1. */
					+ (ts->hasEgrFlick ? 2 : 0)
					;

				break;
 			case 0x30: /* GPIO */
				ts->hasF30 = true;

				ts->f30.data_offset = fd.dataBase;
				ts->f30.interrupt_offset = interruptCount / 8;
				ts->f30.interrupt_mask = ((1 < INTERRUPT_SOURCE_COUNT(fd.intSrc)) - 1) << (interruptCount % 8);

				ret = i2c_transfer(ts->client->adapter, query_i2c_msg, 2);
				if (ret < 0)
					printk(KERN_ERR "Error reading F30 query registers\n");


				ts->f30.points_supported = query[1] & 0x1F;
				ts->f30.data_length = data_length = (ts->f30.points_supported + 7) / 8;

				break;
			default:
				goto pdt_next_iter;
		}

		/* Change to end address for comparison
		  NOTE: make sure final value of ts->data_reg is subtracted */
		data_length += fd.dataBase;
		if (data_length > ts->data_length) {
			ts->data_length = data_length;
		}

		if (fd.dataBase < ts->data_reg) {
			ts->data_reg = fd.dataBase;
		}

pdt_next_iter:
		interruptCount += INTERRUPT_SOURCE_COUNT(fd.intSrc);
	}

	/* Now that PDT has been read, interrupt count determined, F01 data length can be determined.*/
	ts->f01.data_length = data_length = 1 + ((interruptCount + 7) / 8);
	/* Change to end address for comparison
	NOTE: make sure final value of ts->data_reg is subtracted*/
	data_length += ts->f01.data_offset;
	if (data_length > ts->data_length) {
		ts->data_length = data_length;
	}

	/*Change data_length back from end address to length*/
	/*NOTE: make sure this was an address*/
	ts->data_length -= ts->data_reg;

	/*Change all data offsets to be relative to first register read */
 	ts->f01.data_offset -= ts->data_reg;
	ts->f11.data_offset -= ts->data_reg;
	ts->f19.data_offset -= ts->data_reg;
	ts->f30.data_offset -= ts->data_reg;

	ts->data = kcalloc(ts->data_length, sizeof(*ts->data), 0);
	if (ts->data == NULL) {
		printk(KERN_ERR "Not enough memory to allocate space for RMI4 data\n");
		ret = -ENOMEM;
	}

	ts->data_i2c_msg[0].addr = ts->client->addr;
	ts->data_i2c_msg[0].flags = 0;
	ts->data_i2c_msg[0].len = 1;
	ts->data_i2c_msg[0].buf = &ts->data_reg;

	ts->data_i2c_msg[1].addr = ts->client->addr;
	ts->data_i2c_msg[1].flags = I2C_M_RD;
	ts->data_i2c_msg[1].len = ts->data_length;
	ts->data_i2c_msg[1].buf = ts->data;

	printk(KERN_ERR "RMI4 $%02X data read: $%02X + %d\n",
        	ts->client->addr, ts->data_reg, ts->data_length);

	return ret;
}
#ifdef CONFIG_HUAWEI_TOUCHSCREEN_EXTRA_KEY
/*===========================================================================
FUNCTION      is_in_extra_region
DESCRIPTION
              是否在附加TOUCH区
DEPENDENCIES
  None
RETURN VALUE
  true or false
SIDE EFFECTS
  None
===========================================================================*/
static bool is_in_extra_region(int pos_x, int pos_y)
{
    if (pos_x >= touch_extra_key_region.extra_touch_region.touch_x_start
        && pos_x <= touch_extra_key_region.extra_touch_region.touch_x_end
        && pos_y >= touch_extra_key_region.extra_touch_region.touch_y_start
        && pos_y <= touch_extra_key_region.extra_touch_region.touch_y_end)
    {
		TS_DEBUG_RMI("the point is_in_extra_region \n");
		return TRUE;
    }

    return FALSE;
}
/*===========================================================================
FUNCTION      touch_get_extra_keycode
DESCRIPTION
              取得附加区键值
DEPENDENCIES
  None
RETURN VALUE
  KEY_VALUE
SIDE EFFECTS
  None
===========================================================================*/
static u32 touch_get_extra_keycode(int pos_x, int pos_y)
{
    int i = 0;
    u32  touch_keycode = KEY_RESERVED;
    for (i=0; i<EXTRA_MAX_TOUCH_KEY; i++)
    {
        if (abs(pos_x - touch_extra_key_region.extra_key[i].center_x) <= touch_extra_key_region.extra_key[i].x_width
         && abs(pos_y - touch_extra_key_region.extra_key[i].center_y) <= touch_extra_key_region.extra_key[i].y_width )
        {
	        touch_keycode = touch_extra_key_region.extra_key[i].touch_keycode;
	        break;
        }
    }
	
	TS_DEBUG_RMI("touch_keycode = %d \n",touch_keycode);
    return touch_keycode;
}
#endif

static void synaptics_rmi4_work_func(struct work_struct *work)
{
	int ret;
/* delete some lines*/
    __u8 finger_status = 0x00;
    
    #ifdef CONFIG_HUAWEI_TOUCHSCREEN_EXTRA_KEY
	u32 key_tmp = 0;
    static u32 key_tmp_old = 0;
	static u32 key_pressed1 = 0;
    #endif

    __u8 reg = 0;
    __u8 *finger_reg = NULL;
    u12 x = 0;
    u12 y = 0;
    u4 wx = 0;
    u4 wy = 0;
    u8 z = 0 ;

	struct synaptics_rmi4 *ts = container_of(work,
					struct synaptics_rmi4, work);

	ret = i2c_transfer(ts->client->adapter, ts->data_i2c_msg, 2);

	if (ret < 0) {
		printk(KERN_ERR "%s: i2c_transfer failed\n", __func__);
	}
    else /* else with "i2c_transfer's return value"*/
	{
		__u8 *interrupt = &ts->data[ts->f01.data_offset + 1];
		if (ts->hasF11 && interrupt[ts->f11.interrupt_offset] & ts->f11.interrupt_mask) 
        {
            __u8 *f11_data = &ts->data[ts->f11.data_offset];

            int f = 0;
			__u8 finger_status_reg = 0;
			__u8 fsr_len = (ts->f11.points_supported + 3) / 4;
			//int fastest_finger = -1;
			int touch = 0;
            TS_DEBUG_RMI("f11.points_supported is %d\n",ts->f11.points_supported);
            if(ts->is_support_multi_touch)
            {

                for (f = 0; f < ts->f11.points_supported; ++f) 
                {

                	if (!(f % 4))
                        
                	finger_status_reg = f11_data[f / 4];

                	finger_status = (finger_status_reg >> ((f % 4) * 2)) & 3;

                	reg = fsr_len + 5 * f;
                	finger_reg = &f11_data[reg];

                	x = (finger_reg[0] * 0x10) | (finger_reg[2] % 0x10);
                	y = (finger_reg[1] * 0x10) | (finger_reg[2] / 0x10);
                	wx = finger_reg[3] % 0x10;
                	wy = finger_reg[3] / 0x10;
                	z = finger_reg[4];
                    DBG_MASK("the x is %d the y is %d the stauts is %d!\n",x,y,finger_status);
                	/* Linux 2.6.31 multi-touch */
                	input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, f + 1);
                	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
                	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
                	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, z);
                	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MINOR, min(wx, wy));
                	input_report_abs(ts->input_dev, ABS_MT_ORIENTATION, (wx > wy ? 1 : 0));
                    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, z);
                    input_mt_sync(ts->input_dev);  
                    DBG_MASK("the touch inout is ok!\n");
#ifdef CONFIG_HUAWEI_TOUCHSCREEN_EXTRA_KEY
                	/* if the point is not the first one the point 
                	 *location will not be throw into toucscreen extra
                	 *key area */
                	if(is_in_extra_region(x, y) && (0 == f))
                	{
                		key_tmp = touch_get_extra_keycode(x, y);
                        /*save the key value for some times the value is null*/
                        if((key_tmp_old != key_tmp) && (0 != key_tmp))
                        {
                            key_tmp_old = key_tmp;
                        }
                        /*when the key is changged report the first release*/
                        if(key_tmp_old && (key_tmp_old != key_tmp))
                        {
                            input_report_key(ts->key_input, key_tmp_old, 0);
                    		key_pressed1 = 0;
                            DBG_MASK("when the key is changged report the first release!\n");
                        }

                		if(key_tmp)
                		{
                    		if (0 == finger_status)//release bit
                    		{
                    			if(1 == key_pressed1)
                    			{ 

                                    input_report_key(ts->key_input, key_tmp, 0);
                    				key_pressed1 = 0;
                                    DBG_MASK("when the key is released report!\n");
                    			}
                    		}
                    		else
                    		{
                    			if(0 == key_pressed1)
                    			{
                                    input_report_key(ts->key_input, key_tmp, 1);
                                    key_pressed1 = 1;
                                    DBG_MASK("the key is pressed report!\n");
                    			}
                    		}    
                		}
                        input_sync(ts->key_input);	
                	}
                    /*when the touch is out of key area report the last key release*/
                    else
                    {
                        if(0 == f)
                        { 
                            if(1 == key_pressed1)
                            {
                                input_report_key(ts->key_input, key_tmp_old, 0);
                                input_sync(ts->key_input);                      
                                DBG_MASK("when the touch is out of key area report the last key release!\n");    
                                key_pressed1 = 0;
                            }
                        }

                    }
#endif
                    ts->f11_fingers[f].status = finger_status;
                    input_report_key(ts->input_dev, BTN_TOUCH, touch);
                }
            }
            else /* else with "if(ts->is_support_multi_touch)"*/
            {
    			finger_status_reg = f11_data[0];
                finger_status = (finger_status_reg & 3);
                TS_DEBUG_RMI("the finger_status is %2d!\n",finger_status);
          
                reg = fsr_len;
                finger_reg = &f11_data[reg];
                x = (finger_reg[0] * 0x10) | (finger_reg[2] % 0x10);
                y = (finger_reg[1] * 0x10) | (finger_reg[2] / 0x10);
				wx = finger_reg[3] % 0x10;
				wy = finger_reg[3] / 0x10;
				z = finger_reg[4];

                TS_DEBUG_RMI(KERN_ERR "the x_sig is %2d ,the y_sig is %2d \n",x, y);

                input_report_abs(ts->input_dev, ABS_X, x);
				input_report_abs(ts->input_dev, ABS_Y, y);

				input_report_abs(ts->input_dev, ABS_PRESSURE, z);
				input_report_abs(ts->input_dev, ABS_TOOL_WIDTH, z);
                input_report_key(ts->input_dev, BTN_TOUCH, finger_status);
                input_sync(ts->input_dev);
            
                
#ifdef CONFIG_HUAWEI_TOUCHSCREEN_EXTRA_KEY
                if(is_in_extra_region(x, y))
                {
                    key_tmp = touch_get_extra_keycode(x, y);
                    /*save the key value for some times the value is null*/
                    if((key_tmp_old != key_tmp) && (0 != key_tmp))
                    {
                        key_tmp_old = key_tmp;
                    }
                    /*when the key is changged report the first release*/
                    if(key_tmp_old && (key_tmp_old != key_tmp))
                    {
                        input_report_key(ts->key_input, key_tmp_old, 0);
                        key_pressed1 = 0;
                        DBG_MASK("when the key is changged report the first release!\n");
                    }
                
                    if(key_tmp)
                    {
                        if (0 == finger_status)//release bit
                        {
                            if(1 == key_pressed1)
                            { 
                                input_report_key(ts->key_input, key_tmp, 0);
                                key_pressed1 = 0;
                                DBG_MASK("when the key is released report!\n");
                            }
                        }
                        else
                        {
                            if(0 == key_pressed1)
                            {
                                input_report_key(ts->key_input, key_tmp, 1);
                                key_pressed1 = 1;
                                DBG_MASK("the key is pressed report!\n");
                            }
                        }    
                    }
                    input_sync(ts->key_input);  
                }
                /*when the touch is out of key area report the last key release*/
                else
                {
                    if(1 == key_pressed1)
                    {
                        input_report_key(ts->key_input, key_tmp_old, 0);
                        input_sync(ts->key_input);                      
                        DBG_MASK("when the touch is out of key area report the last key release!\n");    
                        key_pressed1 = 0;
                    }
                }
#endif
            }


 
            /* f == ts->f11.points_supported */
			/* set f to offset after all absolute data */
			f = (f + 3) / 4 + f * 5;
			if (ts->f11_has_relative) 
            {
				/* NOTE: not reporting relative data, even if available */
				/* just skipping over relative data registers */
				f += 2;
			}
			if (ts->hasEgrPalmDetect) 
            {
             	input_report_key(ts->input_dev,
	                 BTN_DEAD,
	                 f11_data[f + EGR_PALM_DETECT_REG] & EGR_PALM_DETECT);
			}
			if (ts->hasEgrFlick) 
            {
             	if (f11_data[f + EGR_FLICK_REG] & EGR_FLICK) 
                {
					input_report_rel(ts->input_dev, REL_X, f11_data[f + 2]);
					input_report_rel(ts->input_dev, REL_Y, f11_data[f + 3]);
				}
			}
			if (ts->hasEgrSingleTap) 
            {
				input_report_key(ts->input_dev,
				                 BTN_TOUCH,
				                 f11_data[f + EGR_SINGLE_TAP_REG] & EGR_SINGLE_TAP);
			}
			if (ts->hasEgrDoubleTap) 
            {
				input_report_key(ts->input_dev,
				                 BTN_TOOL_DOUBLETAP,
				                 f11_data[f + EGR_DOUBLE_TAP_REG] & EGR_DOUBLE_TAP);
			}
        }

		if (ts->hasF19 && interrupt[ts->f19.interrupt_offset] & ts->f19.interrupt_mask) 
        {
			int reg;
			int touch = 0;
			for (reg = 0; reg < ((ts->f19.points_supported + 7) / 8); reg++)
			{
				if (ts->data[ts->f19.data_offset + reg]) 
                {
					touch = 1;
				   	break;
				}
			}
			input_report_key(ts->input_dev, BTN_DEAD, touch);

		}
    		input_sync(ts->input_dev);
	}

	if (ts->use_irq)
		enable_irq(ts->client->irq);
}

static enum hrtimer_restart synaptics_rmi4_timer_func(struct hrtimer *timer)
{
	struct synaptics_rmi4 *ts = container_of(timer, \
					struct synaptics_rmi4, timer);

	queue_work(synaptics_wq, &ts->work);

	hrtimer_start(&ts->timer, ktime_set(0, 12 * NSEC_PER_MSEC), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

irqreturn_t synaptics_rmi4_irq_handler(int irq, void *dev_id)
{
	struct synaptics_rmi4 *ts = dev_id;

	disable_irq_nosync(ts->client->irq);
	queue_work(synaptics_wq, &ts->work);

	return IRQ_HANDLED;
}

static void synaptics_rmi4_enable(struct synaptics_rmi4 *ts)
{  
	if (ts->use_irq)
		enable_irq(ts->client->irq);
	else
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

	ts->enable = 1;
}

static void synaptics_rmi4_disable(struct synaptics_rmi4 *ts)
{

	if (ts->use_irq)
		disable_irq_nosync(ts->client->irq);
	else
		hrtimer_cancel(&ts->timer);

	cancel_work_sync(&ts->work);

	ts->enable = 0;
}

static ssize_t synaptics_rmi4_enable_show(struct device *dev,
                                         struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4 *ts = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", ts->enable);
}

static ssize_t synaptics_rmi4_enable_store(struct device *dev,
                                          struct device_attribute *attr,
                                          const char *buf, size_t count)
{
	struct synaptics_rmi4 *ts = dev_get_drvdata(dev);
	unsigned long val;
	int error;

	error = strict_strtoul(buf, 10, &val);

	if (error)
		return error;

	val = !!val;

	if (val != ts->enable) {
		if (val)
			synaptics_rmi4_enable(ts);
		else
			synaptics_rmi4_disable(ts);
	}

	return count;
}

DEV_ATTR(synaptics_rmi4, enable, 0664);

static int synaptics_rmi4_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	int i;
	int ret = 0;
    struct vreg *v_gp4 = NULL;
    int gpio_config;

	struct synaptics_rmi4 *ts = NULL;

	/* power on touchscreen */   
    v_gp4 = vreg_get(NULL,"gp4");   
    ret = IS_ERR(v_gp4); 
    if(ret)         
        goto err_power_on_failed;    
    ret = vreg_set_level(v_gp4,2700);        
    if (ret)        
        goto err_power_on_failed;    
    ret = vreg_enable(v_gp4);
    TS_DEBUG_RMI("the power is ok\n");
    if (ret)       
        goto err_power_on_failed;
    mdelay(50);
    
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
    {
		printk(KERN_ERR "%s: need I2C_FUNC_I2C\n", __func__);
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
    TS_DEBUG_RMI("the i2c_check_functionality is ok \n");

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) 
    {
        printk(KERN_ERR "%s: check zalloc failed!\n", __func__);
        ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
    synaptics_wq = create_singlethread_workqueue("synaptics_wq");
    if (!synaptics_wq)
    {
        printk(KERN_ERR "Could not create work queue synaptics_wq: no memory");
        goto error_wq_creat_failed; 
    }
	INIT_WORK(&ts->work, synaptics_rmi4_work_func);
    ts->is_support_multi_touch = TRUE;
	ts->client = client;
	i2c_set_clientdata(client, ts);

	ret = synaptics_rmi4_read_pdt(ts);
    
	if (ret <= 0) {
		if (ret == 0)
			printk(KERN_ERR "Empty PDT\n");

		printk(KERN_ERR "Error identifying device (%d)\n", ret);
		ret = -ENODEV;
		goto err_pdt_read_failed;
	}

/* we write this reg to changge the sensitive */

    if(ts->f11_has_Sensitivity_Adjust)
    {
        ret = i2c_smbus_write_byte_data(ts->client, F11_2D_CTRL14, SENSITIVE);
        if(ret < 0) 
        {
            printk(KERN_ERR "%s: failed to write err=%d\n", __FUNCTION__,  ret);
	    }  
        else
        {
                TS_DEBUG_RMI("the SENSITIVE is changged ok!\n");
        }
    }
    else
    {
            printk(KERN_ERR "the SENSITIVE is failed to changge!\n");
    }
/*we write this reg to write the reporting mode*/
    ret = i2c_smbus_write_byte_data(ts->client, F11_2D_CTRL00, REPORTING_MODE);
    if(ret < 0) 
    {
        printk(KERN_ERR "%s: ReportingMode failed to write err=%d\n", __FUNCTION__,  ret);
    }  
    else
    {
        TS_DEBUG_RMI("the ReportingMode is changged ok!\n");
    }
    
	ts->input_dev = input_allocate_device();
	if (!ts->input_dev)
    {
		printk(KERN_ERR "failed to allocate input device.\n");
		ret = -EBUSY;
		goto err_alloc_dev_failed;
	}

	ts->input_dev->name = "Synaptics RMI4";
	dev_set_drvdata(&(ts->input_dev->dev), ts);
	ts->input_dev->phys = client->name;
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(ABS_X, ts->input_dev->absbit);
	set_bit(ABS_Y, ts->input_dev->absbit);
/*we removed it to here to register the touchscreen first */
	ret = input_register_device(ts->input_dev);
	if (ret) 
    {
		printk(KERN_ERR "synaptics_rmi4_probe: Unable to register %s \
				input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	} 
    else 
	{
		TS_DEBUG_RMI("synaptics input device registered\n");
	}
	
	if (ts->hasF11) {
		for (i = 0; i < ts->f11.points_supported; ++i) {
          if(ts->is_support_multi_touch)
          {

			/* Linux 2.6.31 multi-touch */
			input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 1,
                    			ts->f11.points_supported, 0, 0);
			input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->f11_max_x, 0, 0);
			input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->f11_max_y - TS_KEY_Y_MAX, 0, 0);
            input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
			input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 0xF, 0, 0);
			input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MINOR, 0, 0xF, 0, 0);
			input_set_abs_params(ts->input_dev, ABS_MT_ORIENTATION, 0, 1, 0, 0);
          }
          else
          {
            input_set_abs_params(ts->input_dev, ABS_X, 0, ts->f11_max_x, 0, 0);
            input_set_abs_params(ts->input_dev, ABS_Y, 0, ts->f11_max_y - TS_KEY_Y_MAX, 0, 0);
            input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
            input_set_abs_params(ts->input_dev, ABS_TOOL_WIDTH, 0, 255, 0, 0);
                
          }

		}
		if (ts->hasEgrPalmDetect)
			set_bit(BTN_DEAD, ts->input_dev->keybit);
		if (ts->hasEgrFlick) {
			set_bit(REL_X, ts->input_dev->keybit);
			set_bit(REL_Y, ts->input_dev->keybit);
		}
		if (ts->hasEgrSingleTap)
			set_bit(BTN_TOUCH, ts->input_dev->keybit);
		if (ts->hasEgrDoubleTap)
			set_bit(BTN_TOOL_DOUBLETAP, ts->input_dev->keybit);
	}
	if (ts->hasF19) {
		set_bit(BTN_DEAD, ts->input_dev->keybit);
	}
	if (ts->hasF30) {
		for (i = 0; i < ts->f30.points_supported; ++i) {
			set_bit(BTN_F30 + i, ts->input_dev->keybit);
		}
	}
#ifdef CONFIG_HUAWEI_TOUCHSCREEN_EXTRA_KEY
/* Set the key value according to productions, the default value is U8800 */
    	if(machine_is_msm7x30_u8820())
		{
			touch_extra_key_region.extra_key[0].touch_keycode = KEY_MENU;
			touch_extra_key_region.extra_key[1].touch_keycode = KEY_HOME;
			touch_extra_key_region.extra_key[2].touch_keycode = KEY_BACK;
			touch_extra_key_region.extra_key[3].touch_keycode = KEY_SEARCH;
		}
		else
		{
			touch_extra_key_region.extra_key[0].touch_keycode = KEY_BACK;
			touch_extra_key_region.extra_key[1].touch_keycode = KEY_MENU;
			touch_extra_key_region.extra_key[2].touch_keycode = KEY_HOME;
			touch_extra_key_region.extra_key[3].touch_keycode = KEY_SEARCH;			
		}
		ts->key_input = input_allocate_device();
		if (!ts->key_input  || !ts) {
			ret = -ENOMEM;
			goto err_input_register_device_failed;
		}
		ts->key_input->name = "touchscreen_key";
		
		set_bit(EV_KEY, ts->key_input->evbit);
		for (i = 0; i < EXTRA_MAX_TOUCH_KEY; i++)
		{
			set_bit(touch_extra_key_region.extra_key[i].touch_keycode & KEY_MAX, ts->key_input->keybit);
		}

		ret = input_register_device(ts->key_input);
		if (ret)
			goto err_key_input_register_device_failed;

#endif


    gpio_config = GPIO_CFG(GPIO_TOUCH_INT, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA);
    
    ret = gpio_tlmm_config(gpio_config, GPIO_ENABLE);
    if (ret) 
	{
		ret = -EIO;
		goto err_key_input_register_device_failed;
	}


	if (client->irq) {
		gpio_request(client->irq, client->name);
		gpio_direction_input(client->irq);

		TS_DEBUG_RMI("Requesting IRQ...\n");

		if (request_irq(client->irq, synaptics_rmi4_irq_handler,
				IRQF_TRIGGER_LOW, client->name, ts) >= 0) {
			TS_DEBUG_RMI("Received IRQ!\n");
			ts->use_irq = 1;
			if (set_irq_wake(client->irq, 1) < 0)
				printk(KERN_ERR "failed to set IRQ wake\n");
		} else {
			TS_DEBUG_RMI("Failed to request IRQ!\n");
		}
	}

	if (!ts->use_irq) {
		printk(KERN_ERR "Synaptics RMI4 device %s in polling mode\n", client->name);
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = synaptics_rmi4_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}

	/*
	 * Device will be /dev/input/event#
	 * For named device files, use udev
	 */

	ts->enable = 1;

	dev_set_drvdata(&ts->input_dev->dev, ts);

	if (sysfs_create_file(&ts->input_dev->dev.kobj, &dev_attr_synaptics_rmi4_enable.attr) < 0)
		printk("failed to create sysfs file for input device\n");

	#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_rmi4_early_suspend;
	ts->early_suspend.resume = synaptics_rmi4_late_resume;
	register_early_suspend(&ts->early_suspend);
	#endif
	printk(KERN_ERR "probing for Synaptics RMI4 device %s at $%02X...\n", client->name, client->addr);
    
	return 0;
err_key_input_register_device_failed:
    if(NULL != ts->key_input)
        input_free_device(ts->key_input);
err_input_register_device_failed:
    if(NULL != ts->input_dev)
	    input_free_device(ts->input_dev);
err_pdt_read_failed:
err_alloc_dev_failed:
error_wq_creat_failed:
    if(NULL != ts)
        kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
    if(NULL != v_gp4)
	{
        ret = vreg_disable(v_gp4);
	 	TS_DEBUG_RMI(KERN_ERR "the power is off: gp4 = %d \n ", ret);	
	}
err_power_on_failed:
    TS_DEBUG_RMI("THE POWER IS FAILED!!!\n");

	return ret;
}


static int synaptics_rmi4_remove(struct i2c_client *client)
{
struct synaptics_rmi4 *ts = i2c_get_clientdata(client);
	unregister_early_suspend(&ts->early_suspend);
	if (ts->use_irq)
		free_irq(client->irq, ts);
	else
		hrtimer_cancel(&ts->timer);
	input_unregister_device(ts->input_dev);
#ifdef CONFIG_HUAWEI_TOUCHSCREEN_EXTRA_KEY
	   input_unregister_device(ts->key_input);
#endif
	kfree(ts);
	return 0;
}

static int synaptics_rmi4_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret;
	struct synaptics_rmi4 *ts = i2c_get_clientdata(client);
/* if use interrupt disable the irq ,else disable timer */ 
    if (ts->use_irq)
	    disable_irq_nosync(client->irq);
	else
		hrtimer_cancel(&ts->timer);

	ret = cancel_work_sync(&ts->work);    
	if (ret && ts->use_irq) /* if work was pending disable-count is now 2 */
    {   
        enable_irq(client->irq);
        printk(KERN_ERR "synaptics_ts_suspend: can't cancel the work ,so enable the irq \n");
    }
    ret = i2c_smbus_write_byte_data(client, F01_RMI_CTRL00, 0x01);
    if(ret < 0)
    {
        printk(KERN_ERR "synaptics_ts_suspend: the touch can't get into deep sleep \n");
    }

	ts->enable = 0;

	return 0;
}

static int synaptics_rmi4_resume(struct i2c_client *client)
{
    int ret;
	struct synaptics_rmi4 *ts = i2c_get_clientdata(client);
    
    
    ret = i2c_smbus_write_byte_data(ts->client, F01_RMI_CTRL00, 0x00);
    if(ret < 0)
    {
        printk(KERN_ERR "synaptics_ts_resume: the touch can't resume! \n");
    }
    mdelay(50);
    if (ts->use_irq) {
		enable_irq(client->irq);
	}
	else
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    printk(KERN_ERR "synaptics_rmi4_touch is resume!\n");

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_rmi4_early_suspend(struct early_suspend *h)
{
	struct synaptics_rmi4 *ts;
	ts = container_of(h, struct synaptics_rmi4, early_suspend);
	synaptics_rmi4_suspend(ts->client, PMSG_SUSPEND);
}

static void synaptics_rmi4_late_resume(struct early_suspend *h)
{
	struct synaptics_rmi4 *ts;
	ts = container_of(h, struct synaptics_rmi4, early_suspend);
	synaptics_rmi4_resume(ts->client);
}
#endif

static const struct i2c_device_id synaptics_ts_id[] = {
	{ "Synaptics_rmi", 0 },
	{ }
};
static struct i2c_driver synaptics_rmi4_driver = {
	.probe		= synaptics_rmi4_probe,
	.remove		= synaptics_rmi4_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= synaptics_rmi4_suspend,
	.resume		= synaptics_rmi4_resume,
#endif
    .id_table   = synaptics_ts_id,
	.driver = {
		.name	= "Synaptics_rmi",
	},
};
static int __devinit synaptics_rmi4_init(void)
{
	return i2c_add_driver(&synaptics_rmi4_driver);
}

static void __exit synaptics_rmi4_exit(void)
{
	i2c_del_driver(&synaptics_rmi4_driver);
	if (synaptics_wq)
		destroy_workqueue(synaptics_wq);
}

module_init(synaptics_rmi4_init);
module_exit(synaptics_rmi4_exit);

MODULE_DESCRIPTION("Synaptics RMI4 Driver");
MODULE_LICENSE("GPL");

