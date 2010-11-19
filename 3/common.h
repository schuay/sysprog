/*********************************************
 * Module: SYSPROG WS 2010 TU Wien
 * Author: Jakob Gruber ( 0203440 )
 * Description: A Tic Tac Toe server using shared memory
 * Assignment: 3
 * *******************************************/

#define SIDE_LEN (3)
#define FIELD_LEN (SIDE_LEN * SIDE_LEN)

enum tilestat {
    STAT_CLEAR,
    STAT_P1_X,
    STAT_P1_O,
    STAT_P2_X,
    STAT_P2_O
};
enum flags {
    FLAG_WON = 1 << 0,
    FLAG_LOST = 1 << 1,
    FLAG_CHEAT = 1 << 2,
    FLAG_ABORT = 1 << 3
};
enum cmds {
    CMD_NEXT_TURN,
    CMD_NEW_GAME
};

typedef struct {
    enum tilestat field[FIELD_LEN];
    int flags;
    enum cmds command;
} tttdata;
