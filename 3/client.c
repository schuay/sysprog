/*********************************************
 * Module: SYSPROG WS 2010 TU Wien
 * Author: Jakob Gruber ( 0203440 )
 * Description: A Tic Tac Toe server using shared memory
 * Assignment: 3
 * *******************************************/

#include <stdlib.h>

#include "util.h"

/*****************************************
 * Name:    usage
 * Desc:    prints usage to stderr
 * Args:
 * Returns:
 * Globals:
 ****************************************/
void usage(void) {
    (void)fprintf(stderr, "usage: %s\n", appname);
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
    if(argc != 1) {
        usage();
        return(EXIT_FAILURE);
    }

    return(ret);
}
