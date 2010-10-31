#####################################
# Module: SYSPROG WS 2010 TU Wien   #
# Author: Jakob Gruber ( 0203440 )  #
# Date:   2010-10-31                #
#####################################

GCC=gcc
CFLAGS=-ansi -pedantic -Wall -c -D_XOPEN_SOURCE=500
LDFLAGS=
TAR=tar
TARFLAGS=-cvz
SOURCES=main.c util.c
OBJECTS=$(SOURCES:.c=.o)
TARGET=mygzip

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(GCC) $(LDLAGS) $(OBJECTS) -o $@

%.o: %.c
	$(GCC) $(CFLAGS) $< -o $@

archive:
	$(TAR) $(TARFLAGS) -f $(TARGET).tgz $(SOURCES) Makefile

clean:
	rm -f $(TARGET) $(OBJECTS)
