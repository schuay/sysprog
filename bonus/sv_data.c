#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/slab.h>

#include "common.h"

typedef struct {
        int major;
        int minor;
        struct cdev cdev;
        struct semaphore sem;
        char *data;
        size_t size;
        char key[KEY_LEN];
        uid_t uid;
        int ready;
} svd_dev;

static svd_dev devs[SV_NR_DEVS];

int svd_create(int id, uid_t uid, size_t size, const char *key) {
        int retval = 0;
        svd_dev *dev = &devs[id];

        if(size > (size_t)SV_MAX_SIZE) return(-ENOTSUPP);

        if(down_interruptible(&devs[id].sem) != 0) return(-ERESTARTSYS);
        /* already initialized? */
        if(dev->ready) {
                retval = -EEXIST;
                goto exit;
        }
        /* alloc and zero mem */
        dev->data = kmalloc(size, GFP_KERNEL);
        if(dev->data == NULL) {
                retval = -ENOMEM;
                goto exit;
        }
        memset(dev->data, 0, size);
        /* set properties */
        dev->size = size;
        dev->uid = uid;
        strlcpy(dev->key, key, KEY_LEN);
        dev->ready = 1;

exit:
        up(&devs[id].sem);
        return(retval);
}
int svd_truncate(int id, uid_t uid) {
        int retval = 0;
        svd_dev *dev = &devs[id];

        if(down_interruptible(&devs[id].sem) != 0) return(-ERESTARTSYS);
        if(dev->uid != uid) {
                retval = -EPERM;
                goto exit;
        }
        /* zero region */
        memset(dev->data, 0, dev->size);

exit:
        up(&devs[id].sem);
        return(0);
}
int svd_remove(int id, uid_t uid) {
        int retval = 0;
        svd_dev *dev = &devs[id];

        if(down_interruptible(&devs[id].sem) != 0) return(-ERESTARTSYS);
        if(dev->uid != uid) {
                retval = -EPERM;
                goto exit;
        }
        /* free dynamic mem and zero region */
        kfree(dev->data);
        memset(dev, 0, sizeof(svd_dev));

exit:
        up(&devs[id].sem);
        return(0);
}

loff_t svd_llseek(struct file *filp, loff_t off, int whence) {
        return (loff_t)0;
}
ssize_t svd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos) {
        return (ssize_t)0;
}
ssize_t svd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos) {
        return (ssize_t)0;
}
int svd_release(struct inode *inode, struct file *filp) {
        return(0);
}
int svd_open(struct inode *inode, struct file *filp) {
        return(0);
}

static struct file_operations svc_fops = {
    .owner =    THIS_MODULE,
    .llseek =   svd_llseek,
    .read =     svd_read,
    .write =    svd_write,
    .open =     svd_open,
    .release =  svd_release,
};

int __init svd_setup(int id, int major) {
        int err, minor = id + 1;
        dev_t dev = MKDEV(major, minor);

        memset(&devs[id], 0, sizeof(svd_dev));
        init_MUTEX(&devs[id].sem);
        devs[id].major = major;
        devs[id].minor = minor;

        cdev_init(&devs[id].cdev, &svc_fops);
        devs[id].cdev.owner = THIS_MODULE;
        devs[id].cdev.ops = &svc_fops;
        err = cdev_add(&devs[id].cdev, dev, 1);
        if(err) {
                printk(KERN_WARNING SV_NAME "could not add cdevice\n");
                return(-1);
        }
        PDEBUG("added sv_data%d maj %d min %d\n", id, major, minor);
        return(0);
}
void __exit svd_remove_dev(int id) {
        kfree(devs[id].data);
        cdev_del(&devs[id].cdev);
        PDEBUG("removed sv_data%d\n", id);
}
    
/* vim:set ts=8 sw=8: */
