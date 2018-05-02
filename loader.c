#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "qcodes.h"
#include "read_word.h"
#include "objects.h"
#include "gc.h"
#include "data.h"

num load_number(FILE *fptr);
char* load_string(FILE *fptr);
scm load_symbol(FILE *fptr);
scm load_char(FILE *fptr);

void load_code(FILE *fptr) {
	char *w, *info;
	scm sym, sym2;
	
	while((w = read_word(fptr))) {
		if(!strcmp("halt",w)) {
			vm_add_codeword(CODE_HALT);
			continue;
		}
		
		if(!strcmp("datum-false",w)) {
			vm_add_codeword(CODE_DATUM_FALSE);
			continue;
		}
		if(!strcmp("datum-true",w)) {
			vm_add_codeword(CODE_DATUM_TRUE);
			continue;
		}
		if(!strcmp("datum-null",w)) {
			vm_add_codeword(CODE_DATUM_NULL);
			continue;
		}
		if(!strcmp("datum-symbol",w)) {
			vm_add_codeword(CODE_DATUM_SYMBOL);
			load_symbol(fptr);
			continue;
		}
		if(!strcmp("datum-char",w)) {
			vm_add_codeword(CODE_DATUM_CHAR);
			load_char(fptr);
			continue;
		}
		if(!strcmp("datum-number",w)) {
			vm_add_codeword(CODE_DATUM_NUMBER);
			load_number(fptr);
			continue;
		}
		if(!strcmp("datum-string",w)) {
			vm_add_codeword(CODE_DATUM_STRING);
			load_string(fptr);
			continue;
		}
		
		if(!strcmp("allocate-closure",w)) {
			vm_add_codeword(CODE_ALLOCATE_CLOSURE);
			load_number(fptr);
			load_number(fptr);
			continue;
		}
		if(!strcmp("closure-set!",w)) {
			vm_add_codeword(CODE_CLOSURE_SET);
			load_number(fptr);
			continue;
		}
		
		if(!strcmp("var-glo",w)) {
			vm_add_codeword(CODE_VAR_GLO);
			w = read_word(fptr);
			if(!w) { puts("didnt happen"); exit(-1); }
			sym = symtab_intern(w);
			if(!glovar_lookup(sym)) {
				glovar_define(sym, ATOM_FLS, ATOM_FLS);
			}
			void *g = glovar_lookup(sym);
			vm_add_codeword(SCM_PTR(g));
			continue;
		}
		if(!strcmp("set-glo",w)) {
			vm_add_codeword(CODE_SET_GLO);
			w = read_word(fptr);
			if(!w) { puts("didnt happen"); exit(-1); }
			sym = symtab_intern(w);
			if(!glovar_lookup(sym)) {
				glovar_define(sym, ATOM_FLS, ATOM_FLS);
			}
			void *g = glovar_lookup(sym);
			vm_add_codeword(SCM_PTR(g));
			continue;
		}
		if(!strcmp("var-loc",w)) {
			vm_add_codeword(CODE_VAR_LOC);
			load_number(fptr);
			continue;
		}
		if(!strcmp("set-loc",w)) {
			vm_add_codeword(CODE_SET_LOC);
			load_number(fptr);
			continue;
		}
		if(!strcmp("var-env",w)) {
			vm_add_codeword(CODE_VAR_ENV);
			load_number(fptr);
			continue;
		}
		if(!strcmp("set-env",w)) {
			vm_add_codeword(CODE_SET_ENV);
			load_number(fptr);
			continue;
		}
		if(!strcmp("clo-set-acc",w)) {
			vm_add_codeword(CODE_CLO_SET_ACC);
			continue;
		}
		if(!strcmp("clo-set-loc",w)) {
			vm_add_codeword(CODE_CLO_SET_LOC);
			load_number(fptr);
			continue;
		}
		if(!strcmp("set-clo-reg",w)) {
			vm_add_codeword(CODE_SET_CLO_REG);
			continue;
		}
		
		if(!strcmp("jump",w)) {
			vm_add_codeword(CODE_JUMP);
			load_number(fptr);
			continue;
		}
		if(!strcmp("branch",w)) {
			vm_add_codeword(CODE_BRANCH);
			load_number(fptr);
			continue;
		}
		if(!strcmp("push",w)) {
			vm_add_codeword(CODE_PUSH);
			continue;
		}
		if(!strcmp("stack-grow",w)) {
			vm_add_codeword(CODE_STACK_GROW);
			load_number(fptr);
			continue;
		}
		if(!strcmp("stackframe",w)) {
			vm_add_codeword(CODE_STACKFRAME);
			load_number(fptr);
			continue;
		}
		if(!strcmp("call",w)) {
			vm_add_codeword(CODE_CALL);
			continue;
		}
		if(!strcmp("ret",w)) {
			vm_add_codeword(CODE_RET);
			continue;
		}
		if(!strcmp("shiftback",w)) {
			vm_add_codeword(CODE_SHIFTBACK);
			load_number(fptr);
			continue;
		}
		
		if(!strcmp("information",w)) {
			vm_add_codeword(CODE_INFORMATION);
			sym = load_number(fptr);
			sym2 = load_number(fptr);
			info = load_string(fptr);
			information_store(vm_code + vm_code_size + sym,
					  vm_code + vm_code_size + sym2,
					  info);
			continue;
		}
		
		fprintf(stderr, "load_code unknown word <%s> %ld\n", w, strlen(w));
		exit(-1);
	}
}


num load_number(FILE *fptr) {
	char *w;
	num n;

	w = read_word(fptr);
	if(!w) {
		fprintf(stderr, "load_number\n");
		exit(-1);
	}
	
	errno = 0;
	n = strtoll(w, NULL, 10);
	if(errno) {
		fprintf(stderr, "load_number <%s>\n", w);
		exit(-1);
	}

	vm_add_codeword(n);
	
	return n;
}

char* load_string(FILE *fptr) {
	char *w;
	scm str;

	w = read_word(fptr);
	if(!w) {
		fprintf(stderr, "load_string\n");
		exit(-1);
	}

	//w = strdup(w);
	//n = SCM_PTR(w);
	
	str = allocate_strg(w, strlen(w));
	gc_add_permanent_root(str);
	vm_add_codeword(str);
	
	// this needs to be a GC root though

	return w;
}

scm load_symbol(FILE *fptr) {
	char *w;
	scm n;

	w = read_word(fptr);
	if(!w) {
		fprintf(stderr, "load_string\n");
		exit(-1);
	}
	
	n = symtab_intern(w);
	
	vm_add_codeword(n);

	return n;
}

scm load_char(FILE *fptr) {
	char *w;
	scm n;

	w = read_word(fptr);
	if(!(w[0] && !w[1])) {
		fprintf(stderr, "load_char\n");
		exit(-1);
	}
	
	n = w[0];

	vm_add_codeword(mk_chr(n));
	
	return n;
}
