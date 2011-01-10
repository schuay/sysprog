/*********************************************
 * Module: SYSPROG WS 2010 TU Wien
 * Author: Jakob Gruber ( 0203440 )
 * Description: A Tic Tac Toe server using shared memory
 * Assignment: 3
 * *******************************************/

#define SIDE_LEN (3)
#define FIELD_LEN (SIDE_LEN * SIDE_LEN)

#define SEMKEYPATH ("/dev/null")
#define SEMSERVER (203440)
#define SEMCLIENT (203441)

#define SHMKEYPATH ("/dev/null")
#define SHMKEYID (1)

#define SEMBUSY (0)
#define SEMFREE (1)

enum tilestat {
    STAT_CLEAR,
    STAT_P1_X,
    STAT_AI_O
};
enum flags {
    FLAG_WON = 1 << 0,
    FLAG_LOST = 1 << 1,
    FLAG_CHEAT = 1 << 2,
    FLAG_ABORT = 1 << 3,
    FLAG_DRAW = 1 << 4
};
enum cmds {
    CMD_NONE,
    CMD_NEXT_TURN,
    CMD_NEW_GAME
};

typedef struct {
    enum tilestat field[FIELD_LEN];
    int flags;
    enum cmds command;
} tttdata;
