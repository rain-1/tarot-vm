#include <assert.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "objects.h"
#include "data.h"
#include "loader.h"
#include "interpreter.h"
#include "gc.h"

// get Nth thing on the stack
#define STACK(N) stack[reg_rbp + 1 + N]

#if 0
#define ALIGN __attribute((aligned (8)))
#else
#define ALIGN
#endif

//////////////////////////////////////////////////
// list functions

scm bltn_cons(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	return allocate_cons(p, q);
}

scm bltn_car(void) ALIGN {
	scm a = STACK(0);
	return get_cons_car(a);
}

scm bltn_cdr(void) ALIGN {
	scm a = STACK(0);
	return get_cons_cdr(a);
}

scm bltn_nullq(void) ALIGN {
	if (STACK(0) == ATOM_NUL)
		return ATOM_TRU;
	else
		return ATOM_FLS;
}

scm bltn_pairq(void) ALIGN {
	return mk_bool(scm_gettag(STACK(0)) == TAG_CONS);
}

scm bltn_symbolq(void) ALIGN {
	return mk_bool(scm_gettag(STACK(0)) == ATOM_SYM);
}

scm bltn_stringq(void) ALIGN {
	return mk_bool(scm_gettag(STACK(0)) == TAG_STRG);
}

scm bltn_charq(void) ALIGN {
	return mk_bool(scm_gettag(STACK(0)) == ATOM_CHR);
}

scm bltn_booleanq(void) ALIGN {
	scm x = scm_gettag(STACK(0));
	return mk_bool(x == ATOM_TRU || x == ATOM_FLS);
}

scm bltn_numberq(void) ALIGN {
	return mk_bool(scm_gettag(STACK(0)) == TAG_NUMB);
}

scm bltn_procedureq(void) ALIGN {
	return mk_bool(scm_gettag(STACK(0)) == TAG_BLTN || scm_gettag(STACK(0)) == TAG_CLOS);
}

void display_helper(FILE *to, scm atom);
	
scm bltn_error(void) ALIGN {
	scm atom = STACK(0);
	display_helper(stderr, atom);
	fprintf(stderr, "\n");
	stack_trace();
	exit(1);
}

scm bltn_exit(void) ALIGN {
	exit(1);
}




//////////////////////////////////////////////////
// printing functions

scm bltn_display_port(void) ALIGN {
	scm atom = STACK(1);
	scm port = STACK(0);
	FILE *to;

	if(scm_gettag(port) == ATOM_FLS) {
		to = stdout;
	}
	else if(scm_gettag(port) == ATOM_PRT) {
		to = port_get_file(port);
	}
	else {
		printf("DBG tag %lu\n", scm_gettag(atom));
		info_assert("not a port" == 0);
	}

	display_helper(to, atom);

	return ATOM_FLS;
}

void display_helper(FILE *to, scm atom) {
	switch (scm_gettag(atom)) {
	case ATOM_FLS:
		fprintf(to, "#f");
		break;

	case ATOM_TRU:
		fprintf(to, "#t");
		break;

	case ATOM_NUL:
		fprintf(to, "()");
		break;

	case ATOM_SYM:
		fprintf(to, "%s", symtab_lookup(atom));
		break;

	case ATOM_CHR:
		fprintf(to, "%c", (char)get_chr(atom));
		break;
	
	case TAG_NUMB:
		fprintf(to, "%ld", get_numb(atom));
		break;

	case TAG_STRG:
		fprintf(to, "%s", get_strg_data(atom));
		break;

	default:
		fprintf(stderr, "Unsupported type in call to print, atom=%lu\n", atom);
		break;
	}
}

scm bltn_newline(void) ALIGN {
	scm port = STACK(0);
	FILE *to;

	if(scm_gettag(port) == ATOM_FLS) {
		to = stdout;
	}
	else {
		to = port_get_file(port);
	}
  
	fprintf(to, "\n");
  
	return ATOM_FLS;
}




//////////////////////////////////////////////////
// equality functions

scm bltn_eq(void) ALIGN {
	if (STACK(0) == STACK(1))
		return ATOM_TRU;
	else
		return ATOM_FLS;
}
scm bltn_equals(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	info_assert(scm_gettag(p) == TAG_NUMB);
	info_assert(scm_gettag(q) == TAG_NUMB);

	if (p == q)
		return ATOM_TRU;
	else
		return ATOM_FLS;
}




//////////////////////////////////////////////////
// arithmetic operators

scm bltn_mul(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	info_assert(scm_gettag(p) == TAG_NUMB);
	info_assert(scm_gettag(q) == TAG_NUMB);
	return mk_numb(get_numb(p) * get_numb(q));
}

scm bltn_div(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	info_assert(scm_gettag(p) == TAG_NUMB);
	info_assert(scm_gettag(q) == TAG_NUMB);
	return mk_numb(get_numb(p) / get_numb(q));
}

scm bltn_add(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	info_assert(scm_gettag(p) == TAG_NUMB);
	info_assert(scm_gettag(q) == TAG_NUMB);
	return mk_numb(get_numb(p) + get_numb(q));
}

scm bltn_sub(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	info_assert(scm_gettag(p) == TAG_NUMB);
	info_assert(scm_gettag(q) == TAG_NUMB);
	return mk_numb(get_numb(p) - get_numb(q));
}

scm bltn_mod(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	info_assert(scm_gettag(p) == TAG_NUMB);
	info_assert(scm_gettag(q) == TAG_NUMB);
	return mk_numb(get_numb(p) % get_numb(q));
}




//////////////////////////////////////////////////
// inequalities

scm bltn_lt(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	info_assert(scm_gettag(p) == TAG_NUMB);
	info_assert(scm_gettag(q) == TAG_NUMB);
	return mk_bool(get_numb(p) < get_numb(q));
}
scm bltn_gt(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	info_assert(scm_gettag(p) == TAG_NUMB);
	info_assert(scm_gettag(q) == TAG_NUMB);
	return mk_bool(get_numb(p) > get_numb(q));
}
scm bltn_le(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	info_assert(scm_gettag(p) == TAG_NUMB);
	info_assert(scm_gettag(q) == TAG_NUMB);
	return mk_bool(get_numb(p) <= get_numb(q));
}
scm bltn_ge(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	info_assert(scm_gettag(p) == TAG_NUMB);
	info_assert(scm_gettag(q) == TAG_NUMB);
	return mk_bool(get_numb(p) >= get_numb(q));
}




//////////////////////////////////////////////////
// mutation

scm bltn_set_car(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	set_cons_car(p, q);
	return ATOM_FLS;
}

scm bltn_set_cdr(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	set_cons_cdr(p, q);
	return ATOM_FLS;
}




//////////////////////////////////////////////////
// vectors

scm bltn_make_vector(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	info_assert(scm_gettag(p) == TAG_NUMB);
	return allocate_vect(get_numb(p), q);
}

scm bltn_vectorq(void) ALIGN {
	return mk_bool(scm_gettag(STACK(0)) == TAG_VECT);
}

scm bltn_vector_ref(void) ALIGN {
	scm vec = STACK(0);
	scm idx = STACK(1);
	info_assert(scm_gettag(idx) == TAG_NUMB);
	info_assert(get_numb(idx) < get_hdr_scm_size(get_vect(STACK(0))[0]));
	return get_vect(vec)[1 + get_numb(idx)];
}

scm bltn_vector_set(void) ALIGN {
	scm vec = STACK(0);
	scm idx = STACK(1);
	scm val = stack[reg_rbp + 3];
	info_assert(scm_gettag(idx) == TAG_NUMB);
	get_vect(vec)[1 + get_numb(idx)] = val;
	return val;
}

scm bltn_vector_length(void) ALIGN {
	return mk_numb(get_hdr_scm_size(get_vect(STACK(0))[0]));
}




//////////////////////////////////////////////////
///// string ones

scm bltn_make_string(void) ALIGN {
	scm args_0 = STACK(0);
	scm args_1 = STACK(1);
	
	char string_tmp_buf[51200] = { 0 };
	int len;

	//assert(bytecode_args_num == 2);

	info_assert(scm_gettag(args_0) == TAG_NUMB);
	info_assert(scm_gettag(args_1) == ATOM_CHR);

	int i;
	char c;

	len = get_numb(args_0);
	c = (char)get_chr(args_1);

	for(i = 0; i < len; i++) {
		string_tmp_buf[i] = c;
	}
	string_tmp_buf[i] = '\0';

	return allocate_strg(string_tmp_buf, len);

}

scm bltn_string_set(void) ALIGN {
	scm args_0 = STACK(0);
	scm args_1 = STACK(1);
	scm args_2 = stack[reg_rbp + 3];
	//  info_assert(args_num == 3);

	info_assert(scm_gettag(args_0) == TAG_STRG);
	info_assert(scm_gettag(args_1) == TAG_NUMB);
	info_assert(scm_gettag(args_2) == ATOM_CHR);

	int i = get_numb(args_1);

	info_assert(i < get_strg_len(args_0));

	get_strg_data(args_0)[i] = get_chr(args_2);

	return 0;
}

scm bltn_string_ref(void) ALIGN {
	scm args_0 = STACK(0);
	scm args_1 = STACK(1);
	//  info_assert(args_num == 2);

	info_assert(scm_gettag(args_0) == TAG_STRG);
	info_assert(scm_gettag(args_1) == TAG_NUMB);

	int i = get_numb(args_1);

	info_assert(i < get_strg_len(args_0));

	return mk_chr(get_strg_data(args_0)[i]);
}

scm bltn_string_length(void) ALIGN {
	scm args_0 = STACK(0);
	info_assert(scm_gettag(args_0) == TAG_STRG);
	return mk_numb(get_strg_len(args_0));
}

scm bltn_string_to_symbol(void) ALIGN {
	scm args_0 = STACK(0);
	//info_assert(bytecode_args_num == 1);

	info_assert(scm_gettag(args_0) == TAG_STRG);

	scm res = symtab_intern((char*)get_strg_data(args_0));
	//printf("SYM:[%s][%lu]\n", get_strg_data(args_0), res);
	return res;
}

scm bltn_string_eql(void) ALIGN {
	scm args_0 = STACK(0);
	scm args_1 = STACK(1);

	info_assert(scm_gettag(args_0) == TAG_STRG);
	info_assert(scm_gettag(args_1) == TAG_STRG);
	
	if(get_strg_len(args_0) != get_strg_len(args_1))
		return ATOM_FLS;

	if(strcmp((char*)get_strg_data(args_0), (char*)get_strg_data(args_1)))
		return ATOM_FLS;
	
	return ATOM_TRU;
}

scm bltn_number_to_char(void) ALIGN {
	return mk_chr(get_numb(STACK(0)));
}
scm bltn_char_to_number(void) ALIGN {
	return mk_numb(get_chr(STACK(0)));
}
scm bltn_symb_to_strn(void) ALIGN {
	char *s = symtab_lookup(STACK(0));
	info_assert(s);
	return allocate_strg(s, strlen(s));
}

scm bltn_strn_append(void) ALIGN {
	scm p = STACK(0);
	scm q = STACK(1);
	scm s = allocate_strg(NULL, get_strg_len(p) + get_strg_len(q));

	memcpy(get_strg_data(s), get_strg_data(p), get_strg_len(p));
	memcpy(get_strg_data(s) + get_strg_len(p), get_strg_data(q), get_strg_len(q));
	get_strg_data(s)[get_strg_len(p) + get_strg_len(q)] = '\0';

	return s;
}




//////////////////////////////////////////////////
// io

scm bltn_eof_objectq(void) ALIGN {
	int r = get_chr(STACK(0)) == -1;
	//	if(r)puts("OEOF");
	return mk_bool(r);
}

scm bltn_read_char(void) ALIGN {
	info_assert(scm_gettag(STACK(0)) == ATOM_PRT);
	char c=fgetc(port_get_file(STACK(0)));
	//		printf("CHR:%c\n", c);
	return mk_chr(c);
}

scm bltn_open_input_file(void) ALIGN {
	info_assert(scm_gettag(STACK(0)) == TAG_STRG);

	FILE *f = fopen((char *)get_strg_data(STACK(0)), "r");
	if(!f) {
		fprintf(stderr, "couldn't open file %s\n", get_strg_data(STACK(0)));
	}
	info_assert(f);
	
	return mk_port(f);
}

scm bltn_open_output_file(void) ALIGN {
	info_assert(scm_gettag(STACK(0)) == TAG_STRG);

	FILE *f = fopen((char *)get_strg_data(STACK(0)), "a");
	if(!f) {
		fprintf(stderr, "couldn't open file %s\n", get_strg_data(STACK(0)));
	}
	info_assert(f);
	
	return mk_port(f);
}

scm bltn_close_port(void) ALIGN {
	info_assert(scm_gettag(STACK(0)) == ATOM_PRT);

	port_close(STACK(0));

	return ATOM_FLS;
}

int fpeek(FILE *stream)
{
	int c;

	c = fgetc(stream);
	ungetc(c, stream);

	return c;
}

scm bltn_peek_char(void) ALIGN {
	info_assert(scm_gettag(STACK(0)) == ATOM_PRT);
	return mk_chr(fpeek(port_get_file(STACK(0))));
}




//////////////////////////////////////////////////
// VM

scm bltn_vm_open(void) ALIGN {
	int fd[2];
	// make a pipe
	info_assert(!pipe(&fd[0]));

	// make a port out of it
	return mk_pipe(fdopen(fd[1], "w"), fdopen(fd[0], "r"));
}

scm bltn_vm_finish(void) ALIGN {
	scm p = STACK(0);
	info_assert(scm_gettag(p) == ATOM_PRT);

	FILE *f1, *f2;

	scm *tmp = vm_code + vm_code_size;

	f1 = port_get_file(p);
	f2 = port_get_pipe_end(p);

	fclose(f1);
	load_code(f2);
	fclose(f2);

	// TODO remove it from the table
	// but dont re-close the fds

	vm_exec(tmp);

	return reg_acc;
}

scm bltn_gensym(void) ALIGN {
	char string_tmp_buf[512] = { 0 };

	info_assert(scm_gettag(STACK(0)) == TAG_STRG);

	snprintf(string_tmp_buf, sizeof(string_tmp_buf), "%s%08x", get_strg_data(STACK(0)), rand()%0xFFFFFFFF);

	return symtab_intern(string_tmp_buf); 
}

static scm parameter_argv;

scm bltn_command_line_arguments(void) ALIGN {
//	printf("CLA %lx\n", parameter_argv); // why is this not printed
	return parameter_argv;
}

//struct timespec bltn_timer_start, bltn_timer_end;
struct timeval bltn_timer_start, bltn_timer_end;
scm bltn_timer(void) ALIGN {
	gettimeofday(&bltn_timer_end, NULL);
//	clock_gettime(CLOCK_MONOTONIC_RAW, &bltn_timer_end);
	
	uint64_t delta_ms
		= (bltn_timer_end.tv_sec - bltn_timer_start.tv_sec) * 1000
		+ (bltn_timer_end.tv_usec - bltn_timer_start.tv_usec) / 1000;
	
	return mk_numb(delta_ms);
}


/////////////////////////////

void builtins_init(scm argv) {
	parameter_argv = argv;

	gettimeofday(&bltn_timer_start, NULL);
//	clock_gettime(CLOCK_MONOTONIC_RAW, &bltn_timer_start);

	// list functions
	glovar_define(symtab_intern("cons"), mk_numb(2), mk_bltn(bltn_cons));
	glovar_define(symtab_intern("car"), mk_numb(1), mk_bltn(bltn_car));
	glovar_define(symtab_intern("cdr"), mk_numb(1), mk_bltn(bltn_cdr));
	glovar_define(symtab_intern("set-car!"), mk_numb(2), mk_bltn(bltn_set_car));
	glovar_define(symtab_intern("set-cdr!"), mk_numb(2), mk_bltn(bltn_set_cdr));
	glovar_define(symtab_intern("null?"), mk_numb(1), mk_bltn(bltn_nullq));
	glovar_define(symtab_intern("pair?"), mk_numb(1), mk_bltn(bltn_pairq));
	glovar_define(symtab_intern("symbol?"), mk_numb(1), mk_bltn(bltn_symbolq));
	glovar_define(symtab_intern("string?"), mk_numb(1), mk_bltn(bltn_stringq));
	glovar_define(symtab_intern("char?"), mk_numb(1), mk_bltn(bltn_charq));
	glovar_define(symtab_intern("boolean?"), mk_numb(1), mk_bltn(bltn_booleanq));
	glovar_define(symtab_intern("number?"), mk_numb(1), mk_bltn(bltn_numberq));
	glovar_define(symtab_intern("procedure?"), mk_numb(1), mk_bltn(bltn_procedureq));

	// printing functions
	glovar_define(symtab_intern("error"), mk_numb(1), mk_bltn(bltn_error));
	glovar_define(symtab_intern("exit"), mk_numb(0), mk_bltn(bltn_exit));
	
	// equality functions
	glovar_define(symtab_intern("eq?"), mk_numb(2), mk_bltn(bltn_eq));
	glovar_define(symtab_intern("="), mk_numb(2), mk_bltn(bltn_equals));

	// arithmetic operators
	glovar_define(symtab_intern("*"), mk_numb(2), mk_bltn(bltn_mul));
	glovar_define(symtab_intern("+"), mk_numb(2), mk_bltn(bltn_add));
	glovar_define(symtab_intern("-"), mk_numb(2), mk_bltn(bltn_sub));
	glovar_define(symtab_intern("modulo"), mk_numb(2), mk_bltn(bltn_mod));
	glovar_define(symtab_intern("quotient"), mk_numb(2), mk_bltn(bltn_div));
	glovar_define(symtab_intern("remainder"), mk_numb(2), mk_bltn(bltn_mod));

	// inequalities
	glovar_define(symtab_intern("<"), mk_numb(2), mk_bltn(bltn_lt));
	glovar_define(symtab_intern(">"), mk_numb(2), mk_bltn(bltn_gt));
	glovar_define(symtab_intern("<="), mk_numb(2), mk_bltn(bltn_le));
	glovar_define(symtab_intern(">="), mk_numb(2), mk_bltn(bltn_ge));

	// vectors
	glovar_define(symtab_intern("make-vector"), mk_numb(2), mk_bltn(bltn_make_vector));
	glovar_define(symtab_intern("vector?"), mk_numb(1), mk_bltn(bltn_vectorq));
	glovar_define(symtab_intern("vector-ref"), mk_numb(2), mk_bltn(bltn_vector_ref));
	glovar_define(symtab_intern("vector-set!"), mk_numb(3), mk_bltn(bltn_vector_set));
	glovar_define(symtab_intern("vector-length"), mk_numb(1), mk_bltn(bltn_vector_length));

	// strings
	glovar_define(symtab_intern("make-string"), mk_numb(2), mk_bltn(bltn_make_string));
	glovar_define(symtab_intern("string-set!"), mk_numb(3), mk_bltn(bltn_string_set));
	glovar_define(symtab_intern("string-ref"), mk_numb(2), mk_bltn(bltn_string_ref));
	glovar_define(symtab_intern("string->symbol"), mk_numb(1), mk_bltn(bltn_string_to_symbol));
	glovar_define(symtab_intern("string-length"), mk_numb(1), mk_bltn(bltn_string_length));
	glovar_define(symtab_intern("string=?"), mk_numb(2), mk_bltn(bltn_string_eql));

	glovar_define(symtab_intern("integer->char"), mk_numb(1), mk_bltn(bltn_number_to_char));
	glovar_define(symtab_intern("char->integer"), mk_numb(1), mk_bltn(bltn_char_to_number));
	glovar_define(symtab_intern("symbol->string"), mk_numb(1), mk_bltn(bltn_symb_to_strn));

	glovar_define(symtab_intern("string-append"), mk_numb(2), mk_bltn(bltn_strn_append));

	// io
	glovar_define(symtab_intern("eof-object?"), mk_numb(1), mk_bltn(bltn_eof_objectq));
	glovar_define(symtab_intern("read-char"), mk_numb(1), mk_bltn(bltn_read_char));
	glovar_define(symtab_intern("peek-char"), mk_numb(1), mk_bltn(bltn_peek_char));
	glovar_define(symtab_intern("open-input-file"), mk_numb(1), mk_bltn(bltn_open_input_file));
	glovar_define(symtab_intern("open-output-file"), mk_numb(1), mk_bltn(bltn_open_output_file));
	glovar_define(symtab_intern("close-port"), mk_numb(1), mk_bltn(bltn_close_port));
	glovar_define(symtab_intern("standard-input"), ATOM_FLS, mk_port(stdin));
	glovar_define(symtab_intern("command-line-arguments"), mk_numb(0), mk_bltn(bltn_command_line_arguments));
	glovar_define(symtab_intern("timer"), mk_numb(0), mk_bltn(bltn_timer));

	// vm
	glovar_define(symtab_intern("vm:open"), mk_numb(0), mk_bltn(bltn_vm_open));
	glovar_define(symtab_intern("vm:finish"), mk_numb(1), mk_bltn(bltn_vm_finish));
	glovar_define(symtab_intern("%display"), mk_numb(2), mk_bltn(bltn_display_port));
	glovar_define(symtab_intern("%newline"), mk_numb(1), mk_bltn(bltn_newline));

	glovar_define(symtab_intern("gensym"), mk_numb(1), mk_bltn(bltn_gensym));
}
