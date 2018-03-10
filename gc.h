#include "objects.h"

typedef enum {
        HDR_WHITE = 0,
	HDR_BLACK = 1,
} hdr_color;

static inline scm
make_hdr(hdr_color col,
	 scm raw_size,
	 scm scm_size)
{
	info_assert(scm_size <= 0xFFFFFF);
	info_assert(raw_size <= 0xFFFFFFFF);

	return col
		| (scm_size << 8)
		| (raw_size << 32);
}

static inline hdr_color
get_hdr_color(scm o)
{
	return o & 0xFF;
}

static inline scm
get_hdr_scm_size(scm o) {
	return (o >> 8) & 0xFFFFFF;
}

static inline scm
get_hdr_raw_size(scm o)
{
	return (o >> 32) & 0xFFFFFFFF;
}

static inline scm
set_hdr_color(scm o,
	      hdr_color col)
{
	return col
		| (o & ~0xFF);
}

struct list {
	// object is a pointer into the heap
	// object[0] will be the GC header
	// object[1..] is the data
	
	scm *object;
	
	struct list *next;
	struct list *prev;
};

extern struct list *objects;

void objects_push(scm *obj);

scm *allocate(scm len);
scm allocate_strg(char *str, scm len);
scm allocate_cons(scm car, scm cdr);
scm allocate_vect(scm len, scm val);
scm allocate_clos(scm *lbl, scm len);

void gc_report();
void gc_add_permanent_root(scm obj);

void mark();
void mark_object(scm obj);
void sweep();

scm *heap_alloc(scm n);
scm heap_alloc_closure(scm n, scm lbl);
scm heap_alloc_cons(scm car, scm cdr);
scm heap_alloc_vect(scm n, scm val);
scm heap_alloc_strn(char *s, int i);
scm heap_alloc_port(FILE *a, FILE *b);
