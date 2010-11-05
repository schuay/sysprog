/*********************************************
 * Module: SYSPROG WS 2010 TU Wien
 * Author: Jakob Gruber ( 0203440 )
 * Description: A gzip compressor (fork, pipes, exec)
 * Assignment: 2
 * *******************************************/

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define SS_PIPE_READ 0
#define SS_PIPE_WRITE 1
#define SS_FORK_CHILD 0
#define SS_FD_STDIN 0
#define SS_FD_STDOUT 1
#define SS_FD_STDERR 2

char *appname;

void ss_perror(const char *fmt, ...);
FILE *ss_fopen(const char *path, const char *mode);
int ss_fprintf(FILE *stream, const char *format, ...);
int ss_close(int fd);
