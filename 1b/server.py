#!/bin/env python26

import socket
import random

HOST = ''
PORT = 91234
quit = False
score = 0
runningscore = 0
turncount = 0

def proc_AUTH():
    global turncount

    sendcmd("TURN %d\n" % (score))
    turncount += 1
def proc_ROLL():
    global runningscore, turncount

    val1 = random.randint(1, 6)
    val2 = random.randint(1, 6)
    sendcmd("THRW 2 %d %d\n" % (val1, val2))

    if val1 == val2:
        runningscore = 0
        otherturns()
    else:
        runningscore += val1 + val2

    sendcmd("TURN %d\n" % (score + runningscore))
    turncount += 1
def proc_SAVE():
    global score, runningscore, turncount

    score += runningscore
    runningscore = 0
    if score > 200:
        sendcmd("WIN\n")
    else:
        otherturns()
        sendcmd("TURN %d\n" % (score + runningscore))
        turncount += 1
def proc_BYE():
    global quit

    sendcmd("BYE\n")
    print "turns taken:", turncount
    quit = True

cmds = {\
    'AUTH':proc_AUTH,\
    'ROLL':proc_ROLL,\
    'SAVE':proc_SAVE,\
    'BYE':proc_BYE}

def createsocket():
    """creates, binds, and listens an INET STREAM socket"""
    global HOST, PORT
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((HOST, PORT))
    sock.listen(1)

    return sock

def otherturns():
    """sends some random stuff for other players' turns"""
    for player in (0, 1):
        for turn in range(1, 5):
            sendcmd("THRW %d %d %d blablabla\n" % \
                (player, random.randint(1,6), random.randint(1,6)))

def handlecmd(tokens):
    """processes incoming commands"""
    if len(tokens) == 0:
        return

    print "rcvd:", tokens

    if tokens[0] not in cmds:
        print "ERR: unknown cmd"
        return

    cmds[tokens[0]]()

def sendcmd(cmd):
    """sends an outgoing command"""
    global conn

    print "sent:", cmd
    conn.send(cmd)
    
if __name__ == "__main__":

    sock = createsocket()

    try:
        while True:
            conn, addr = sock.accept()
            print 'Connected by ', addr

            sendcmd("HELO\n")
            while quit != True:
                data = conn.recv(256)
                handlecmd(data.split())

            with open("results", 'a') as f:
                f.write("%d\n" % (turncount))

            conn.close()
            quit = False
            score = runningscore = turncount = 0

            print "ready for next connection"

    finally:
        conn.close()
        sock.close()
