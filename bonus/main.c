#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

#define SVC_NAME "sv_ctl"
#define SVC_NR_DEVS (1)

#undef PDEBUG
#define PDEBUG(fmt, args...) printk( KERN_DEBUG SVC_NAME ": " fmt, ##args)

static int sv_major;
static int sv_minor = 0;

static int __init mod_init(void)
{
    int result;
    dev_t dev = 0;

    result  = alloc_chrdev_region(&dev, sv_minor, SVC_NR_DEVS, SVC_NAME);
    sv_major = MAJOR(dev);

    if(result < 0) {
        printk(KERN_WARNING SVC_NAME ": can't alloc dev nr\n");
        return(result);
    }

    PDEBUG("allocated dev nr %d, %d", sv_major, sv_minor);

    return 0;
}

static void __exit mod_exit(void)
{
    dev_t dev = MKDEV(sv_major, sv_minor);

    unregister_chrdev_region(dev, SVC_NR_DEVS);
    PDEBUG("unregistered dev nr %d, %d", sv_major, sv_minor);
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jakob Gruber");
