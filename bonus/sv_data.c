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
        sv_data data;
} svd_dev;

static svd_dev devs[SV_NR_DEVS];

static struct file_operations svc_fops = {
    .owner =            THIS_MODULE,
};
int svd_setup(int id, int major) {
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
void svd_remove_cdev(int id) {
        cdev_del(&devs[id].cdev);
        PDEBUG("removed sv_data%d\n", id);
}
    
/* vim:set ts=8 sw=8: */
