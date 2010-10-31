/*********************************************
 * Module: SYSPROG WS 2010 TU Wien
 * Author: Jakob Gruber ( 0203440 )
 * Description: A gzip compressor (fork, pipes, exec)
 * Assignment: 2
 * *******************************************/

#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "util.h"

#define NUM_CHILDREN 2
#define BUFFER_SIZE 256

void usage(void) {
    (void)fprintf(stderr, "usage: %s [file]\n", appname);
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
    int ret = 0;

    if(dup2(infd, SS_FD_STDIN) == -1 ||
       dup2(outfd, SS_FD_STDOUT) == -1) {
        ss_perror(strerror(errno));
        ret = 1;
    }
    (void)close(infd);
    (void)close(outfd);
    if(ret == 1) return(1);

    (void)execlp(file, file, args, (char *)NULL);

    /* execlp only returns on error */
    ss_perror(strerror(errno));

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
        outfile = ss_fopen(outfname, "wb");
        if(outfile == NULL) return(1);
    }

    while((ret = read(infd, buf, BUFFER_SIZE)) > 0) {
        if(fwrite(buf, sizeof(char), BUFFER_SIZE, outfile) < BUFFER_SIZE) {
            /* either EOF or error, abort in any case */
            ret = -1;
            break;
        }
    }
    if(fclose(outfile) != 0) {
        ss_perror("fclose failed");
    }
    (void)close(infd);

    if(ret == -1) {
        ss_perror(strerror(errno));
        return(1);
    }

    return(0);
}

int main(int argc, char **argv) {
    int 
        i, 
        ret,                /* stores exit status at several different points */
        main_ret,           /* application exit code */
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
        ss_perror("failed to create pipe: %s", strerror(errno));
        (void)close(p1[SS_PIPE_READ]); (void)close(p1[SS_PIPE_WRITE]);
        return(1);
    }

    /* fork 2 children */
    for(i = 0; i < NUM_CHILDREN; i++) {
        pid[i] = fork();
        /* code executed by parent process */
        if(pid[i] > SS_FORK_CHILD) {
#ifdef DEBUG
            (void)ss_fprintf(stdout, "created child with pid %d\n", pid[i]);
#endif
        /* code executed by child process. exit() MUST be called here */
        } else if(pid[i] == SS_FORK_CHILD) {
            if(i == 0) {
                (void)close(p1[SS_PIPE_WRITE]);
                (void)close(p2[SS_PIPE_READ]);
                ret = main_child1(p1[SS_PIPE_READ], p2[SS_PIPE_WRITE]);
            } else {
                (void)close(p1[SS_PIPE_READ]);
                (void)close(p1[SS_PIPE_WRITE]);
                (void)close(p2[SS_PIPE_WRITE]);
                ret = main_child2(p2[SS_PIPE_READ], (argc < 2 ? NULL : argv[1]));
            }
            exit(ret);
        /* code executed on fork error */
        } else {
            ss_perror("unable to fork");
            (void)close(p1[SS_PIPE_READ]);
            (void)close(p1[SS_PIPE_WRITE]);
            (void)close(p2[SS_PIPE_READ]);
            (void)close(p2[SS_PIPE_WRITE]);
            return(1);
        }
    }

    /* close unused pipe ends */
    (void)close(p1[SS_PIPE_READ]);
    (void)close(p2[SS_PIPE_READ]);
    (void)close(p2[SS_PIPE_WRITE]);

    /* send stdin to 1st pipe */
    while((ret = read(SS_FD_STDIN, buf, BUFFER_SIZE)) > 0) {
        if((ret = write(p1[SS_PIPE_WRITE], buf, ret)) == -1) {
            break;
        }
    }
    if(ret == -1) ss_perror(strerror(errno));

    /* close resources */
    (void)close(p1[SS_PIPE_WRITE]);

    /* wait for child threads to finish */
    main_ret = 0;
    for(i = 0; i < NUM_CHILDREN; i++) {
        ws = waitpid(pid[i], &ret, 0);
        if(WIFEXITED(ret) == 0) {
            /* error returned */
            ss_perror("child %d exited with status %d\n", pid[i], WEXITSTATUS(ret));
            main_ret = 1;
        }
    } 

    return(main_ret);
}
