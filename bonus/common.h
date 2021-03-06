#include <linux/ioctl.h>

#define KEY_LEN (10)

#undef PDEBUG
#define PDEBUG(fmt, args...) do { if (debug) printk( KERN_WARNING GLBL_NAME ": " fmt, ##args); } while(0);

enum svcmd {
        SVCREATE,
        SVTRUNCATE,
        SVREMOVE,
};

struct svc_ioctl_data {
        int id;
        size_t size;
        char key[KEY_LEN];
        enum svcmd cmd;
};

#define SVC_IOC_MAGIC 'w'
#define SVC_IOCSCTL _IOW(SVC_IOC_MAGIC, 1, struct svc_ioctl_data)
#define SVC_IOC_MAXNR (1)

#define SV_MAX_SIZE (1048576)
#define SV_NR_DEVS  (4)
#define SV_NAME "sv_data"
#define GLBL_NAME "secvault"

/* vim:set ts=8 sw=8: */
