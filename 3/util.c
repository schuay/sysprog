/*********************************************
 * Module: SYSPROG WS 2010 TU Wien
 * Author: Jakob Gruber ( 0203440 )
 * Description: A gzip compressor (fork, pipes, exec)
 * Assignment: 3
 * *******************************************/

#include "util.h"

/*****************************************
 * Name:    ss_perror 
 * Desc:    prints formatted error to stderr
 * Args:    fmt - printf-like format string
 *          ... - printf-like args
 * Returns: void
 * Globals: appname
 ****************************************/
void ss_perror(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    if(appname == NULL) {
        (void)fprintf(stderr, "appname not set!\n");
    } else {
        (void)fprintf(stderr, "%s: ", appname);
    }
    (void)vfprintf(stderr, fmt, ap);
    (void)fprintf(stderr, "\n");
    va_end(ap);
}

/*****************************************
 * Name:    ss_fopen 
 * Desc:    wraps fopen, prints error msg on error
 * Args:    equivalent to fopen(3)
 * Returns: equivalent to fopen(3)
 * Globals: none
 ****************************************/
FILE *ss_fopen(const char *path, const char *mode) {
    FILE *ret = fopen(path, mode);
    if(ret == NULL) {
        ss_perror(strerror(errno));
    }
    return(ret);
}

/*****************************************
 * Name:    ss_fprintf 
 * Desc:    wraps fprintf, prints error msg on error
 * Args:    equivalent to fprintf(3)
 * Returns: equivalent to fprintf(3)
 * Globals: none
 ****************************************/
int ss_fprintf(FILE *stream, const char *format, ...) {
    int ret;
    va_list ap;
    va_start(ap, format);
    ret = vfprintf(stream, format, ap);
    va_end(ap);
    if(ret < 0) ss_perror("vfprintf failed");
    return(ret);
}

/*****************************************
 * Name:    ss_close
 * Desc:    wraps close, prints error msg on error
 * Args:    equivalent to close(2)
 * Returns: equivalent to close(2)
 * Globals: none
 ****************************************/
int ss_close(int fd) {
    int ret = close(fd);
    if(ret == -1) {
        ss_perror(strerror(errno));
    }
    return(ret);
}

/*****************************************
 * Name:    ss_strtol
 * Desc:    wraps strtol, prints error msg on error
 * Args:    nptr, string to be converted to long
 *          result, pointer to target long variable
 * Returns: 0 on success, nonzero on failure
 * Globals: none
 ****************************************/
int ss_strtol(const char *nptr, long int *result) {
    long int res;
    char *endptr;
    int ret = 0;
    errno = 0;

    res = strtol(nptr, &endptr, 10);

    if(errno != 0) {
        ss_perror(strerror(errno));
        ret = -1;
    } else if (*endptr != '\0') {
        ss_perror("string contains nonnumeric characters");
        ret = -1;
    } else {
        *result = res;
    }

    return(ret);
}
