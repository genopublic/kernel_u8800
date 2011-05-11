/* drivers\video\msm\hw_mddi_lcd.h
 * LCD driver for 7x30 platform
 *
 * Copyright (C) 2010 HUAWEI Technology Co., ltd.
 * 
 * Date: 2010/12/07
 * 
 */
#ifndef HW_MDDI_LCD_H
#define HW_MDDI_LCD_H

/* MDDI interface type 1 or 2 */
typedef enum
{
    MDDI_TYPE1 = 0,
    MDDI_TYPE2,
    MDDI_MAX_TYPE
}mddi_type;
/* MDDI output bpp type */
typedef enum
{
    MDDI_OUT_16BPP = 16,
    MDDI_OUT_24BPP = 24,
    MDDI_OUT_MAX_BPP = 0xFF
}bpp_type;

mddi_type mddi_port_type_probe(void);

#endif
