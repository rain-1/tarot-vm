#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>

#include "objects.h"
#include "data.h"
#include "loader.h"
#include "interpreter.h"
#include "gc.h"
#include "builtins.h"

#include <signal.h>

void load_and_exec(char *filename);
void load_qcode_dir();

void stack_and_quit() {
	puts("SIGSEGV");
	stack_trace();
	exit(1);
}

void usage() {
	puts("./vm a.q b.q c.q -- arg1 arg2");
}

int main(int argc, char **argv) {
	if(argc < 2) {
		usage();
		exit(-1);
	}

	int i = 0;
	
	//signal(SIGSEGV, stack_and_quit);

	// find '--' position in argv list
	int dd_pos = -1;
	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "--")) {
			dd_pos = i;
			break;
		}
	}
	if(dd_pos == -1) {
		usage();
		exit(-1);
	}

	vm_init();

	/// SETUP ARGV
	reg_acc = ATOM_NUL;
	for(i = argc-1; i > dd_pos; i--) {
		// push argv[i] onto the list
		reg_clo = allocate_strg(argv[i], strlen(argv[i]));
		reg_acc = allocate_cons(reg_clo, reg_acc);
	}
	gc_add_permanent_root(reg_acc);
	
	// SETUP STACK
	stack[++reg_rsp] = 0xC0FFEEEEEEEEEEEE;
	stack[++reg_rsp] = SCM_PTR(NULL);
	stack[++reg_rsp] = SCM_PTR(NULL);
	stack[++reg_rsp] = 0;
	stack[++reg_rsp] = 0xDEADBEEFDEADBEEF;
	reg_rbp = reg_rsp;

	// SETUP STRUCTURES
	porttbl_init();
	builtins_init(reg_acc);
	profile_init();

	// LOAD AND EXEC CODE
	for(i = 1; i < dd_pos; i++) {
		load_and_exec(argv[i]);
	}

	profile_output();
	
	exit(0);
}

void load_and_exec(char *filename)
{
	scm *tmp;
	FILE *code_fptr = fopen(filename, "r");
	

	if(!code_fptr) {
		fprintf(stderr, "file <%s>\n", filename);
		exit(-1);
	}

	tmp = vm_code + vm_code_size;
	load_code(code_fptr);
	vm_exec(tmp);
}
