#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>

#include "common.h"

#define SVC_NAME "sv_ctl"
#define SVC_NR_DEVS (1)

#undef PDEBUG
#define PDEBUG(fmt, args...) printk( KERN_DEBUG SVC_NAME ": " fmt, ##args)

static struct {
        int svc_major;
        int svc_minor;
        struct cdev cdev;
        struct semaphore sem;
} svc_dev; 

long svc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
        int retval = 0;

        PDEBUG("processing ioctl with cmd %d\n", cmd);

        return(retval);
}

struct file_operations svc_fops = {
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
        int result;
        dev_t dev = 0;

        /* allocated dynamic dev nr */
        result  = alloc_chrdev_region(&dev, svc_dev.svc_minor, SVC_NR_DEVS, SVC_NAME);
        svc_dev.svc_major = MAJOR(dev);

        if(result < 0) {
                printk(KERN_WARNING SVC_NAME ": can't alloc dev nr\n");
                return(result);
        }

        PDEBUG("allocated dev nr %d, %d", svc_dev.svc_major, svc_dev.svc_minor);

        svc_setup_cdev();

        return(0);
}

static void __exit svc_exit(void)
{
        dev_t dev = MKDEV(svc_dev.svc_major, svc_dev.svc_minor);

        cdev_del(&svc_dev.cdev);
        PDEBUG("deleted cdev\n");

        unregister_chrdev_region(dev, SVC_NR_DEVS);
        PDEBUG("unregistered dev nr %d, %d\n", svc_dev.svc_major, svc_dev.svc_minor);
}

module_init(svc_init);
module_exit(svc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jakob Gruber");

/* vim:set ts=8 sw=8: */
