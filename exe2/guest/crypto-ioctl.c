/*
 * crypto-ioctl.c
 *
 * Implementation of ioctl for 
 * virtio-crypto (guest side) 
 *
 * Stefanos Gerangelos <sgerag@cslab.ece.ntua.gr>
 *                                                                               
 */

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/freezer.h>
#include <linux/list.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/virtio.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/ioctl.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include "crypto.h"
#include "crypto-vq.h"
#include "crypto-chrdev.h"

long crypto_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct crypto_device *crdev;
	struct crypto_data *cr_data;
	ssize_t ret;

	crdev = filp->private_data;

	cr_data = kzalloc(sizeof(crypto_data), GFP_KERNEL);
	if (!cr_data)
		return -ENOMEM;

	cr_data->cmd = cmd;

	switch (cmd) {

        /* get the metadata for every operation 
	 * from userspace and send the buffer 
         * to the host */

	case CIOCGSESSION:
		/* ? */
		/*we need to add no data just start a session*/
		debug("in ciocgsession before send the data");	
		send_buf(crdev,cr_data,sizeof(crypto_data),true);		
			
		if (!device_has_data(crdev)) {
			debug("sleeping in CIOCGSESSION\n");
			if (filp->f_flags & O_NONBLOCK)
				return -EAGAIN;

			/* Go to sleep unti we have data. */
			ret = wait_event_interruptible(crdev->i_wq,
			                               device_has_data(crdev));

			if (ret < 0) 
				goto free_buf;
		}
		printk(KERN_ALERT "woke up in CIOCGSESSION\n");

		if (!device_has_data(crdev))
			return -ENOTTY;

		ret = fill_readbuf(crdev, (char *)cr_data, sizeof(crypto_data));

		/* copy the response to userspace */
		/* ? */

		break;

	case CIOCCRYPT: 
		/* ? */

		if (!device_has_data(crdev)) {
			printk(KERN_WARNING "sleeping in CIOCCRYPTO\n");	
			if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
			
			/* Go to sleep until we have data. */
			ret = wait_event_interruptible(crdev->i_wq,
			device_has_data(crdev));
			
			if (ret < 0)
			goto free_buf;
		}

		ret = fill_readbuf(crdev, (char *)cr_data, sizeof(crypto_data));
		ret = copy_to_user((void __user *)arg, &cr_data->op.crypt, 
		                   sizeof(struct crypt_op));
		if (ret)
			goto free_buf;

		/* copy the response to userspace */
		/* ? */

		break;

	case CIOCFSESSION:

		/* ? */

		if (!device_has_data(crdev)) {
			printk(KERN_WARNING "PORT HAS NOT DATA!!!\n");
			if (filp->f_flags & O_NONBLOCK)
				return -EAGAIN;

			/* Go to sleep until we have data. */
			ret = wait_event_interruptible(crdev->i_wq,
			                               device_has_data(crdev));

			if (ret < 0)
				goto free_buf;
		}

		ret = fill_readbuf(crdev, (char *)cr_data, sizeof(crypto_data));

		/* copy the response to userspace */
		/* ? */

		break;

	default:
		return -EINVAL;
	}

	return 0;

free_buf:
	kfree(cr_data);
	return ret;	
}
