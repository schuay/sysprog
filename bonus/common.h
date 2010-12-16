#include <linux/ioctl.h>

#define KEY_LEN (10)
#define SVC_IOC_MAGIC 'w'
#define SVC_IOCTEST _IO(SVC_IOC_MAGIC, 3)

typedef struct {
        int id;
        size_t size;
        char key[KEY_LEN];
} svc_ioctl_data;


/* vim:set ts=8 sw=8: */
