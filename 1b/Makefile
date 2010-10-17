GCC=gcc
CFLAGS=-ansi -pedantic -Wall -g -D_GNU_SOURCE
TARGET=app

all: app

app: main.c
	$(GCC) $(CFLAGS) main.c -o $(TARGET)

debug: main.c
	$(GCC) $(CFLAGS) -DDEBUG main.c -o $(TARGET)

clean:
	rm -f $(TARGET)
