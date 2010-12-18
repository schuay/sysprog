#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stropts.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "util.h"

/* globals */
svc_ioctl_data data;

/*****************************************
 * Name:    usage
 * Desc:    prints usage to stderr
 * Args:
 * Returns:
 * Globals:
 ****************************************/
void usage(void) {
    (void)fprintf(stderr, "usage: %s [-c size|-e|-d] <secvault id>\n", appname);
}
int parseargs(int argc, char **argv) {
    int opt, cmdnum = 0;
    long int strtolres;

    while((opt = getopt(argc, argv, "c:ed")) > -1) {
        switch(opt) {
            case 'c':
                if(optarg == NULL || ss_strtol(optarg, &strtolres) != 0) {
                    ss_perror("option %c requires a numeric argument\n", opt);
                    return(-1);
                }
                data.size = (size_t)strtolres;
                data.cmd = SVCREATE;
                cmdnum++;
                break;
            case 'e':
                data.cmd = SVTRUNCATE;
                cmdnum++;
                break;
            case 'd':
                data.cmd = SVREMOVE;
                cmdnum++;
                break;
            case 'h':
            case '?':
                usage();
                return(-1);
            default:
                assert(0);
        }
    }

    if(cmdnum != 1) {
        usage();
        return(-1);
    }

    if(optind != argc - 1 ||                        /* more than 1 extra arg */
       ss_strtol(argv[optind], &strtolres) != 0 ||  /* or arg not a number */
       strtolres < 0 || strtolres >= SV_NR_DEVS) {  /* or arg not in range */
        usage();
        return(-1);
    } else {
        data.id = (int)strtolres;
    }

    return(0);
}

int main(int argc, char **argv) {
    int fd, retval = EXIT_SUCCESS;

    appname = argv[0];

    memset(&data, 0, sizeof(svc_ioctl_data));
    if(parseargs(argc, argv) != 0) {
        return(EXIT_FAILURE);
    }
    
    fd = open("/dev/sv_ctl", O_RDWR);
    if(fd == -1) {
        perror("open");
        return(EXIT_FAILURE);
    }
    if(ioctl(fd, SVC_IOCSCTL, &data) == -1) {
        perror("ioctl");
        retval = EXIT_FAILURE;
    }

    if(close(fd) == -1) {
        perror("close");
        retval = EXIT_FAILURE;
    }

    return(retval);
}
/* vim:set ts=4 sw=4 et: */
