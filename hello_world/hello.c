#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <asm/uaccess.h>	/* copy_*_user */

#include "hello.h"

MODULE_LICENSE("Dual BSD/GPL");

int hello_major =   HELLO_MAJOR;
int hello_minor =   0;
int hello_nr_devs = HELLO_NR_DEVS;	/* number of bare hello devices */


MODULE_LICENSE("Dual BSD/GPL");

struct hello_dev *hello_devices;	/* allocated in hello_init_module */

/*
 * Open and close
 */

int hello_open(struct inode *inode, struct file *filp)
{
	struct hello_dev *dev; /* device information */

	dev = container_of(inode->i_cdev, struct hello_dev, cdev);
	filp->private_data = dev; /* for other methods */

	/* now trim to 0 the length of the device if open was write-only */
	if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
        /*
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
		up(&dev->sem); */
	}
	return 0;          /* success */
}

int hello_release(struct inode *inode, struct file *filp)
{
	return 0;
}
/*
 * Data management: read and write
 */

ssize_t hello_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	struct hello_dev *dev = filp->private_data; 
	ssize_t retval = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	if (*f_pos >= HELLO_MAXSIZE)
		goto out;
	if (*f_pos + count > HELLO_MAXSIZE)
		count = HELLO_MAXSIZE - *f_pos;


	/* read only up to the end of this quantum */
	if (copy_to_user(buf,dev->data+*f_pos, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

  out:
	up(&dev->sem);
	return retval;
}

ssize_t hello_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	struct hello_dev *dev = filp->private_data;
	ssize_t retval = -ENOMEM; /* value used in "goto out" statements */

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	/* write only up to the end of this quantum */
	if (count + dev->size > HELLO_MAXSIZE)
		count = HELLO_MAXSIZE - dev->size;

	if (copy_from_user(dev->data+*f_pos, buf, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
    dev->size += count;
	retval = count;
    printk(KERN_NOTICE "[hello] f_pos:%lld, dev->size:%ld", *f_pos, dev->size);

  out:
	up(&dev->sem);
	return retval;
}

/*
 * The "extended" operations -- only seek
 */

loff_t hello_llseek(struct file *filp, loff_t off, int whence)
{
	struct hello_dev *dev = filp->private_data;
	loff_t newpos;

	switch(whence) {
	  case 0: /* SEEK_SET */
		newpos = off;
		break;

	  case 1: /* SEEK_CUR */
		newpos = filp->f_pos + off;
		break;

	  case 2: /* SEEK_END */
		newpos = dev->size + off;
		break;

	  default: /* can't happen */
		return -EINVAL;
	}
	if (newpos < 0) return -EINVAL;
	filp->f_pos = newpos;
	return newpos;
}


/*
 * Create a set of file operations for our proc file.
 */
struct file_operations hello_fops = {
	.owner =    THIS_MODULE,
	.llseek =   hello_llseek,
	.read =     hello_read,
	.write =    hello_write,
	.open =     hello_open,
	.release =  hello_release,
};


/*
 * Actually create (and remove) the /proc file(s).
 
static void hello_create_proc(void)
{
	proc_create("hellomem", 0, NULL, &hellomem_proc_fops);
	proc_create("helloseq", 0, NULL, &hello_proc_ops);
}

static void hello_remove_proc(void)
{
	remove_proc_entry("hellomem", NULL );
	remove_proc_entry("helloseq", NULL);
}
*/
/*
 * Set up the char_dev structure for this device.
 */
static void hello_setup_cdev(struct hello_dev *dev, int index)
{
	int err, devno = MKDEV(hello_major, hello_minor + index);
    
	cdev_init(&dev->cdev, &hello_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &hello_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding hello%d", err, index);
}

void hello_exit(void)
{
	int i;
	dev_t devno = MKDEV(hello_major, hello_minor);

	/* Get rid of our char dev entries */
	if (hello_devices) {
		for (i = 0; i < hello_nr_devs; i++) {
            if(hello_devices[i].data)
                kfree(hello_devices[i].data);
			cdev_del(&hello_devices[i].cdev);
		}
		kfree(hello_devices);
	}

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, hello_nr_devs);
	printk(KERN_ALERT"Goodbye,cruel world\n");
}

static int hello_init(void)
{
	int result, i;
	dev_t dev = 0;

	printk(KERN_ALERT"Hello world ! \n");
/*
 * Get a range of minor numbers to work with, asking for a dynamic
 * major unless directed otherwise at load time.
 */
	if (hello_major) {
		dev = MKDEV(hello_major, hello_minor);
		result = register_chrdev_region(dev, hello_nr_devs, "hello");
	} else {
		result = alloc_chrdev_region(&dev, hello_minor, hello_nr_devs,
				"hello");
		hello_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "hello: can't get major %d\n", hello_major);
		return result;
	}

    /* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	hello_devices = kmalloc(hello_nr_devs * sizeof(struct hello_dev), GFP_KERNEL);
	if (!hello_devices) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(hello_devices, 0, hello_nr_devs * sizeof(struct hello_dev));

    /* Initialize each device. */
	for (i = 0; i < hello_nr_devs; i++) {
		hello_devices[i].data = kmalloc(HELLO_MAXSIZE ,GFP_KERNEL);
		if(!hello_devices[i].data){
            result = -ENOMEM;
            goto fail;
        }
        memset(hello_devices[i].data,0,sizeof(HELLO_MAXSIZE));
        hello_devices[i].size = 0;
		sema_init(&hello_devices[i].sem, 1);
		hello_setup_cdev(&hello_devices[i], i);
	}

	return 0; /* succeed */

  fail:
	hello_exit();
	return result;
}

module_init(hello_init);
module_exit(hello_exit);
