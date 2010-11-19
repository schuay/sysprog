/*********************************************
 * Module: SYSPROG WS 2010 TU Wien
 * Author: Jakob Gruber ( 0203440 )
 * Description: A Tic Tac Toe server using shared memory
 * Assignment: 3
 * *******************************************/

#include <stdlib.h>
#include <sys/wait.h>

#include "util.h"
#include "common.h"

/* GLOBALS */

static int cheatchecks = 1;

/*****************************************
 * Name:    usage
 * Desc:    prints usage to stderr
 * Args:
 * Returns:
 * Globals:
 ****************************************/
void usage(void) {
    (void)fprintf(stderr, "usage: %s [-c]\n", appname);
}

int parseargs(int argc, char **argv) {
    int opt;
    int ret = 0;

    while((opt = getopt(argc, argv, "c")) != -1) {
        switch(opt) {
        case 'c':
            cheatchecks++;
            break;
        default:
            ret = 1;
        }
    }

    if(cheatchecks > 1) {
        ret = 1;
    }

    return(ret);
}

/*****************************************
 * Name:    main
 * Desc:    main entry point.
 * Args:    argc - argument count
 *          argv - pointer to argument list
 * Returns: 0 on success, 1 on error
 * Globals: appname
 ****************************************/
int main(int argc, char **argv) {
    int ret = EXIT_SUCCESS;

    appname = argv[0];

    /* argument handling */
    if(argc > 2 || parseargs(argc, argv) != 0) {
        usage();
        return(EXIT_FAILURE);
    }

    return(ret);
}
