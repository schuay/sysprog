/*********************************************
 * Module: SYSPROG WS 2010 TU Wien
 * Author: Jakob Gruber ( 0203440 )
 * Description: A gzip compressor (fork, pipes, exec)
 * Assignment: 2
 * *******************************************/

#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

/*****************************************
 * Name:
 * Desc:
 * Args:
 * Returns:
 * Globals:
 ****************************************/

#define PIPE_READ 0
#define PIPE_WRITE 1
#define FORK_CHILD 0
#define NUM_CHILDREN 2
#define BUFFER_SIZE 256
#define STDIN 0
#define STDOUT 1
#define STDERR 2

char
    *appname;

void usage(void) {
    fprintf(stderr, "usage: %s [file]\n", appname);
}

/*****************************************
 * Name:    printerror
 * Desc:    prints error to stderr
 * Args:
 * Returns:
 * Globals:
 ****************************************/
void printerror(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "%s: ", appname);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

/*****************************************
 * Name:    main_child1
 * Desc:    main entry point for child 1
 *          compresses data from inpipe with gzip
 *          to outpipe
 * Args:    infd - read pipe file descriptor
 *          outfd - write pipe file descriptor
 * Returns:
 * Globals:
 ****************************************/
int main_child1(int infd, int outfd) {
    const char *file = "gzip";
    const char *args = "-cf";

    dup2(infd, STDIN);
    dup2(outfd, STDOUT);
    close(infd);
    close(outfd);
    execlp(file, file, args, (char *)NULL);

    /* execlp only returns on error */
    printerror(strerror(errno));

    return(1);
}

/*****************************************
 * Name:    main_child2
 * Desc:    main entry point for child 2
 *          reads data from inpipe and
 *          writes it to a file or stdout
 * Args:    inpipe - read pipe file descriptor
 *          outfname - name of output file or NULL for stdout
 * Returns:
 * Globals:
 ****************************************/
int main_child2(int infd, const char *outfname) {
    char buf[BUFFER_SIZE];
    FILE *outfile;
    int ret;

    if(outfname == NULL) {
        outfile = stdout;
    } else {
        outfile = fopen(outfname, "wb");
        if(outfile == NULL) {
            /* handle error */
            return(1);
        }
    }

    while((ret = read(infd, buf, BUFFER_SIZE)) > 0) {
        if(fwrite(buf, sizeof(char), BUFFER_SIZE,  outfile) == EOF) {
            /* handle error */
            return(1);
        }
    }
    if(ret == -1) {
        /* handle error */
        return(1);
    }

    fclose(outfile);
    close(infd);

    return(0);
}

int main(int argc, char **argv) {
    int 
        i, 
        ret,         /* child exit status */
        p1[2],              /* pipe #1 */
        p2[2];              /* pipe #2 */
    pid_t 
        pid[NUM_CHILDREN],  /* children pids */
        ws;                 /* result of waitpid */
    char
        buf[BUFFER_SIZE];

    appname = argv[0];

    /* argument handling */
    if(argc > 2) {
        usage();
        return(1);
    }

    /* create both pipes */
    if(pipe(p1) == -1 || pipe(p2) == -1) {
        printerror("failed to create pipe: %s", strerror(errno));
        close(p1[PIPE_READ]); close(p1[PIPE_WRITE]);
        return(1);
    }

    /* fork 2 children */
    for(i = 0; i < NUM_CHILDREN; i++) {
        pid[i] = fork();
        /* code executed by parent process */
        if(pid[i] > FORK_CHILD) {
#ifdef DEBUG
            printf("created child with pid %d\n", pid[i]);
#endif
        /* code executed by child process. exit() MUST be called here */
        } else if(pid[i] == FORK_CHILD) {
            if(i == 0) {
                close(p1[PIPE_WRITE]);
                close(p2[PIPE_READ]);
                ret = main_child1(p1[PIPE_READ], p2[PIPE_WRITE]);
            } else {
                close(p1[PIPE_READ]);
                close(p1[PIPE_WRITE]);
                close(p2[PIPE_WRITE]);
                ret = main_child2(p2[PIPE_READ], (argc < 2 ? NULL : argv[1]));
            }
            exit(ret);
        /* code executed on fork error */
        } else {
            printerror("unable to fork");
            return(1);
        }
    }

    /* close unused pipe ends */
    close(p1[PIPE_READ]);
    close(p2[PIPE_READ]);
    close(p2[PIPE_WRITE]);

    while((ret = read(STDIN, buf, BUFFER_SIZE)) > 0) {
        write(p1[PIPE_WRITE], buf, ret);
    }
    if(ret == -1) {
        /* handle error */
    }

    /* close resources */
    close(p1[PIPE_WRITE]);

    /* wait for child threads to finish */
    for(i = 0; i < NUM_CHILDREN; i++) {
        ws = waitpid(pid[i], &ret, 0);
        if(WIFEXITED(ret) == 0) {
            /* error returned */
        }
#ifdef DEBUG
        printf("child %d exited with status %d\n", pid[i], WEXITSTATUS(ret));
#endif
    } 

    return(0);
}
