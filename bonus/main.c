#include <linux/module.h>
#include <linux/init.h>

static int __init mod_init(void)
{
    return 0;
}

static void __exit mod_exit(void)
{
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jakob Gruber");
