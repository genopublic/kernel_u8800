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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/diagchar.h>
#include <linux/sched.h>
#include <mach/usbdiag.h>
#include <asm/current.h>
#include "phudiagchar.h"
#include <linux/timer.h>

MODULE_DESCRIPTION("PHU Diag Char Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");

struct phudiag_dev *phudriver;

static int phudiag_open(struct inode *inode, struct file *file)
{
	#ifdef PHUDIAG_DEBUG
	printk("phudiagchar : Enter phudiag_open(). \n");
	#endif
	
	if (phudriver) {

		//if (phudriver->ref_count == 0 && phudriver->count == 0 )
		/*
		if(phudriver->ref_count>=1){
			//return -EFAULT;
			printk("phudiagchar : Multi-open : phudriver->ref_count = %d!. \n",phudriver->ref_count);
			return 0;
		}
		else{
			printk("phudiagchar : open ok!. \n");
			phudriver->ref_count++;
			phudriver->opened = 1;
			phudriver->read_buf_busy = 0;
			return 0;
		}
		*/
		#ifdef PHUDIAG_DEBUG
		printk("phudiagchar : open ok!. \n");
		#endif
		
		phudriver->ref_count++;
		phudriver->opened = 1;
		phudriver->in_busy = 0;
		return 0;
	}
	printk("phudiagchar : open failed!. \n");
	
	return -ENOMEM;
}

static int phudiag_close(struct inode *inode, struct file *file)
{
	#ifdef PHUDIAG_DEBUG
	printk("phudiagchar : Enter phudiag_close(). \n");
	#endif
	if (phudriver) {
		if(phudriver->ref_count>=1){
			phudriver->ref_count--;
			phudriver->opened = 0;
			phudriver->in_busy = 1;
		}
		else{
			phudriver->ref_count=0;
		}
		#ifdef PHUDIAG_DEBUG
		printk("phudiag_close() : phudriver->ref_count=%d. \n",phudriver->ref_count);
		#endif
		//if(0==phudriver->ref_count){
		//	phudiagmem_free(phudriver);
		//	}
	}

	return 0;
	
}


static int phudiag_ioctl(struct inode *inode, struct file *filp,
			   unsigned int iocmd, unsigned long ioarg)
{
	int len = 0;

	#ifdef PHUDIAG_DEBUG
	printk("phudiagchar : Enter phudiag_ioctl(). \n");
	#endif
	
	switch (iocmd) {
	//case FIONREAD:
	case 0x541B:
		if(phudriver->in_buf_busy)
		{
			return 0;
		}
		
		phudriver->in_buf_busy = 1;
		len = phudiagfwd_ring_buf_get_data_length(phudriver->in_buf);
		phudriver->in_buf_busy = 0;
		
		#ifdef PHUDIAG_DEBUG
		printk("phudiag_ioctl() : len=%d \n",len);
		#endif
		
		if(len >= 0)
		{
			return put_user(len, (int __user *)ioarg);	
		}
		else
		{
			return -EINVAL;
		}
	default:
		printk("phudiag_ioctl() : return -EINVAL. \n");
		return -EINVAL;
	}
}

static int phudiag_read(struct file *file, char __user *buf, size_t count,
			  loff_t *ppos)
{
	int ret = -1;
	
	#ifdef PHUDIAG_DEBUG
	printk("phudiagchar : Enter phudiag_read(). \n");
	#endif
	
	if(phudriver->in_buf_busy)
	{
		return 0;
	}
	phudriver->in_buf_busy = 1;
	ret= phudiagfwd_ring_buf_user_get_data(phudriver->in_buf,buf,count);
	phudriver->in_buf_busy = 0;
	
	#ifdef PHUDIAG_DEBUG	
	printk("phudiagchar : phudiag_read: phudiagfwd_buf_get_data read data length = %d \n",ret);
	#endif
	
	return ret;
}

static int phudiag_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	int ret = -1;

	#ifdef PHUDIAG_DEBUG
	printk("phudiagchar : Enter phudiag_write(). count=%d\n",count);
	#endif


	if(count > PHU_DIAG_SEND_PACKET_MAX_SIZE)
	{
		printk("phudiagchar : phudiag_write  data is too large! \n");
		return -1;
	}

	ret = phudiagfwd_ring_buf2_user_set_data(phudriver->out_buf ,buf,count);
	schedule_work(&(phudriver->phudiag_write_work));
	
	#ifdef PHUDIAG_DEBUG
	printk("phudiagchar : exit phudiag_write(). \n");
	#endif
	
	return ret;
}

static const struct file_operations phudiagfops = {
	.owner = THIS_MODULE,
	.read = phudiag_read,
	.write = phudiag_write,
	.ioctl = phudiag_ioctl,
	.open = phudiag_open,
	.release = phudiag_close
};

static int phudiag_setup_cdev(dev_t devno)
{

	int err;
	
	#ifdef PHUDIAG_DEBUG
	printk("phudiagchar : Enter phudiag_setup_cdev(). \n");
	#endif

	cdev_init(phudriver->cdev, &phudiagfops);

	phudriver->cdev->owner = THIS_MODULE;
	phudriver->cdev->ops = &phudiagfops;

	err = cdev_add(phudriver->cdev, devno, 1);

	if (err) {
		printk("phu diagchar cdev registration failed !\n\n");
		return -1;
	}

	phudriver->phudiagchar_class = class_create(THIS_MODULE, "phudiag");

	if (IS_ERR(phudriver->phudiagchar_class)) {
		printk(KERN_ERR "phudiagchar : Error creating phu diagchar class.\n");
		return -1;
	}

	device_create(phudriver->phudiagchar_class, NULL, devno,
				  (void *)phudriver, "phudiag");

	return 0;

}

static int phudiag_cleanup(void)
{
	#ifdef PHUDIAG_DEBUG
	printk("phudiagchar : Enter phudiag_cleanup(). \n");
	#endif
	
	if (phudriver) {
		if(phudriver->in_buf)
		{
			phudiag_ring_buf_free(phudriver->in_buf);
		}

		if(phudriver->out_buf)
		{
			phudiag_ring_buf_free(phudriver->out_buf);
		}
				
	
		if(phudriver->smd_buf)
		{
			kfree(phudriver->smd_buf);
		}
		
		if (phudriver->cdev) {
			/* TODO - Check if device exists before deleting */
			device_destroy(phudriver->phudiagchar_class,
				       MKDEV(phudriver->major,
					     phudriver->minor_start));
			cdev_del(phudriver->cdev);
		}
		if (!IS_ERR(phudriver->phudiagchar_class))
			class_destroy(phudriver->phudiagchar_class);
		kfree(phudriver);
	}
	return 0;
}


//static int __init phudiag_init(void)
int phudiag_init(void)
{
	dev_t dev;
	int error;
	
	#ifdef PHUDIAG_DEBUG
	printk("phudiagchar : Entering phudiag_init...\n");
	#endif
	
	phudriver = kzalloc(sizeof(struct phudiag_dev) + 8, GFP_KERNEL);

	if (phudriver) {
	       phudriver->used = 0;

	       phudriver->in_busy = 0;
	       #ifdef PHUDIAG_DEBUG
		printk("phudiag char initializing ..\n");
		#endif
		phudriver->num = 1;
		phudriver->name = ((void *)phudriver) + sizeof(struct phudiag_dev);
		strlcpy(phudriver->name, "phudiag", 7);

		phudriver->in_buf = phudiag_ring_buf_malloc(PHU_DIAG_IN_BUF_SIZE);
		if(!phudriver->in_buf)
		{
			goto fail;
		}	

		phudriver->out_buf = phudiag_ring_buf_malloc(PHU_DIAG_OUT_BUF_SIZE);
		if(!phudriver->out_buf)
		{
			goto fail;
		}

		
				
		phudriver->smd_buf = kzalloc(PHU_DIAG_SMD_BUF_MAX, GFP_KERNEL);
		if(!phudriver->smd_buf)
		{
			printk("phudiagchar :phudriver->read_buf  kzalloc fail!\n");
			goto fail;
		}

		INIT_WORK(&(phudriver->phudiag_write_work), phudiagfwd_write_to_smd_work_fn);

		/* Get major number from kernel and initialize */
		error = alloc_chrdev_region(&dev, phudriver->minor_start,
					    phudriver->num, phudriver->name);
		if (!error) {
			phudriver->major = MAJOR(dev);
			phudriver->minor_start = MINOR(dev);
		} else {
			printk("phudiagchar : Major number not allocated \n");
			goto fail;
		}
		phudriver->cdev = cdev_alloc();
		error = phudiag_setup_cdev(dev);
		if (error)
			goto fail;
	} else {
		printk("phudiagchar : kzalloc failed\n");
		goto fail;
	}
	
	#ifdef PHUDIAG_DEBUG
	printk("phudiagchar : phudiag initialized\n");
	#endif
	return 0;

fail:
	phudiag_cleanup();
	return -1;

}

//static void __exit phudiag_exit(void)
void phudiag_exit(void)
{
	#ifdef PHUDIAG_DEBUG
	printk("phudiagchar : PHU diagchar exiting ..\n");
	#endif
	phudiag_cleanup();
	
	#ifdef PHUDIAG_DEBUG
	printk("phudiagchar : done PHU diagchar exit\n");
	#endif
}

//module_init(phudiag_init);
//module_exit(phudiag_exit);
