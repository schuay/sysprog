/*********************************************
 * Module: SYSPROG WS 2010 TU Wien
 * Author: Jakob Gruber ( 0203440 )
 * Description: A Tic Tac Toe server using shared memory
 * Assignment: 3
 * *******************************************/

#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sem182.h>
#include <signal.h>

#include "util.h"
#include "common.h"

#define BUFSIZE 256
#define CMDSHOW ('s')
#define CMDHELP ('h')
#define CMDPLACE ('p')
#define CMDCHEAT ('c')
#define CMDNEW ('n')
#define CMDQUIT ('q')

enum inputcmds {
    INP_SHOW,
    INP_HELP,
    INP_PLACE,
    INP_CHEAT,
    INP_NEW,
    INP_QUIT
};

typedef struct {
    enum inputcmds cmd;
    int coord1[2];
    int coord2[2];
} input;

static volatile int quit = 0;
    
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
void termhandler(int sig) {
    (void)printf("Exiting...\n");
    quit = 1;
}
int attachsighandlers(void) {
    int sigs[] = {SIGINT, SIGQUIT, SIGTERM};
    int sigcount =  sizeof(sigs) / sizeof(sigs[0]);
    int i;
    for(i = 0; i < sigcount; i++) {
        if(signal(sigs[i], termhandler) == SIG_ERR) {
            perror("signal");
            return(-1);
        }
    }
    return(0);
}
int grabsem(int id) {
    int sem, key;
    /* get semaphores */
    if((key = ftok(SEMKEYPATH, id)) == -1) {
        perror("ftok");
        return(-1);
    }
    if((sem = semgrab(key)) == -1) {
        ss_perror("semgrab failed");
    }
    return(sem);
}
int grabshm() {
    int shm, key;
    /* attach to shared mem */
    if((key = ftok(SHMKEYPATH, SHMKEYID)) == -1) {
        perror("ftok");
        return(-1);
    }
    if((shm = shmget(key, sizeof(tttdata), 0644)) == -1) {
        perror("shmget");
        return(-1);
    }
    return(shm);
}
void printboard(tttdata *data) {
    int x, y;
    char c;
    for(y = 0; y < SIDE_LEN; y++) {
        for(x = 0; x < SIDE_LEN; x++) {
            switch(data->field[y * SIDE_LEN + x]) {
                case STAT_CLEAR:
                    c = '.';
                    break;
                case STAT_P1_X:
                    c = 'X';
                    break;
                case STAT_AI_O:
                    c = 'O';
                    break;
                default:
                    assert(0);
            }
            (void)printf("%c", c);
        }
        (void)printf("\n");
    }
}
void printcommands(void) {
    (void)printf("Commands:\n");
    (void)printf("s: show board\n");
    (void)printf("p <xy>: mark board at coordinate xy\n");
    (void)printf("c <xy> <xy>: cheat mode, mark board at two points\n");
    (void)printf("n: new game\n");
    (void)printf("q: quit\n");
}

#define MAXCMDNUM (5)
int parseinput(char *buf, input *inp) {
    /* split input into tokens */
    const char *delim = " \n";
    char *saveptr;
    char *tok[MAXCMDNUM];
    int i;
    int ret = 0;
    char cmd;
    int invcmd = 0;
    int invarg = 0;
    long int convres;
    const char *errinv = "Invalid command";
    const char *errarg = "Invalid argument";

    memset(inp, 0, sizeof(input));

    tok[0] = strtok_r(buf, delim, &saveptr);
    for(i = 1; i < MAXCMDNUM; i++) {
        tok[i] = strtok_r(NULL, delim, &saveptr);
    }

    /* validate command */
    if(tok[0] == NULL || strlen(tok[0]) != 1) {
        invcmd++;
    } else {
        cmd = *(tok[0]);
        if(cmd == CMDHELP) {
            inp->cmd = INP_HELP;
            if(tok[1] != NULL) {
                invarg++;
            }
        } else if(cmd == CMDSHOW) {
            inp->cmd = INP_SHOW;
            if(tok[1] != NULL) {
                invarg++;
            }
        } else if (cmd == CMDPLACE) {
            inp->cmd = INP_PLACE;
            if(tok[2] != NULL || tok[1] == NULL || strlen(tok[1]) != 2) {
                invarg++;
            } else if(ss_strtol(tok[1], &convres) != 0) {
                invarg++;
            } else {
                inp->coord1[0] = convres / 10;
                inp->coord1[1] = convres % 10;
            }
        } else if (cmd == CMDCHEAT) {
            inp->cmd = INP_CHEAT;
            if(tok[3] != NULL || tok[1] == NULL || strlen(tok[1]) != 2 ||
                                 tok[2] == NULL || strlen(tok[2]) != 2) {
                invarg++;
            } else {
                if(ss_strtol(tok[1], &convres) != 0) {
                    invarg++;
                } else {
                    inp->coord1[0] = convres / 10;
                    inp->coord1[1] = convres % 10;
                }
                if(ss_strtol(tok[2], &convres) != 0) {
                    invarg++;
                } else {
                    inp->coord2[0] = convres / 10;
                    inp->coord2[1] = convres % 10;
                }
            }
        } else if (cmd == CMDNEW) {
            inp->cmd = INP_NEW;
            if(tok[1] != NULL) {
                invarg++;
            }
        } else if (cmd == CMDQUIT) {
            inp->cmd = INP_QUIT;
            if(tok[1] != NULL) {
                invarg++;
            }
        } else {
            invcmd++;
        }

        /* check for invalid values of coords */
        if(inp->coord1[0] < 0 || inp->coord1[0] > SIDE_LEN ||
           inp->coord1[1] < 0 || inp->coord1[1] > SIDE_LEN ||
           inp->coord2[0] < 0 || inp->coord2[0] > SIDE_LEN ||
           inp->coord2[1] < 0 || inp->coord2[1] > SIDE_LEN) {
            invarg++;
        }
    }

    if(invcmd != 0) {
        ss_perror(errinv);
        printcommands();
        ret = -1;
    } else if(invarg != 0) {
        ss_perror(errarg);
        printcommands();
        ret = -1;
    }

    return(ret);
}
#undef MAXCMDNUM

int processturn(tttdata *data) {
    int ret = 0; /* -1 to repeat parsing, 1 on exit command, 0 to go on to next turn */
    char buf[BUFSIZE];
    input inp;
    int fieldind[2];

    if(quit != 0) {
        return(1);
    }

    (void)fprintf(stdout, "-----Your turn------\n");
    printboard(data);
    if(data->flags != 0) {
        if(data->flags & FLAG_WON) {
            (void)fprintf(stdout, "You win!");
        } else if(data->flags & FLAG_DRAW) {
            (void)fprintf(stdout, "Draw!");
        } else if(data->flags & FLAG_LOST) {
            (void)fprintf(stdout, "You lose!");
        } else if(data->flags & FLAG_CHEAT) {
            (void)fprintf(stdout, "You lose! Cheater!");
        }
        (void)fprintf(stdout, " Press the any key to continue.\n");
        (void)fgetc(stdin);
        data->command = CMD_NEXT_TURN;
        return(0);
    } else {
        (void)fprintf(stdout, "Enter command: ");
    }

    /* user input */
    if(fgets(buf, BUFSIZE, stdin) == NULL) {
        return(-1);
    }

    if(parseinput(buf, &inp) != 0) {
        return(-1);
    }
    switch(inp.cmd) {
        case INP_QUIT:
            (void)printf("exiting...\n");
            ret = 1;
            break;
        case INP_SHOW:
            printboard(data);
            ret = -1;
            break;
        case INP_HELP:
            printcommands();
            ret = -1;
            break;
        case INP_PLACE:
            data->command = CMD_NEXT_TURN;
            fieldind[0] = inp.coord1[1] * SIDE_LEN + inp.coord1[0];
            if(data->field[fieldind[0]] != STAT_CLEAR) {
                ss_perror("coord already marked");
                ret = -1;
            }
            data->field[fieldind[0]] = STAT_P1_X;
            break;
        case INP_CHEAT:
            data->command = CMD_NEXT_TURN;
            fieldind[0] = inp.coord1[1] * SIDE_LEN + inp.coord1[0];
            fieldind[1] = inp.coord2[1] * SIDE_LEN + inp.coord2[0];
            if(data->field[fieldind[0]] != STAT_CLEAR ||
               data->field[fieldind[1]] != STAT_CLEAR) {
                ss_perror("coord already marked");
                ret = -1;
            }
            data->field[fieldind[0]] = STAT_P1_X;
            data->field[fieldind[1]] = STAT_P1_X;
            break;
        case INP_NEW:
            data->command = CMD_NEW_GAME;
            break;
        default:
            assert(0);
    }
    /* do stuff */

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
    int retval;
    int semA, semB,  shm;
    tttdata *data;

    appname = argv[0];

    if(attachsighandlers() == -1) {
        return(EXIT_FAILURE);
    }
    /* argument handling */
    if(argc != 1) {
        usage();
        return(EXIT_FAILURE);
    }

    semA = grabsem(SEMAID);
    semB = grabsem(SEMBID);
    shm = grabshm();
    if(semA == -1 || semB == -1 || shm == -1) {
        ss_perror("could not attach to resources. make sure the server is running.");
        return(EXIT_FAILURE);
    }

    (void)printf("Welcome! Press h for help\n");

    data = (tttdata *)shmat(shm, (void *)0, 0);
    while(quit == 0) {
        /* the semaphores initial value is 1
         * P tries squeeze into the semaphore by subtracting one from it
         * however, this only works if at the semaphores value is at least one
         * if it is less than one, P blocks until there is enough space.
         * V adds 1 to the semaphore value, marking it as free */
        P(semB);
        /* working... */
        while((retval = processturn(data)) == -1);
        if(retval == 1) {
            quit = 1;
        }
        V(semA);
    }

    return(ret);
}
