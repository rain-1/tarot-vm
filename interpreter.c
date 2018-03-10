#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qcodes.h"
#include "objects.h"
#include "data.h"
#include "gc.h"

scm *interpreter_eip;

void vm_exec(scm *codexxx) {
	interpreter_eip = codexxx;

	static void* lbl[] = {
		[CODE_HALT]		= &&lbl_halt,
		
		[CODE_DATUM_FALSE]	= &&lbl_datum_false,
		[CODE_DATUM_TRUE]	= &&lbl_datum_true,
		[CODE_DATUM_NULL]	= &&lbl_datum_null,
		[CODE_DATUM_SYMBOL]	= &&lbl_datum_symbol,
		[CODE_DATUM_CHAR]	= &&lbl_datum_char,
		[CODE_DATUM_NUMBER]	= &&lbl_datum_number,
		[CODE_DATUM_STRING]	= &&lbl_datum_string,
		
		[CODE_ALLOCATE_CLOSURE] = &&lbl_allocate_closure,
		[CODE_CLOSURE_SET]	= &&lbl_closure_set,
		
		[CODE_VAR_GLO]		= &&lbl_var_glo,
		[CODE_SET_GLO]		= &&lbl_set_glo,
		[CODE_VAR_LOC]		= &&lbl_var_loc,
		[CODE_SET_LOC]		= &&lbl_set_loc,
		[CODE_VAR_ENV]		= &&lbl_var_env,
		[CODE_SET_ENV]		= &&lbl_set_env,	
		[CODE_CLO_SET_ACC]	= &&lbl_clo_set_acc,
		[CODE_CLO_SET_LOC]	= &&lbl_clo_set_loc,
		[CODE_SET_CLO_REG]	= &&lbl_set_clo_reg,
		
		[CODE_JUMP]		= &&lbl_jump,
		[CODE_BRANCH]		= &&lbl_branch,
		[CODE_PUSH]		= &&lbl_push,
		[CODE_STACK_GROW]	= &&lbl_stack_grow,
		[CODE_STACKFRAME]	= &&lbl_stackframe,
		[CODE_CALL]		= &&lbl_call,
		[CODE_RET]		= &&lbl_ret,
		[CODE_SHIFTBACK]	= &&lbl_shiftback,
		
		[CODE_INFORMATION]	= &&lbl_information,
	};
	
	struct global *glo;
	scm idx;
	scm num;
	
#define CODEWORD (*(interpreter_eip++))
#define NEXT goto *lbl[CODEWORD]

// profile
//#define NEXT profile_tick();goto *lbl[CODEWORD]
	
	NEXT;
	
 lbl_halt:
	return;
	
 lbl_datum_false:
	reg_acc = ATOM_FLS;
	NEXT;
 lbl_datum_true:
	reg_acc = ATOM_TRU;
	NEXT;
 lbl_datum_null:
	reg_acc = ATOM_NUL;
	NEXT;
 lbl_datum_symbol:
	reg_acc = CODEWORD;
	NEXT;
 lbl_datum_char:
	reg_acc = CODEWORD;
	NEXT;
 lbl_datum_number:
	reg_acc = mk_numb(CODEWORD);
	NEXT;
 lbl_datum_string:
	reg_acc = CODEWORD;
	NEXT;
	
 lbl_allocate_closure:
	idx = CODEWORD;
	num = CODEWORD;
	reg_clo = allocate_clos(interpreter_eip + num, idx);
	NEXT;
 lbl_closure_set:
	idx = CODEWORD;
	scm_getptr(reg_clo)[2 + idx] = reg_acc;
	NEXT;
	
 lbl_var_glo:
	glo = (void*)CODEWORD;
	reg_acc = glo->val;
	NEXT;
 lbl_set_glo:
	glo = (void*)CODEWORD;
	glo->val = reg_acc;
	NEXT;
 lbl_var_loc:
	idx = CODEWORD;
	reg_acc = stack[reg_rbp + 1 + idx];
	NEXT;
 lbl_set_loc:
	idx = CODEWORD;
	stack[reg_rbp + 1 + idx] = reg_acc;
	NEXT;
 lbl_var_env:
	idx = CODEWORD;
	reg_acc = reg_env[idx];
	NEXT;
 lbl_set_env:
	idx = CODEWORD;
	reg_env[idx] = reg_acc;
	NEXT;
 lbl_clo_set_acc:
	reg_acc = reg_clo;
	NEXT;
 lbl_clo_set_loc:
	idx = CODEWORD;
	stack[reg_rbp + 1 + idx] = reg_clo;
	NEXT;
 lbl_set_clo_reg:
	reg_clo = reg_acc;
	NEXT;

 lbl_jump:
	idx = CODEWORD;
	interpreter_eip += idx;
	NEXT;
 lbl_branch:
	idx = CODEWORD;
	if (reg_acc == ATOM_FLS)
		interpreter_eip += idx;
	NEXT;
 lbl_push:
	stack[++reg_rsp] = reg_acc;
	NEXT;
 lbl_stack_grow:
	//reg_rsp += CODEWORD;

	for(idx = CODEWORD; idx > 0; idx--) {
		stack[++reg_rsp] = 0;
	}
	NEXT;
 lbl_stackframe:
	idx = CODEWORD;
	stack[++reg_rsp] = 0xC0FFEEEEEEEEEEEE;
	stack[++reg_rsp] = (scm)(interpreter_eip + idx);
	stack[++reg_rsp] = (scm)reg_env;
	stack[++reg_rsp] = reg_rbp;
	stack[++reg_rsp] = 0xDEADBEEFDEADBEEF;
	reg_rbp_tmp = reg_rsp;
	NEXT;
 lbl_call:
	reg_rbp = reg_rbp_tmp;
	reg_rbp_tmp = 0;
	switch(scm_gettag(reg_acc)) {
	case TAG_BLTN:
		reg_acc = (get_bltn(reg_acc))();
		goto lbl_ret;
		break;
	case TAG_CLOS:
		interpreter_eip = PTR_SCM(get_clos(reg_acc)[1]);
		reg_env = get_clos(reg_acc)+2;
		break;
	default:
		stack_trace();
		fprintf(stderr, "call: not a closure 0x%lx %ld\n", reg_acc, scm_gettag(reg_acc));
		exit(1);
	}
	NEXT;
 lbl_ret:
	reg_rsp = reg_rbp;
	assert(stack[reg_rsp--] == 0xDEADBEEFDEADBEEF);
	stack[reg_rsp+1] = 0x1337133713371337;
	reg_rbp = stack[reg_rsp--];
	reg_env = PTR_SCM(stack[reg_rsp--]);
	interpreter_eip = PTR_SCM(stack[reg_rsp--]);
	assert(stack[reg_rsp--] == 0xC0FFEEEEEEEEEEEE);
	stack[reg_rsp+1] = 0x1337133713371337;
	NEXT;
 lbl_shiftback:
	num = CODEWORD;
	for(idx = 0; idx < num; idx++)
		stack[reg_rbp + 1 + idx] = stack[reg_rsp - num + 1 + idx];
	reg_rsp = reg_rbp + num + 1;
	reg_rbp_tmp = reg_rbp;
	NEXT;
	
 lbl_information:
	(void)CODEWORD;
	(void)CODEWORD;
	(void)CODEWORD;
	NEXT;
}
