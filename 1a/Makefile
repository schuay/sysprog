#####################################
# Module: SYSPROG WS 2010 TU Wien   #
# Author: Jakob Gruber ( 0203440 )  #
# Date:   2010-10-31                #
#####################################

GCC=gcc
CFLAGS=-ansi -pedantic -Wall -D_XOPEN_SOURCE=500
TARGET=mycompress

all: $(TARGET)

$(TARGET): main.c
	$(GCC) $(CFLAGS) main.c -o $(TARGET)

clean:
	rm -f $(TARGET)
