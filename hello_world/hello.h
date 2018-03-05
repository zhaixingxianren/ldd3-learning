#ifndef _HELLO_H
#define  _HELLO_H

#ifndef HELLO_MAJOR
#define HELLO_MAJOR 0   /* dynamic major by default */
#endif

#ifndef HELLO_NR_DEVS
#define HELLO_NR_DEVS 4    /* hello0 through hello3 */
#endif

/*
 * The bare device is a variable-length region of memory.
 * Use a linked list of indirect blocks.
 *
 * "hello_dev->data" points to an array of pointers, each
 * pointer refers to a memory area of HELLO_MAXSIZE bytes.
 */
#ifndef HELLO_MAXSIZE
#define HELLO_MAXSIZE 4000
#endif

struct hello_dev {
	void *data;  /* Pointer to data */
	unsigned long size;       /* amount of data stored here */
	struct semaphore sem;     /* mutual exclusion semaphore     */
	struct cdev cdev;	  /* Char device structure		*/
};

/*
 * Prototypes for shared functions
 */


ssize_t hello_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos);
ssize_t hello_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos);
loff_t  hello_llseek(struct file *filp, loff_t off, int whence);
long     hello_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

#endif //_HELLO_H
