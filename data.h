#include <assert.h>

// VM
#define VM_CODE_SIZE (1<<20)

extern scm *vm_code;
extern int vm_code_size;


#define STACKSIZE (1<<27)
// 1<<13 = 8192

extern scm *stack;

extern scm reg_acc;
extern scm *reg_env;
extern scm reg_clo;

extern scm reg_rbp;
extern scm reg_rbp_tmp;

extern scm reg_rsp;

void vm_init();
void vm_add_codeword(scm w);
void vm_dump_code();



// Symbol Table
char *symtab_lookup(scm sym);
scm symtab_intern(char *name);


// Globals
struct global {
	scm val;
	scm name;
	scm arity;
	
	struct global *next;
};
extern struct global *global_variables;

struct global *glovar_lookup(scm name);
void glovar_define(scm name, vm_uint arity, scm value);


// Ports
typedef enum {
	PORT_EMPTY = 0,
	PORT_FILE = 1,
	PORT_PIPE = 2
} port_tag;

typedef struct port {
	port_tag tag;
	union {
		FILE* port;
		FILE* pipe[2];
	} body;
} port;

void porttbl_init();
scm porttbl_alloc(void);
scm mk_port(FILE *f);
scm mk_pipe(FILE *f, FILE *g);
FILE* port_get_file(scm p);
FILE* port_get_pipe_end(scm p);
int port_empty(scm p);
void port_close(scm p);


// Information
void information_store(scm *addr, scm *addr2, char *info);
void stack_trace();
char *stack_probe();

// Profiling
void profile_init();
void profile_tick();
void profile_output();
