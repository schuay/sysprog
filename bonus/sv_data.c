#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/slab.h>

#include "common.h"

struct svd_data {
	char *data;
	size_t len;
	size_t size;
	char key[KEY_LEN];
	uid_t uid;
	int ready;
};

struct svd_dev {
	int major;
	int minor;
	int initialized;
	struct cdev cdev;
	struct semaphore sem;
	struct svd_data contents;
};

static struct svd_dev devs[SV_NR_DEVS];

extern int debug;

/*****************************************
 * Name:    svc_create
 * Desc:    creates a data device
 * Args:    id, id of device
 *          uid, user id
 *          size, size of device
 *          key, encryption key 
 * Returns: 0 on success, nonzero on failure
 * Globals: -
 ****************************************/
int svd_mkdev(int);
int svd_create(int id, uid_t uid, size_t size, const char *key)
{
	int retval = 0;
	struct svd_dev *dev = &devs[id];

	if (size > (size_t)SV_MAX_SIZE)
		return (-ENOTSUPP);

	if (down_interruptible(&dev->sem) != 0)
		return (-ERESTARTSYS);

	/* if device is not yet present, create it */
	if (!dev->initialized) {
		if (svd_mkdev(id) != 0) {
			retval = -ENOMEM;
			goto exit;
		}
	}

	/* already initialized? */
	if (dev->contents.ready) {
		retval = -EEXIST;
		goto exit;
	}
	/* alloc and zero mem */
	dev->contents.data = kmalloc(size, GFP_KERNEL);
	if (dev->contents.data == NULL) {
		retval = -ENOMEM;
		goto exit;
	}
	memset(dev->contents.data, 0, size);
	/* set properties */
	dev->contents.size = size;
	dev->contents.uid = uid;
	strlcpy(dev->contents.key, key, KEY_LEN);
	dev->contents.ready = 1;

	PDEBUG("svd_create completed successfully, uid %d", uid);

exit:
	up(&dev->sem);
	return (retval);
}
/*****************************************
 * Name:    svd_truncate
 * Desc:    truncates a device
 * Args:    id, device id
 *          uid, user id
 * Returns: 0 on success, nonzero on failure
 * Globals: -
 ****************************************/
int svd_truncate(int id, uid_t uid)
{
	int retval = 0;
	struct svd_dev *dev = &devs[id];

	if (down_interruptible(&dev->sem) != 0)
		return (-ERESTARTSYS);
	if (!dev->contents.ready) {
		retval = -ENXIO;
		goto exit;
	}
	if (dev->contents.uid != uid) {
		retval = -EPERM;
		goto exit;
	}
	/* zero region */
	memset(dev->contents.data, 0, dev->contents.size);
	dev->contents.len = 0;

exit:
	up(&dev->sem);
	return (retval);
}
/*****************************************
 * Name:    svd_remove
 * Desc:    removes a device and frees memory
 * Args:    id, device id
 *          uid, user id
 * Returns: 0 on success, nonzero on failure
 * Globals: -
 ****************************************/
int svd_remove(int id, uid_t uid)
{
	int retval = 0;
	struct svd_dev *dev = &devs[id];

	if (down_interruptible(&dev->sem) != 0)
		return (-ERESTARTSYS);
	if (!dev->contents.ready) {
		retval = -ENXIO;
		goto exit;
	}
	if (dev->contents.uid != uid) {
		retval = -EPERM;
		goto exit;
	}
	/* free dynamic mem and zero region */
	kfree(dev->contents.data);
	memset(&dev->contents, 0, sizeof(struct svd_data));

exit:
	up(&dev->sem);
	return (retval);
}

/*****************************************
 * Name:    encrypted_write
 * Desc:    encrypts and writes user input
 * Args:    from, pointer to user input
 *          to, pointer to destination
 *          key, encryption key
 *          count, length of string
 * Returns: 0 on success, nonzero on failure
 * Globals: -
 ****************************************/
static int encrypted_write(const char __user *from, char *to, const char *key, 
		size_t count)
{
	/* en-/decryption functions must be called within a semaphore context */
	int i;

	if (copy_from_user(to, from, count))
		return (-EFAULT);
	for (i = 0; i < count; i++)
		to[i] = to[i] ^ key[i % KEY_LEN];

	return (0);
}
/*****************************************
 * Name:    encrypted_read
 * Desc:    decrypts and writes to user space
 * Args:    from, pointer to kernel space data
 *          to, pointer to user space destination
 *          key, encryption key
 *          count, length of string
 * Returns: 0 on success, nonzero on failure
 * Globals: -
 ****************************************/
static int decrypted_read(const char *from, char __user *to, const char *key,
		size_t count)
{
	/* en-/decryption functions must be called within a semaphore context */
	char *buf;
	int i, retval = 0;

	buf = kmalloc(count, GFP_KERNEL);
	memset(buf, 0, count);
	for (i = 0; i < count; i++)
		buf[i] = from[i] ^ key[i % KEY_LEN];
	if (copy_to_user(to, buf, count) != 0)
		retval = -EFAULT;
	kfree(buf);

	return (retval);
}

/*****************************************
 * Name:    svd_llseek
 * Desc:    implements seek operation
 * Args:    as specified in kernel headers
 * Returns: as specified in kernel headers
 * Globals: -
 ****************************************/
loff_t svd_llseek(struct file *filp, loff_t off, int whence)
{
	struct svd_dev *dev = filp->private_data;
	loff_t newpos;

	switch(whence) {
	  case 0: /* SEEK_SET */
		newpos = off;
		break;

	  case 1: /* SEEK_CUR */
		newpos = filp->f_pos + off;
		break;

	  case 2: /* SEEK_END */
		newpos = dev->contents.size + off;
		break;

	  default: /* can't happen */
		return -EINVAL;
	}
	if (newpos < 0) return -EINVAL;
	filp->f_pos = newpos;
	return newpos;
}
/*****************************************
 * Name:    svd_write
 * Desc:    implements write operation
 * Args:    as specified in kernel headers
 * Returns: as specified in kernel headers
 * Globals: -
 ****************************************/
static ssize_t svd_write(struct file *filp, const char __user *buf, size_t count,
		loff_t *f_pos)
{
	struct svd_dev *dev = filp->private_data;
	ssize_t retval = -ENOMEM;

	if (down_interruptible(&dev->sem))
		return (-ERESTARTSYS);

	/* range valid? also fault on writes exceeding defined device
	 * size */
	if ((long)*f_pos >= dev->contents.size)
		goto exit;
	if ((long)*f_pos + count >= dev->contents.size)
		goto exit;

	/* copy data to kernel space */
	if (encrypted_write(buf, dev->contents.data + *f_pos, dev->contents.key, 
				count)) {
		retval = -EFAULT;
		goto exit;
	}

	dev->contents.len = count;
	*f_pos += count;
	retval = count;

	PDEBUG("wrote %zu successfully, current contents: %s\n", count, dev->contents.data);

exit:
	up(&dev->sem);
	return (retval);
}
/*****************************************
 * Name:    svd_read
 * Desc:    implements read operation
 * Args:    as specified in kernel headers
 * Returns: as specified in kernel headers
 * Globals: -
 ****************************************/
static ssize_t svd_read(struct file *filp, char __user *buf, size_t count,
		loff_t *f_pos)
{
	struct svd_dev *dev = filp->private_data;
	ssize_t retval = 0;

	if (down_interruptible(&dev->sem))
		return (-ERESTARTSYS);

	PDEBUG("read called with fpos %ld, count %zu, current size %zu",
			(long)*f_pos, count, dev->contents.size);

	/* range valid? */
	if ((long)*f_pos >= dev->contents.len)
		goto exit;

	if ((long)*f_pos + count >= dev->contents.len)
		count = dev->contents.len - (long)*f_pos;

	PDEBUG("calling dec_read with fpos %ld, count %zu",
			(long)*f_pos, count);

	/* copy data to user space */
	if (decrypted_read(dev->contents.data + *f_pos, buf, dev->contents.key,
				count)) {
		retval = -EFAULT;
		goto exit;
	}

	*f_pos += count;
	retval = count;

exit:
	up(&dev->sem);
	return (retval);
}
/*****************************************
 * Name:    svd_release
 * Desc:    implements release operation
 * Args:    as specified in kernel headers
 * Returns: as specified in kernel headers
 * Globals: -
 ****************************************/
static int svd_release(struct inode *inode, struct file *filp)
{
	return (0);
}
/*****************************************
 * Name:    svd_open
 * Desc:    implements open operation
 * Args:    as specified in kernel headers
 * Returns: as specified in kernel headers
 * Globals: -
 ****************************************/
static int svd_open(struct inode *inode, struct file *filp)
{
	int retval = 0;
	struct svd_dev *dev;
	dev = container_of(inode->i_cdev, struct svd_dev, cdev);
	filp->private_data = dev;

	if (down_interruptible(&dev->sem))
		return (-ERESTARTSYS);

	/* check if device is ready to use, */
	if (!dev->contents.ready)
		retval = -ENXIO;

	/* and if the current user has access */
	else if (dev->contents.uid != current_uid())
		retval = -EPERM;

	/* now trim to 0 the length of the device if open was write-only */
	else if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
		memset(dev->contents.data, 0, dev->contents.size);

	up(&dev->sem);

	return (retval);
}

static struct file_operations svc_fops = {
    .owner =    THIS_MODULE,
    .llseek =   svd_llseek,
    .read =     svd_read,
    .write =    svd_write,
    .open =     svd_open,
    .release =  svd_release,
};

/*****************************************
 * Name:    svd_setup
 * Desc:    initial device setup
 * Args:    id, dev id
 *          major, major dev nr
 * Returns: 0 on success, nonzero on failure
 * Globals: -
 ****************************************/
int __init svd_setup(int id, int major)
{
	int minor = id + 1;

	memset(&devs[id], 0, sizeof(struct svd_dev));
	sema_init(&devs[id].sem, 1);
	devs[id].major = major;
	devs[id].minor = minor;

	return (0);
}
/*****************************************
 * Name:    svd_mkdev
 * Desc:    creates actual character device
 * Args:    id, device id
 * Returns: 0 on success, nonzero on failure
 * Globals: -
 ****************************************/
int svd_mkdev(int id)
{
	/* must be called within semaphore protected context */

	int err = 0;
	struct svd_dev *dev = &devs[id];
	int major, minor;

	major = dev->major;
	minor = dev->minor;

	cdev_init(&dev->cdev, &svc_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &svc_fops;
	err = cdev_add(&dev->cdev, MKDEV(major, minor), 1);
	dev->initialized = 1;

	if (err) {
		printk(KERN_WARNING SV_NAME "could not add cdevice\n");
		return (-1);
	}
	PDEBUG("added sv_data%d maj %d min %d\n", id, major, minor);

	return (0);
}
/*****************************************
 * Name:    svd_remove_devv
 * Desc:    removes char device, frees all used resources
 * Args:    id, device id
 * Returns: -
 * Globals: -
 ****************************************/
void __exit svd_remove_dev(int id)
{
	if (devs[id].initialized) {
		kfree(devs[id].contents.data);
		cdev_del(&devs[id].cdev);
		PDEBUG("removed sv_data%d\n", id);
	}
}
