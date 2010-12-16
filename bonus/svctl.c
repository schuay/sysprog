#include <stropts.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include "common.h"

int main(int argc, char **argv) {
    int fd, retval = 0;
    
    fd = open("/dev/sv_ctl", O_RDWR);
    if(fd == -1) {
        perror("open");
        return(-1);
    }
    if(ioctl(fd, SVC_IOCTEST, 0) == -1) {
        perror("ioctl");
    }

    return(retval);
}
