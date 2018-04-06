#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <search.h>

#include "objects.h"
#include "data.h"
#include "interpreter.h"

#define SYMBOL_TABLE_SIZE (1 << 14)

char **symbol_table = NULL;

void vm_init() {
	vm_code = calloc(VM_CODE_SIZE, sizeof(scm));
	stack = calloc(STACKSIZE, sizeof(scm));
	symbol_table = calloc(SYMBOL_TABLE_SIZE, sizeof(char*));
}

////
// VM

scm *vm_code = NULL;
int vm_code_size = 0;


scm *stack = NULL;

scm reg_acc = 0;
scm *reg_env = NULL;
scm reg_clo = 0;

scm reg_rbp = 0;
scm reg_rbp_tmp = 0;

scm reg_rsp = 0;


void vm_add_codeword(scm w)
{
	vm_code[vm_code_size++] = w;

	if(vm_code_size >= VM_CODE_SIZE) {
		fprintf(stderr, "VM_CODE_SIZE\n");
		exit(-1);
	}
}

void vm_dump_code()
{
	for(int i = 0; i < vm_code_size; i++) {
		printf("%p %lu\n", vm_code+i, vm_code[i]);
	}
}


////
// Symbol Table

vm_uint symbol_table_size = 0;

char *symtab_lookup(scm sym) {
	vm_uint idx;
	
	idx = get_sym(sym);
	info_assert(idx < SYMBOL_TABLE_SIZE);
	
	return symbol_table[idx];
}

scm symtab_intern(char *name) {
	vm_uint idx;

	// check if the symbol already exists
	for(idx = 0; idx < symbol_table_size; idx++) {
		if(!strcmp(name, symbol_table[idx])) {
			return mk_sym(idx);
		}
	}

	if(symbol_table_size >= SYMBOL_TABLE_SIZE) {
		fprintf(stderr, "ran out of space for symbols\n");
		exit(1);
	}
	
	// it doesn't, so create it
	idx = symbol_table_size;
	symbol_table[idx] = strdup(name);
	symbol_table_size++;
	
	return mk_sym(idx);
}

////
// Global Variables

struct global *global_variables = NULL;

struct global *glovar_lookup(scm name) {
	struct global *link;

	for(link = global_variables; link; link = link->next) {
//		printf("THE POINTER IS %p\n", link);

		if(link->name == name) {
			return link;
		}
	}
	return NULL;
}

void glovar_define(scm name, vm_uint arity, scm value) {
	struct global *link;
	
	link = glovar_lookup(name);
	if(!link) {
		link = malloc(sizeof(struct global));
	}

	link->name = name;
	link->arity = arity;
	link->val = value;
	link->next = global_variables;
	
	global_variables = link;
}

////
// Ports Table

#define PORTS_TABLE_SIZE 100
port ports_table[PORTS_TABLE_SIZE];

void porttbl_init() {
	vm_uint i;

	for(i = 0; i < PORTS_TABLE_SIZE; i++) {
		ports_table[i] = (port){ .tag = PORT_EMPTY };
	}
}

vm_uint porttbl_alloc() {
	vm_uint i;

	for(i = 0; i < PORTS_TABLE_SIZE; i++) {
		if(ports_table[i].tag == PORT_EMPTY) {
			return i;
		}
	}

	fprintf(stderr, "VM Error: no free ports\n");
	exit(1);
}

// TODO more ports stuff
scm
mk_port(FILE *f)
{
	scm p = porttbl_alloc();

	ports_table[p].tag = PORT_FILE;
	ports_table[p].body.port = f;

	return mk_prt(p);
}

scm
mk_pipe(FILE *f, FILE *g)
{
	scm p = porttbl_alloc();

	ports_table[p].tag = PORT_PIPE;
	ports_table[p].body.pipe[0] = f;
	ports_table[p].body.pipe[1] = g;

	return mk_prt(p);
}

FILE*
port_get_file(scm p)
{
	port f = ports_table[get_prt(p)];
	switch(f.tag) {
	case PORT_EMPTY:
		fprintf(stderr, "Error: Attempted to dereference an empty port\n");
		exit(1);
		return NULL;
	case PORT_FILE:
		return f.body.port;
	case PORT_PIPE:
		return f.body.pipe[0];
	}

	exit(1);
	return NULL;
}

FILE*
port_get_pipe_end(scm p)
{
	port f = ports_table[get_prt(p)];
	assert(f.tag == PORT_PIPE);
	return f.body.pipe[1];
}

int
port_empty(scm p)
{
	port f = ports_table[get_prt(p)];
	return (f.tag == PORT_EMPTY);
}


void
port_close(scm s)
{
	scm i = get_prt(s);
	port p = ports_table[i];
	
	switch (p.tag) {
	case PORT_EMPTY:
		fprintf(stderr, "Error: attempted to close an empty port\n");
		exit(1);
		break;
	case PORT_FILE:
		fclose(p.body.port);
		break;
	case PORT_PIPE:
		fclose(p.body.pipe[0]);
		fclose(p.body.pipe[1]);
		break;
	}
	
	ports_table[i].tag = PORT_EMPTY;
}


////
// Information

struct info_list {
	scm *addr;
	scm *addr2;
	char *info;
	
	struct info_list *next;
};

struct info_list *info_all;

struct info_list *info_findplace(struct info_list *l, scm *addr) {
	if(l == NULL)
		return NULL;
	
	if(addr >= l->addr)
		return NULL;
	
	if(!l->next)
		return l;
	
	if(addr >= l->next->addr)
		return l;
	
	return info_findplace(l->next, addr);
}

void information_store(scm *addr, scm *addr2, char *info)
{
	struct info_list *i;
	struct info_list *place;

	i = malloc(sizeof(struct info_list));
	i->addr = addr;
	i->addr2 = addr2;
	assert(addr < addr2);
	i->info = strdup(info);
	i->next = NULL;

//printf("stored information %s %p %p\n", info, addr, addr2);
	
	place = info_findplace(info_all, addr);
	if(!place) {
		i->next = info_all;
		info_all = i;
	}
	else {
		// place is the last node below this node
		
		if(place->next)
			i->next = place->next->next;
		place->next = i;
	}
}

char *information_lookup(scm *addr)
{
	struct info_list *l = info_findplace(info_all, addr);

	if(!l)
		return "***";
	
	if(!(l->next))
		return "***";
	
	l = l->next;
	
	if(l->addr <= addr && addr <= l->addr2)
		return l->info;

//	printf("FAIL @ %s %p %p %p\n", l->info, l->addr, addr, l->addr2);
	
	return "???";
}

void stack_trace()
{
	scm tmp;
	scm gc_stack_ptr;
	scm gc_stack_base_ptr;
	scm *ret_addr;
	
	fprintf(stderr, "=======================\n");
	fprintf(stderr, "STACK TRACE\n");
	
	gc_stack_ptr = reg_rsp;
	gc_stack_base_ptr = reg_rbp;
	
	gc_stack_ptr = gc_stack_base_ptr;
	while(gc_stack_ptr > 0) {
		tmp = stack[gc_stack_ptr--];
		assert(tmp == 0xDEADBEEFDEADBEEF);

		// rbp
		gc_stack_base_ptr = stack[gc_stack_ptr--];
		
		// reg env
		gc_stack_ptr--;

		// ret addr
		ret_addr = PTR_SCM(stack[gc_stack_ptr--]);
		fprintf(stderr, "%s\n", information_lookup(ret_addr));
		
		tmp = stack[gc_stack_ptr--];
		assert(tmp == 0xC0FFEEEEEEEEEEEE);
		
		gc_stack_ptr = gc_stack_base_ptr;
	}
	
	fprintf(stderr, "=======================\n\n");
}

char *stack_probe()
{
	return information_lookup(interpreter_eip);
}

////
// Profiling

int profiling_value[2000] = { 0 };
int profiling_value_used = 0;

void profile_init() {
	hcreate(2000);
}

static uint64_t profile_ticks = 0;

void profile_tick() {
	ENTRY e, *ep;
	
	static int tick = 0;

	profile_ticks++;
	
	// only sample every 300 steps
	++tick;
	tick = tick % 300;
	if(tick) return;

	e.key = stack_probe();
	e.data = NULL;

	ep = hsearch(e, FIND);
	if(ep) {
		int *p = ((int*)ep->data);
		++*p;
	}
	else {
		e.data = &(profiling_value[profiling_value_used]);
		profiling_value_used++;

		hsearch(e, ENTER);
	}
}


void profile_output() {
	ENTRY e, *ep;

//puts("PROFIL OUTP>UT");
	for (struct info_list *l = info_all; l; l = l->next) {
//printf("keY $%s\n", l->info);
		e.key = l->info;
		e.data = 0;
		
	        ep = hsearch(e, FIND);
	        if(ep && ep->data) {
			printf("%d - %s\n", *((int*)ep->data), e.key);
		}
	}
}
