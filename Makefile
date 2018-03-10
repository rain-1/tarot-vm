CC=gcc
CFLAGS+=-g -O3 -std=c99 -Wall -Werror -Wno-unused-but-set-variable -D_GNU_SOURCE -falign-functions=8

SOURCES=read_word.c \
 data.c gc.c builtins.c \
 loader.c interpreter.c \
 main.c
OBJECTS=$(SOURCES:.c=.o)

.PHONY: all
all: vm

.PHONY: clean
clean:
	rm -f $(OBJECTS)
	rm -f vm

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

vm: $(OBJECTS)
	$(CC) -o vm $(CFLAGS) $(OBJECTS)
