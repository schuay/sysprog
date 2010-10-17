import socket
import random

HOST = ''
PORT = 91234
quit = False
score = 0
runningscore = 0
turncount = 0

def createsocket():
    global HOST, PORT
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((HOST, PORT))
    sock.listen(1)

    return sock

def otherturns():
    for player in (0, 1):
        for turn in range(1, 5):
            sendcmd("THRW %d %d %d blablabla\n" % (player, random.randint(1,6), random.randint(1,6)))

def handlecmd(tokens):
    global score, runningscore, quit, turncount

    if len(tokens) == 0:
        return

    cmd = tokens[0]

    print "rcvd:", tokens

    if cmd == "AUTH":
        print "authenticating user: ", tokens[1]
        sendcmd("TURN %d\n" % (score))
        turncount += 1
    elif cmd == "ROLL":

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
    elif cmd == "SAVE":
        score += runningscore
        runningscore = 0
        if score > 200:
            sendcmd("WIN\n")
        else:
            otherturns()
            sendcmd("TURN %d\n" % (score + runningscore))
            turncount += 1
    elif cmd == "BYE":
        sendcmd("BYE\n")
        print "turns taken:", turncount
        quit = True
    else:
        print "UNKNOWN CMD"

def sendcmd(cmd):
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
        sock.shutdown(2)
        sock.close()
