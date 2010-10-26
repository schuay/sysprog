/*********************************************
 * Module: SYSPROG WS 2010 TU Wien
 * Author: Jakob Gruber ( 0203440 )
 * Description: A dice rolling client
 * Assignment: 1b
 * *******************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <stdarg.h>

#define MAX_STR_LEN 100
#define NUM_PLAYERS 3
#define ME 2
#define WINNING_SCORE 200

/* GLOBALS */

FILE
    *sockf = NULL;
struct addrinfo 
    *serv_addr = NULL;
char 
    inbuffer[MAX_STR_LEN + 1], 
    outbuffer[MAX_STR_LEN + 1],
    overflowbuffer[MAX_STR_LEN + 1];
char 
    *appname;
int 
    scores[NUM_PLAYERS], 
    runningscore, 
    activeplayer;

struct {
    const char *name;
    const char *host;
    const char *port;
    long int limit;
} config;

/*
Name:       strtoint
Desc:       safely converts a string to a long int
Args:
    str:    string to convert
    val:    conversion destination
Returns:    0 on success, nonzero on failure
Globals:    appname
*/
int strtoint(const char *str, long int *val) {
    if(str == NULL) {
        return(1);
    }

    errno = 0;
    *val = strtol(str, NULL, 0);
    if(errno != 0) {
        fprintf(stderr, "%s: %s is not a number\n", appname, str);
        return(1);
    }
    
    return(0);
}

/*
Name:   usage
Desc:   prints usage instructions to stdout
Args:   -
Returns:-
Globals:appname
*/
void usage(void) {
    printf("usage: %s -n <bot-name> [-p <server-port> -l <limit>] <server-hostname>\n", appname);
}

/*
Name:   parseargs
Desc:   parses and processes cli args
Args:
    argc:   argument count
    argv:   arguments
Returns:
    0 on success, nonzero on failure
Globals:appname,config
*/
int parseargs(int argc, char **argv) {
    int opt;

    while((opt = getopt(argc, argv, "l:n:p:h"))) {
        if(opt < 0) {
            break;
        }

        switch(opt) {
            case 'n':
                if(optarg == NULL) {
                    fprintf(stderr, "%s: option %c requires an argument\n", appname, opt);
                    return(1);
                }
                config.name = optarg;
                break;
            case 'p':
                if(optarg == NULL) {
                    fprintf(stderr, "%s: option %c requires an argument\n", appname, opt);
                    return(1);
                }
                config.port = optarg; 
                break;
            case 'l':
                if(strtoint(optarg, &config.limit) != 0) {
                    fprintf(stderr, "%s: option %c requires an argument\n", appname, opt);
                    return(1);
                }
                break;
            case 'h':
            case 0:
            case '?':
                usage();
                return(-1);
            default:
                assert(0);
        }
    }

    for(; optind < argc; optind++) {
        config.host = argv[optind]; 
    }

    return(0);
}

/*
Name:   setconfigdefaults
Desc:   sets predetermined config defaults
Args:   -
Returns:-
Globals:config
*/
void setconfigdefaults(void) {
    config.port = "9001";
    config.limit = 36;
}

/*
Name:   validateconfig
Desc:   checks if configuration is legal
Args:   -
Returns:
    0 on success, nonzero on failure
Globals:appname
*/
int validateconfig(void) {
    if(config.name == NULL) {
        fprintf(stderr, "%s: name not specified\n", appname);
        return(1);
    }
    if(config.host == NULL) {
        fprintf(stderr, "%s: host not specified\n", appname);
        return(1);
    }
    return(0);
}

/*
Name:   cleanup
Desc:   safely deallocates global resources
Args:   -
Returns:-
Globals:sockf,serv_addr
*/
void cleanup(void) {
    /* socket
     * addrinfo
     */

    if(serv_addr != NULL) {
        freeaddrinfo(serv_addr);
    }
    if(sockf != NULL) {
        fclose(sockf);
    }
}

/*
Name:   safe_exit
Desc:   deallocates resources and exits. sig handler for SIGINT, SIGQUIT, SIGTERM
Args:
    sig:    signal
Returns:-
*/
void safe_exit(int sig) {
    fprintf(stderr, "Received termination request - terminating...\n");
    cleanup();
    exit(sig);
}

/*
Name:   sndmsg
Desc:   sends a message through the socket
Args:
    str:    printf like format string
    ...:    printf format args
Returns:
    0 on success, nonzero on failure
Globals:sockf,outbuffer
*/
int sndmsg(const char *str, ...) {
    va_list args;

    memset(outbuffer, 0, sizeof(outbuffer));

    va_start(args, str);
    vsprintf(outbuffer, str, args);
    va_end(args);

    if(fputs(outbuffer, sockf) < 0) {
        return(1);
    }

#ifdef DEBUG
    printf("sent: %s", outbuffer);
#endif

    return(0);
}

/*
Name:   processmsg_throw
Desc:   handle a THRW msg received from server
Args:
    playerid:   id of current player
    val1:       first thrown value
    val2:       second thrown value
Returns:
    0 on success, nonzero on failure
Globals:appname,scores,runningscore,activeplayer
*/
int processmsg_throw(const char *playerid, const char *val1, const char *val2) {
    long int iplayerid, ival1, ival2;

    /* sanity checks + string to int conversions */
    if(strtoint(playerid, &iplayerid) != 0 || iplayerid > 2 || iplayerid < 0) {
        fprintf(stderr, "%s: protocol error\n", appname);
        return(-1);
    }
    if(strtoint(val1, &ival1) != 0 || ival1 > 6 || ival1 < 0) {
        fprintf(stderr, "%s: protocol error\n", appname);
        return(-1);
    }
    if(strtoint(val2, &ival2) != 0 || ival2 > 6 || ival2 < 0) {
        fprintf(stderr, "%s: protocol error\n", appname);
        return(-1);
    }

    /* switch active player */
    if(activeplayer != iplayerid) {
        activeplayer = iplayerid;
        runningscore = 0;
    }

    /* process thrown values */
    if(ival1 == ival2) {
        scores[iplayerid] -= runningscore;
        runningscore = 0;
    } else {
        scores[iplayerid ] += ival1 + ival2;
        runningscore += ival1 + ival2;
    }

    return(0);
}

/*
Name:   torollornottoroll
Desc:   decide whether to roll or save
Args:   -
Returns:
    0: save, 1: roll
Globals:scores,runningscore,activeplayer,config
*/
#define MAX(a, b) ((a) > (b) ? (a) : (b))
int torollornottoroll(void) {
    int limit = config.limit;
    int leaderscore = MAX(scores[0], scores[1]);

    /* raise limit if others are at 0 points, */
    /* or others are more than limit ahead */
    if((scores[0] == 0 && scores[1] == 0) ||
       leaderscore > scores[ME] + limit) {
        limit *= 1.5;
    }

    /* PANIC MODE if others are close to winning, */
    /* or we are close to winning */
    if(leaderscore > WINNING_SCORE - 40 ||
       WINNING_SCORE - scores[ME] < 6) {
        limit = 1000;
    }

    /* my first move in this turn, roll! */
    if(activeplayer != ME) {
        return(1);
    }

    /* reached our turn point limit, save */
    if(runningscore > limit &&
       runningscore + scores[ME] > leaderscore) {
        return(0);
    }
    /* reached a winning score, save */
    if(scores[ME] > WINNING_SCORE) {
        return(0);
    }

    /* else roll */
    return(1);
}

/*
Name:   processmsg_turn
Desc:   handle TURN msg received from server
Args:
    ownscore:   current value of own score
Returns:
    0 on success, nonzero on failure
Globals:appname,scores
*/
int processmsg_turn(const char *ownscore) {
    long int iownscore;
    char *answer;

    if(strtoint(ownscore, &iownscore) != 0) {
        fprintf(stderr, "%s: protocol error\n", appname);
        return(-1);
    }

    /* make sure we are tracking the score correctly */
    assert(iownscore == scores[ME]);

    /* decide whether to save or roll */
    answer = (torollornottoroll() == 0) ?
             "SAVE" :
             "ROLL";

    if(sndmsg("%s\n", answer) != 0) {
        return(1);
    }

    return(0);
}

/*
Name:   processmsg
Desc:   process messages after retrieval from server. this includes handling
        message overflow (a message has been cut off and will be continued in
        the next buffer) and messge splitting (buffer contains more than one msg)
Args:   -
Returns:
    0 on success, nonzero on failure
Globals:inbuffer,overflowbuffer,appname,scores,config
*/
int processmsg(void) {
    char 
        *unmodifiedbuffer,          /* unmodified version of tokbuffer */
        *tok[5],                    /* tokens of current cmd (tokbuffer) */
        *save,                      /* savepoint for strtok */
        *search = " \n";
    int 
        i,
        ret = 0;                    /* return code */

    unmodifiedbuffer = strdup(inbuffer);

    /* split cmd into tokens (we only care - at most - about the first 5 */
    tok[0]= strtok_r(inbuffer, search, &save);
    for (i = 1; i < 5; i++) {
        tok[i] = strtok_r(NULL, search, &save);
    }

    /* parse cmd and take appropriate action */
    if(tok[0] == NULL) {
        fprintf(stderr, "%s: empty token , ignoring...\n", appname);
    } else if(strcmp(tok[0], "HELO") == 0) {
        if(sndmsg("AUTH %s\n", config.name) != 0) ret = 1;
    } else if(strcmp(tok[0], "TURN") == 0) {
        if(processmsg_turn(tok[1]) != 0) ret = 1;
    } else if(strcmp(tok[0], "THRW") == 0) {
        if(processmsg_throw(tok[1], tok[2], tok[3]) != 0) ret = 1;
    } else if(strcmp(tok[0], "WIN") == 0 ||
              strcmp(tok[0], "DEF") == 0) {
        printf("%s\n", tok[0]);
        if(sndmsg("BYE %d %d %d\n", scores[2], scores[0], scores[1]) != 0) ret = 1;
    } else if(strcmp(tok[0], "BYE") == 0) {
        ret = 2;
    } else if(strcmp(tok[0], "ERR") == 0) {
        fprintf(stderr, "%s: server error: %s\n", appname, unmodifiedbuffer);
        ret = 1;
    } else {
        fprintf(stderr, "%s: unrecognized token %s, ignoring...\n",
            appname, tok[0]);
    }

    free(unmodifiedbuffer);

    return(ret);
}

/*
Name:   rcvmsg
Desc:   reads messages from the socket
Args:   -
Returns:
    0 on success, nonzero on failure
Globals:sockf,inbuffer,appname
*/
int rcvmsg(void) {
    char *ret;

    memset(inbuffer, 0, sizeof(inbuffer));
    ret = fgets(inbuffer, MAX_STR_LEN, sockf);

    if (ret == NULL) {
        return(1);  /* EOF */
    }

#ifdef DEBUG
    printf("rcvd: %s", inbuffer);
#endif

    return(0);
}

/*
Name:   createandconnectsocket
Desc:   creates and connects a socket
Args:   -
Returns:
    0 on success, nonzero on failure
Globals:sockf,serv_addr,appname,config
*/
int createandconnectsocket(void) {

    struct addrinfo hints;
    int error;
    int sockfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    error = getaddrinfo(config.host, config.port, &hints, &serv_addr);
    if(error != 0) {
        fprintf(stderr, "%s: error in getaddrinfo: %s\n", appname, gai_strerror(error));
        return(1);
    }
    sockfd = socket(serv_addr->ai_family, serv_addr->ai_socktype, serv_addr->ai_protocol);
    if(sockfd < 0) {
        fprintf(stderr, "%s: error opening socket\n", appname);
        return(1);
    }
    if(connect(sockfd, serv_addr->ai_addr, serv_addr->ai_addrlen) != 0) {
        fprintf(stderr, "%s: error connecting socket: %s\n", appname, strerror(errno));
        return(1);
    }
    sockf = fdopen(sockfd, "r+");
    if(sockf == NULL) {
        fprintf(stderr, "%s: error opening socket FILE: %s\n", appname, strerror(errno));
        return(1);
    }

    return(0);
}

int main(int argc, char **argv) {
    signal(SIGINT, safe_exit);
    signal(SIGQUIT, safe_exit);
    signal(SIGTERM, safe_exit);

    appname = argv[0];

    setconfigdefaults();
    if(parseargs(argc, argv) != 0) {
        return(1);
    }
    if(validateconfig() != 0) {
        usage();
        return(1);
    }

#ifdef DEBUG
    printf("name: %s\n", config.name);
    printf("host: %s\n", config.host);
    printf("port: %s\n", config.port);
    printf("limit: %d\n", (int)config.limit);
#endif

    if(createandconnectsocket() != 0) {
        cleanup();
        return(1);
    }

    while(rcvmsg() == 0) {
        if(processmsg() != 0) {
            break;
        }
    }

    cleanup();
    return(0);
}