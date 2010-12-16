/*********************************************
 * Module: SYSPROG WS 2010 TU Wien
 * Author: Jakob Gruber ( 0203440 )
 * Description: A Tic Tac Toe server using shared memory
 * Assignment: 3
 * *******************************************/

#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sem182.h>
#include <time.h>

#include "util.h"
#include "common.h"

typedef struct {
    int semAid;
    int semBid;
    int shmid;
} resources;

/* GLOBALS */

static int cheatcheck = 1;
static volatile int quit = 0;

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
            cheatcheck--;
            break;
        default:
            ret = 1;
        }
    }

    if(cheatcheck < 0) {
        ret = 1;
    }

    return(ret);
}
int createsem(int id, int val) {
    key_t key;
    int sem;

    if((key = ftok(SEMKEYPATH, id)) == -1) {
        perror("ftok");
        return(-1);
    }
    if((sem = seminit(key, 0600, SEMFREE)) == -1) {
        ss_perror("seminit failed");
        return(-1);
    }

    return(sem);
}
int createshm() {
    key_t key;
    int shm;

    if((key = ftok(SHMKEYPATH, SHMKEYID)) == -1) {
        perror("ftok");
        return(-1);
    }
    if((shm = shmget(key, sizeof(tttdata), 0644 | IPC_CREAT)) == -1) {
        perror("shmget");
        return(-1);
    }

    return(shm);
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
int initresources(resources *res) {
    /* initial values */
    res->semAid = res->semBid = res->shmid = -1;

    /* create semaphore and shm */
    if((res->semAid = createsem(SEMAID, SEMFREE)) == -1) {
        return(-1);
    }
    if((res->semBid = createsem(SEMBID, SEMBUSY)) == -1) {
        return(-1);
    }
    if((res->shmid = createshm()) == -1) {
        return(-1);
    }

    return(0);
}
int deinitresources(resources *res) {
    int ret = 0;
    if(res->semAid != -1) {
        if(semrm(res->semAid) == -1) {
            ss_perror("semrm failed");
            ret = EXIT_FAILURE;
        }
    }
    if(res->semBid != -1) {
        if(semrm(res->semBid) == -1) {
            ss_perror("semrm failed");
            ret = EXIT_FAILURE;
        }
    }
    if(res->shmid != -1) {
        if(shmctl(res->shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl");
            ret = EXIT_FAILURE;
        }
    }
    return(ret);
}

void cptttdata(tttdata *src, tttdata *dst) {
    int i;
    for(i = 0; i < FIELD_LEN; i++) {
        dst->field[i] = src->field[i];
    }
    dst->flags = src->flags;
    dst->command = src->command;
}
void initgame(tttdata *data) {
    int i;
    memset(data, 0, sizeof(tttdata));
    data->flags = 0;
    for (i = 0; i < FIELD_LEN; i++) {
        data->field[i] = STAT_CLEAR;
    }
}
int validatemove(const tttdata *cur, const tttdata *old) {
    int ret = 0;
    int movenum = 0;
    int i;
    for(i = 0; i < FIELD_LEN; i++) {
        if(old->field[i] != cur->field[i]) {
            /* only valid change is STAT_CLEAR -> STAT_P1_X */
            if(old->field[i] != STAT_CLEAR || cur->field[i] != STAT_P1_X) {
                ret = 1;
                ss_perror("invalid client move");
            } else {
                movenum++;
            }
        }
    }
    if(cheatcheck == 1 && movenum > 1) {
        ss_perror("cheat attempt detected");
        ret = 1;
    }
    return(ret); 
}
int isgamewon(const tttdata *data) {
    /* 0 no win, 1 P1 win, 2 AI win, 3 draw */
    int i, j;
    int sumAI, sumP1;
    int freetilecount = 0;

    /* get number of free tiles */
    for(i = 0; i < FIELD_LEN; i++) {
        if(data->field[i] == STAT_CLEAR) {
            freetilecount++;
        }
    }
    if(freetilecount == 0) {
        return(3);
    }

    for(i = 0; i < SIDE_LEN; i++) {
        /* check rows */
        sumAI = sumP1 = 0;
        for(j = 0; j < SIDE_LEN; j++) {
            if(data->field[i * SIDE_LEN + j] == STAT_P1_X) {
                sumP1++;
            } else if(data->field[i * SIDE_LEN + j] == STAT_AI_O) {
                sumAI++;
            }
        }
        if(sumAI == 3) {
            return(2);
        } else if(sumP1 == 3) {
            return(1);
        }
        /* check cols */
        sumAI = sumP1 = 0;
        for(j = 0; j < SIDE_LEN; j++) {
            if(data->field[j * SIDE_LEN + i] == STAT_P1_X) {
                sumP1++;
            } else if(data->field[j * SIDE_LEN + i] == STAT_AI_O) {
                sumAI++;
            }
        }
        if(sumAI == 3) {
            return(2);
        } else if(sumP1 == 3) {
            return(1);
        }
    }
    /* check diagonal 1 */
    sumAI = sumP1 = 0;
    for(i = 0; i < SIDE_LEN; i++) {
        if(data->field[i * SIDE_LEN + SIDE_LEN - i - 1] == STAT_P1_X) {
            sumP1++;
        } else if(data->field[i * SIDE_LEN + SIDE_LEN - i - 1] == STAT_AI_O) {
            sumAI++;
        }
    }
    if(sumAI == 3) {
        return(2);
    } else if(sumP1 == 3) {
        return(1);
    }
    /* check diagonal 2 */
    sumAI = sumP1 = 0;
    for(i = 0; i < SIDE_LEN; i++) {
        if(data->field[i * SIDE_LEN + i] == STAT_P1_X) {
            sumP1++;
        } else if(data->field[i * SIDE_LEN + i] == STAT_AI_O) {
            sumAI++;
        }
    }
    if(sumAI == 3) {
        return(2);
    } else if(sumP1 == 3) {
        return(1);
    }
    return(0);
}

void makemove(tttdata *data) {
    int i;
    int freetilecount = 0;
    int pickedtile;

    if(data->command == CMD_NONE) {
        return;
    }

    /* get number of free tiles */
    for(i = 0; i < FIELD_LEN; i++) {
        if(data->field[i] == STAT_CLEAR) {
            freetilecount++;
        }
    }

    /* pick random free tile */
    pickedtile = rand() % freetilecount;

    /* mark that tile */
    freetilecount = 0;
    for(i = 0; i < FIELD_LEN; i++) {
        if(data->field[i] == STAT_CLEAR) {
            if(freetilecount == pickedtile) {
                data->field[i] = STAT_AI_O;
            }
            freetilecount++;
        }
    }
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
    resources res;
    tttdata *data = NULL, olddata;
    int newgame = 0;

    appname = argv[0];

    if(attachsighandlers() == -1) {
        return(EXIT_FAILURE);
    }

    /* argument handling */
    if(argc > 2 || parseargs(argc, argv) != 0) {
        usage();
        return(EXIT_FAILURE);
    }

    /* allocate semaphores and shm */
    if(initresources(&res) != 0) {
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    /* attach to shm */
    data = (tttdata *)shmat(res.shmid, (void *)0, 0);
    if (data == (tttdata *)(-1)) {
        perror("shmat");
        goto cleanup;
    }

    /* seed random number gen */
    srand(time(NULL));

    /* prepare for first game */
    P(res.semAid);
    initgame(data);
    cptttdata(data, &olddata);

    while (!quit) {
        /* lock server sem (semA) */
        P(res.semAid);
        /* do stuff */
        if(newgame != 0 || data->command == CMD_NEW_GAME) {
            initgame(data);
            newgame = 0;
        } else if(validatemove(data, &olddata) != 0) {
            data->flags = FLAG_CHEAT;
            newgame = 1;
        } else if (isgamewon(data) == 1) {
            data->flags = FLAG_WON;
            newgame = 1;
        } else if (isgamewon(data) == 3) {
            data->flags = FLAG_DRAW;
            newgame = 1;
        } else {
            /* make move,
             * check win conditions again */
            makemove(data);
            if(isgamewon(data) == 2) {
                data->flags = FLAG_LOST;
                newgame = 1;
            }
        }
        data->command = CMD_NONE;
        cptttdata(data, &olddata);
        /* unlock client sem (semB) */
        V(res.semBid);
    }

cleanup:
    if(deinitresources(&res) != 0) {
        ret = EXIT_FAILURE;
    }

    return(ret);
}
