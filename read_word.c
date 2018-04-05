#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *wbuf = NULL;
int wlen;
	
char *read_word(FILE *fptr) {
	int c, i;

	if(feof(fptr)) {
		return NULL;
	}

	c = fgetc(fptr);
	if(c == EOF) {
		return NULL;
	}

	if(!wbuf) {
		wlen = 64;
		wbuf = malloc(wlen);
	}
	
	i = 0;
	do {
		wbuf[i++] = c;
		
		if(!c) break;
		
		if(i >= wlen) {
			wlen *= 2;
			wbuf = realloc(wbuf, wlen);
		}
	} while((c = fgetc(fptr)) != EOF);
	
	return wbuf;
}
