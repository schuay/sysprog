#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/slab.h>

#include "common.h"
#include "sv_data.h"

#define SVC_NAME "sv_ctl"
#define SVC_NR_DEVS (1)

static struct {
        int svc_major;
        int svc_minor;
        struct cdev cdev;
        struct semaphore sem; /* to be removed */
} svc_dev; 

long svc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
        int retval = 0;
        svc_ioctl_data data;

        if(_IOC_TYPE(cmd) != SVC_IOC_MAGIC) return(-ENOTTY);
        if(_IOC_NR(cmd) > SVC_IOC_MAXNR) return(-ENOTTY);

        PDEBUG("processing ioctl with cmd %d\n", cmd);
        switch(cmd) {
                case SVC_IOCSCTL:
                        if(copy_from_user(&data, (const void __user *)arg,
                                                sizeof(svc_ioctl_data)) != 0)
                                return(-EFAULT);
                        break;
                default:
                        return(-ENOTTY);
        }
        PDEBUG("rcvd contents: id: %d size: %zu key %s", data.id, data.size, data.key);

        /* doing some stuff with svc_dev, lock it */
        if(down_interruptible(&svc_dev.sem) != 0)
                return(-ERESTARTSYS);
        /* and unlock */
        up(&svc_dev.sem);


        return(retval);
}

static struct file_operations svc_fops = {
    .owner =            THIS_MODULE,
    .unlocked_ioctl =   svc_ioctl,
};

static void svc_setup_cdev(void)
{
        int err;
        dev_t dev = MKDEV(svc_dev.svc_major, svc_dev.svc_minor);

        cdev_init(&svc_dev.cdev, &svc_fops);
        svc_dev.cdev.owner = THIS_MODULE;
        svc_dev.cdev.ops = &svc_fops;
        err = cdev_add(&svc_dev.cdev, dev, 1);
        if(err) {
                printk(KERN_WARNING SVC_NAME "could not add cdevice\n");
        } else {
                PDEBUG("cdev added successfully\n");
        }
}

static int __init svc_init(void)
{
        int result, i;
        dev_t dev = 0;

        /* allocated dynamic dev nr */
        result  = alloc_chrdev_region(&dev, svc_dev.svc_minor, 
                        SVC_NR_DEVS + SV_NR_DEVS, GLBL_NAME);
        svc_dev.svc_major = MAJOR(dev);

        if(result < 0) {
                printk(KERN_WARNING SVC_NAME ": can't alloc dev nr\n");
                return(result);
        }

        PDEBUG("allocated dev nr %d, %d", svc_dev.svc_major, svc_dev.svc_minor);

        init_MUTEX(&svc_dev.sem);
        PDEBUG("sem initialized\n");

        for(i = 0; i < SV_NR_DEVS; i++)
                if(svd_setup(i, svc_dev.svc_major) != 0)
                        return(-1);
        svc_setup_cdev();

        return(0);
}

static void __exit svc_exit(void)
{
        int i;
        dev_t dev = MKDEV(svc_dev.svc_major, svc_dev.svc_minor);

        for(i = 0; i < SV_NR_DEVS; i++) svd_remove_cdev(i);

        cdev_del(&svc_dev.cdev);
        PDEBUG("deleted cdev\n");

        unregister_chrdev_region(dev, SVC_NR_DEVS + SV_NR_DEVS);
        PDEBUG("unregistered dev nr %d, %d\n", svc_dev.svc_major, svc_dev.svc_minor);
}

module_init(svc_init);
module_exit(svc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jakob Gruber");

/* vim:set ts=8 sw=8: */
