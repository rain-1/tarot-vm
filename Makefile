CFLAGS+=-g -O3 -std=c99 -Wall -Werror

CC=gcc
CFLAGS+=-falign-functions=8 -D_GNU_SOURCE

#CC=clang
#CFLAGS+=-fsanitize=address
#CFLAGS+=-malign-functions=8

#CC=tcc

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
