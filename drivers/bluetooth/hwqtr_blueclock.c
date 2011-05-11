/*

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.


   Copyright (C) 2010-2012 - Huawei

   Date         Author           Comment
   -----------  --------------   --------------------------------
   2010-Oct-19  huawei            clock request for QTR8200  

*/

#include <linux/module.h>	/* kernel module definitions */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
//#include <linux/notifier.h>
#include <linux/proc_fs.h>
//#include <linux/spinlock.h>
//#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
//#include <linux/workqueue.h>
//#include <linux/platform_device.h>

#include <mach/rpc_pmapp.h>
#include <mach/vreg.h>
#include <linux/kernel.h>



//#include <linux/irq.h>
#include <linux/param.h>
//#include <linux/bitops.h>
//#include <linux/termios.h>
//#include <mach/gpio.h>
//#include <mach/msm_serial_hs.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h> /* event notifications */
#include "hci_uart.h"





#define BT_CLOCK_DBG
#ifndef BT_CLOCK_DBG
#define BT_DBG(fmt, arg...)
#endif
/*
 * Defines
 */

#define VERSION     "1.0"
#define PROC_DIR    "bluetooth/clock"

struct proc_dir_entry *bluetooth_dir, *clock_dir;


static int blueclock_start(void)
{// here is the main fucntion:

    int rc = 0;
    const char *id = "BTPW";

    printk(KERN_INFO "------blueclock_start is in! \n");

    rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
                        PMAPP_CLOCK_VOTE_PIN_CTRL);
    
    if (rc < 0)
    {
        printk(KERN_INFO "------pmapp_clock_vote error \n");
        return -1;
    }
    
    printk(KERN_INFO "------pmapp_clock_vote OK! \n");
    return 0;
}


/**
 * Modify the low-CLOCK protocol used by the Host via the proc interface.
 * @param file Not used.
 * @param buffer The buffer to read from.
 * @param count The number of bytes to be written.
 * @param data Not used.
 * @return On success, the number of bytes written. On error, -1, and
 * <code>errno</code> is set appropriately.
 */
static int blueclock_write_proc_proto(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char proto;
	
	printk(KERN_INFO "------blueclock_write_proc_proto() is invoked!\n");

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&proto, buffer, 1))
		return -EFAULT;

	if (proto == '0')
	{
	    printk(KERN_INFO "invalid value\n");		
	}
	else
	{
	    /*request blue clock to CTL mode*/
		blueclock_start();
	}
	return count;
}

static int __init blueclock_probe(struct platform_device *pdev)
{
    return 0;
}

static int blueclock_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver blueclock_driver = {
	.probe = blueclock_probe,
	.remove = blueclock_remove,
	.driver = {
		.name = "blueclock",
		.owner = THIS_MODULE,
	},
};
/**
 * Initializes the module.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
static int __init blueclock_init(void)
{
	int retval;
	struct proc_dir_entry *ent;

	BT_INFO("MSM-hw clock Mode Driver Ver %s", VERSION);

	retval = platform_driver_register(&blueclock_driver);
	if (retval)
		return retval;

	bluetooth_dir = proc_mkdir("bluetooth", NULL);
	if (bluetooth_dir == NULL) {
		BT_ERR("Unable to create /proc/bluetooth directory");
		return -ENOMEM;
	}

	clock_dir = proc_mkdir("clock", bluetooth_dir);
	if (clock_dir == NULL) {
		BT_ERR("Unable to create /proc/%s directory", PROC_DIR);
		return -ENOMEM;
	}

	/* read/write proc entries */
	ent = create_proc_entry("proto", 0, clock_dir);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/proto entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}

	ent->write_proc = blueclock_write_proc_proto;

	return 0;

fail:
	remove_proc_entry("proto", clock_dir);
	remove_proc_entry("clock", bluetooth_dir);
	remove_proc_entry("bluetooth", 0);
	return retval;
}

/**
 * Cleans up the module.
 */
static void __exit blueclock_exit(void)
{

	platform_driver_unregister(&blueclock_driver);
	remove_proc_entry("proto", clock_dir);
	remove_proc_entry("clock", bluetooth_dir);
	remove_proc_entry("bluetooth", 0);
}

module_init(blueclock_init);
module_exit(blueclock_exit);

MODULE_DESCRIPTION("Bluetooth Clock Mode Driver ver %s " VERSION);
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif

