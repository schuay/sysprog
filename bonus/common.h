#include <linux/ioctl.h>

#define KEY_LEN (10)

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
#define SVC_IOCSCTL _IOW(SVC_IOC_MAGIC, 3, svc_ioctl_data)

#define SV_MAX_ID (3)

/* vim:set ts=8 sw=8: */
