#####################################
# Module: SYSPROG WS 2010 TU Wien   #
# Author: Jakob Gruber ( 0203440 )  #
# Date:   2010-10-31                #
#####################################

GCC=gcc
CFLAGS=-ansi -pedantic -Wall -g -D_XOPEN_SOURCE=500
TARGET=app

all: app

app: main.c
	$(GCC) $(CFLAGS) main.c -o $(TARGET)

debug: main.c
	$(GCC) $(CFLAGS) -DDEBUG main.c -o $(TARGET)

clean:
	rm -f $(TARGET)
