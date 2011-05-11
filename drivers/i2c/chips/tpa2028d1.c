/* drivers\i2c\chips\tpa2028d1.c
 *
 * Copyright (C) 2009 HUAWEI Corporation.
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
#include <linux/i2c.h>
#include <linux/io.h>
#include <mach/gpio.h>
#include <linux/earlysuspend.h>
#include <linux/audio_amplifier.h>
#include <linux/delay.h>

#define REG1_DEFAULT_VALUE 0xc3
//#define TPA_DEBUG
#ifdef TPA_DEBUG
#define TPA_DEBUG_TPA(fmt, args...) printk(KERN_INFO fmt, ##args)
#else
#define TPA_DEBUG_TPA(fmt, args...)
#endif
#define TPA2028D1_I2C_NAME "tpa2028d1"
static struct i2c_client *g_client;

static int tpa2028d1_i2c_write(char *txData, int length)
{

	struct i2c_msg msg[] = {
		{
		 .addr = g_client->addr,
		 .flags = 0,
		 .len = length,
		 .buf = txData,
		 },
	};

	if (i2c_transfer(g_client->adapter, msg, 1) < 0) 
    {
		TPA_DEBUG_TPA("tpa2028d1_i2c_write: transfer error\n");
		return -EIO;
	} 
    else
    {
        return 0;
    }
}
static int tpa2028d1_i2c_read(char * reg, char *rxData)
{
    
	struct i2c_msg msgs[] = {
		{
		 .addr = g_client->addr,
		 .flags = 0,
		 .len = 1,
		 .buf = reg,
		 },
		{
		 .addr = g_client->addr,
		 .flags = I2C_M_RD,
		 .len = 1,
		 .buf = rxData,
		 },
	};

	if (i2c_transfer(g_client->adapter, msgs, 2) < 0) 
    {
		TPA_DEBUG_TPA("tpa2028d1_i2c_read: transfer error\n");
		return -EIO;
	} 
    else
	{
        TPA_DEBUG_TPA("reg(0x%x)'s value:0x%x\n",*reg, *rxData);
		return 0;
    }
}
static char en_data[8][2] = 
{
    /* 26dB open agc */
	/* 20dB open agc */
	/* since the whistling issue is solved, agc gain set to 30dB to louder sound*/
    /* agc gain set to 12dB to eliminate screaming */
    /*26dB, add {0x01, 0xc3} at last */
    {0x01, 0x83},
    {0x02, 0x05},
    {0x03, 0x0a},
    {0x04, 0x00},
    {0x05, 0x1a},
    {0x06, 0x3c},
    {0x07, 0x82},
    {0x01, 0xc3}
    
};
void tpa2028d1_amplifier_on(void)
{
    u8 * en_datap = &(en_data[0][0]);
    int ret = 0;
    char i = 0;
    //u8 r_data;
     /*enable amplifier*/
    msleep(10);
    for (i = 0; i < ARRAY_SIZE(en_data); i++)
    {
         ret = tpa2028d1_i2c_write(en_datap, 2);
         if(ret)
            break;
         en_datap += 2;
    }
    if(ret)
        TPA_DEBUG_TPA("failed to turn on tpa2028d1_amplifier\n");
    else
        TPA_DEBUG_TPA("tpa2028d1_amplifier_on\n");
    /*
    for(i = 1; i<8; i++)
    {
        tpa2028d1_i2c_read(&i, &r_data);
    }
    */
}

void tpa2028d1_amplifier_off(void)
{
    TPA_DEBUG_TPA("tpa2028d1_amplifier_off\n");
}

static char en_4music_data[8][2] = 
{
    /* open AGC 30db for playing music only */
    /* accordingly, need to check if iir filter need modify and volume percentage in hwVolumeFactor.cfg */
	{0x01, 0x83},
    {0x02, 0x05},
    {0x03, 0x0a},
    {0x04, 0x00},
    {0x05, 0x1e},
    {0x06, 0x5c},
    {0x07, 0xc2},
    {0x01, 0xc3}
    
};

static void tpa2028d1_amplifier_4music_on(void)
{
    u8 * en_datap = &(en_4music_data[0][0]);
    int ret = 0;
    char i = 0;

     /*enable amplifier*/
    msleep(10);
    for (i = 0; i < ARRAY_SIZE(en_4music_data); i++)
    {
         ret = tpa2028d1_i2c_write(en_datap, 2);
         if(ret)
            break;
         en_datap += 2;
    }
    if(ret)
        TPA_DEBUG_TPA("failed to turn on tpa2028d1_amplifier\n");
    else
        TPA_DEBUG_TPA("tpa2028d1_amplifier_4music_on\n");

}

static int tpa2028d1_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
       
	char r_data, reg = 0x01;
    int ret = 0;
    struct amplifier_platform_data *pdata = client->dev.platform_data;

	TPA_DEBUG_TPA("tpa2028d1_probe\n");
	
	g_client = client;
    gpio_set_value(82, 1);	/* enable spkr poweramp */
    msleep(10);
    /*identify if this is  tpa2028d1*/
    ret = tpa2028d1_i2c_read(&reg, &r_data);
    
    gpio_set_value(82, 0);	/* disable spkr poweramp */
    if(!ret && (REG1_DEFAULT_VALUE == r_data) && pdata)
    {
        pdata->amplifier_on = tpa2028d1_amplifier_on;
        pdata->amplifier_off = tpa2028d1_amplifier_off;
        #ifdef CONFIG_HUAWEI_KERNEL
        pdata->amplifier_4music_on = tpa2028d1_amplifier_4music_on;
        #endif
    }
    return ret;
}


static int tpa2028d1_remove(struct i2c_client *client)
{
    struct amplifier_platform_data *pdata = client->dev.platform_data;
    if(pdata)
    {
        pdata->amplifier_on = NULL;
        pdata->amplifier_off = NULL;
    }
	return 0;
}


static const struct i2c_device_id tpa2028d1_id[] = {
	{TPA2028D1_I2C_NAME, 0},
	{ }
};

static struct i2c_driver tpa2028d1_driver = {
	.probe		= tpa2028d1_probe,
	.remove		= tpa2028d1_remove,
	.id_table	= tpa2028d1_id,
	.driver = {
	    .name	= TPA2028D1_I2C_NAME,
	},
};

static int __devinit tpa2028d1_init(void)
{
    TPA_DEBUG_TPA("add tpa2028d1 driver\n");
	return i2c_add_driver(&tpa2028d1_driver);
}

static void __exit tpa2028d1_exit(void)
{
	i2c_del_driver(&tpa2028d1_driver);
}

module_init(tpa2028d1_init);
module_exit(tpa2028d1_exit);

MODULE_DESCRIPTION("tpa2028d1 Driver");
MODULE_LICENSE("GPL");
