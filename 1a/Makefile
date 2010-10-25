GCC=gcc
#CFLAGS=-ansi -pedantic -Wall -g -D_GNU_SOURCE
CFLAGS=-ansi -pedantic -Wall -D_XOPEN_SOURCE=500
TARGET=app

all: app

app: main.c
	$(GCC) $(CFLAGS) main.c -o $(TARGET)

clean:
	rm -f $(TARGET)
