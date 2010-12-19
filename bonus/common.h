#include <linux/ioctl.h>

#define KEY_LEN (10)

#undef PDEBUG
#define PDEBUG(fmt, args...) printk( KERN_DEBUG GLBL_NAME ": " fmt, ##args)

enum svcmd {
        SVCREATE,
        SVTRUNCATE,
        SVREMOVE,
};

typedef struct {
        int id;
        size_t size;
        char key[KEY_LEN];
        enum svcmd cmd;
} svc_ioctl_data;

#define SVC_IOC_MAGIC 'w'
#define SVC_IOCSCTL _IOW(SVC_IOC_MAGIC, 1, svc_ioctl_data)
#define SVC_IOC_MAXNR (1)

#define SV_NR_DEVS  (4)
#define SV_NAME "sv_data"
#define GLBL_NAME "secvault"

/* vim:set ts=8 sw=8: */
