CC=gcc
#CC=clang
CFLAGS+=-g -O3 -std=c99 -Wall -Werror -D_GNU_SOURCE
#CFLAGS+=-malign-functions=8
CFLAGS+=-falign-functions=8
#CFLAGS+=-fsanitize=address

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
