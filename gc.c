#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define TIME_GC
#ifdef TIME_GC
#include <sys/time.h>
#endif

#include "gc.h"

#include "objects.h"
#include "data.h"
#include "interpreter.h"

////
// Allocator

struct list *objects = NULL;

size_t gc_total_mem = 0;

scm
allocate_strg(char *str, scm len)
{
	scm padded_len;
	scm *p;
	char *dst;
	
	padded_len = (len + 1 + 8)/8;
	p = heap_alloc(padded_len + 2);
	
	p[0] = make_hdr(HDR_WHITE, padded_len + 1, 0);
	p[1] = len;
	dst = (void*)(p+2);

	if (str)
		strncpy(dst, str, len);

	return scm_puttag(p, TAG_STRG);
}

scm
allocate_cons(scm car, scm cdr)
{
	scm *p;

	p = heap_alloc(3);
	
	p[0] = make_hdr(HDR_WHITE, 0, 2);
	p[1] = car;
	p[2] = cdr;
	
	return scm_puttag(p, TAG_CONS);
}

scm
allocate_vect(scm len, scm val)
{
	scm *p;
	scm i;

	p = heap_alloc(len + 1);
	
	p[0] = make_hdr(HDR_WHITE, 0, len);
	for (i = 0; i < len; i++) {
		p[1 + i] = val;
	}
	
	return scm_puttag(p, TAG_VECT);
}

scm
allocate_clos(scm *lbl, scm len)
{
	scm *p;

	p = heap_alloc(len + 2);
	
	p[0] = make_hdr(HDR_WHITE, 1, len);
	p[1] = SCM_PTR(lbl);
	// the rest are #f because calloc sets them to 0
	
	return scm_puttag(p, TAG_CLOS);
}

////
// Mark and Sweep Garbage Collector

struct list *gc_objects = NULL;

scm gc_timer = 0;
scm gc_fib = 1000;
scm gc_timeout = 1000;

void gc_list_push(scm *p) {
	struct list *link;

	link = malloc(sizeof(struct list));

	link->object = p;
	link->next = gc_objects;
	if(link->next) {
		link->next->prev = link;
	}
	link->prev = NULL;
  
	gc_objects = link;
}


uint64_t gc_iterations = 0;

#ifdef TIME_GC
uint64_t gc_time_inside = 0;
uint64_t gc_time_outside = 0;
struct timeval gc_timer_tval;
struct timeval gc_timer_tmp;
#endif

void gc_report() {
	printf("GC REPORT: %lu collections\n", gc_iterations);
#ifdef TIME_GC
	printf("GC REPORT: %lu%% of time spent in GC\n", (100*gc_time_inside)/(gc_time_inside + gc_time_outside));
#endif
}


scm *heap_alloc(scm n) {
	scm *p;
	scm tmp;

	gc_timer++;
	if( (gc_timer > gc_timeout)) {
		gc_timer = 0;
    
		tmp = gc_timeout;
		gc_timeout += gc_fib;
		gc_fib = tmp;

#ifdef TIME_GC
		uint64_t delta_ms;

		gettimeofday(&gc_timer_tmp, NULL);
		delta_ms
                	= (gc_timer_tmp.tv_sec - gc_timer_tval.tv_sec) * 1000
                	+ (gc_timer_tmp.tv_usec - gc_timer_tval.tv_usec) / 1000;
		gc_timer_tval = gc_timer_tmp;
		
		if(gc_time_outside == 0) // skip first timing as a hack to avoid having to initialize timevals
			gc_time_outside = 1;
		else
			gc_time_outside += delta_ms;
#endif
    
		gc_iterations++;
		mark();
		sweep();

#ifdef TIME_GC
		gettimeofday(&gc_timer_tmp, NULL);
		delta_ms
                	= (gc_timer_tmp.tv_sec - gc_timer_tval.tv_sec) * 1000
                	+ (gc_timer_tmp.tv_usec - gc_timer_tval.tv_usec) / 1000;
		gc_timer_tval = gc_timer_tmp;

		gc_time_inside += delta_ms;


		gc_report();
#endif
	}

	p = calloc(n, sizeof(scm));
	gc_total_mem += n;
	gc_list_push(p);

	return p;
}

#define GC_PERMAROOT_NUM 10000
scm gc_perma_roots[GC_PERMAROOT_NUM];
int gc_num_perma_roots = 0;
void gc_add_permanent_root(scm obj) {
	gc_perma_roots[gc_num_perma_roots++] = obj;
	if(gc_num_perma_roots > GC_PERMAROOT_NUM) {
		puts("ran out of space for permanent GC roots");
		exit(1);
	}
}

//#define DEBUG
#ifdef DEBUG
#define trace(...) fprintf(stderr, __VA_ARGS__)
#else
#define trace(...)
#endif

void mark() {
	scm gc_stack_ptr;
	scm gc_stack_base_ptr;

	scm tmp;

	struct global *g;

	trace("MARK REG\n");
  
	mark_object(reg_acc);
	mark_object(reg_clo);
	if (reg_env)
		mark_object(scm_puttag(reg_env-2, TAG_CLOS));

	trace("MARK GLO\n");
  
	if(global_variables) {
		for(g = global_variables; g; g = g->next) {
			trace("    GLO <%s>\n", symtab_lookup(g->name));
			mark_object(g->val);
		}
	}
	
	trace("MARK PERMA ROOTS\n");
	for(tmp = 0; tmp < gc_num_perma_roots; tmp++) {
		mark_object(gc_perma_roots[tmp]);
	}

	trace("MARK STK\n");
  
	gc_stack_ptr = reg_rsp;
	gc_stack_base_ptr = reg_rbp;

	if(reg_rbp_tmp)
		trace("MARK REG RBP TMP!\n");

	while(gc_stack_ptr > gc_stack_base_ptr) {
		trace("MARK STK TOP %lu/%lu [%lx]\n", gc_stack_ptr, gc_stack_base_ptr, stack[gc_stack_ptr]);
		mark_object(stack[gc_stack_ptr]);
		gc_stack_ptr--;
	}
  
	trace("MARK STK REAL\n");
	while(gc_stack_ptr > 0) {
		tmp = stack[gc_stack_ptr--];
		assert(tmp == 0xDEADBEEFDEADBEEF);
	  
		// rbp
		gc_stack_base_ptr = stack[gc_stack_ptr--];
	  
		// reg env
		if(stack[gc_stack_ptr])
			mark_object(scm_puttag(((scm*)PTR_SCM(stack[gc_stack_ptr]))-2, TAG_CLOS));
		gc_stack_ptr--;
	  
		// ret addr
		gc_stack_ptr--;
	  
		tmp = stack[gc_stack_ptr--];
		assert(tmp == 0xC0FFEEEEEEEEEEEE);
	  
		trace("	    FRAME\n");
	  
		while(gc_stack_ptr > gc_stack_base_ptr) {
			mark_object(stack[gc_stack_ptr]);
			gc_stack_ptr--;
		}
	}
  
	trace("MARK.\n");
}

void mark_object(scm obj) {
	scm *p;
	scm hdr;

	scm raw_size, scm_size;

	int i;
	if (scm_isptr(obj)) {
		if(scm_gettag(obj) == TAG_BLTN)
			return;

		trace("   MARK(%lx)\n", obj);
		
		p = scm_getptr(obj);
		hdr = p[0];

		if(get_hdr_color(hdr) == HDR_BLACK)
			return;
    
		p[0] = set_hdr_color(hdr, HDR_BLACK);
    
		raw_size = get_hdr_raw_size(hdr);
		scm_size = get_hdr_scm_size(hdr);
    
		for(i = 0; i < scm_size; i++) {
			mark_object(p[1 + raw_size + i]);
		}
	}
}

void sweep() {
	struct list *link;
	struct list *tmp;
	scm hdr;
	size_t mem_size;
	
	link = gc_objects;
	while(link) {
		//printf("%p\n", link->object);
		hdr = *(link->object);

		if(get_hdr_color(hdr) == HDR_WHITE) {
			mem_size = 1 + get_hdr_raw_size(hdr) + get_hdr_scm_size(hdr);
			gc_total_mem -= mem_size;
			bzero(link->object, mem_size * sizeof(scm));
			free(link->object);

			// we saw a white one
			// delete it and remove the link
			tmp = link;
			
			// make our prev point to our next
			// and make our next point to our prev
			if(tmp->prev && tmp->next) {
				tmp->prev->next = tmp->next;
				tmp->next->prev = tmp->prev;

				link = tmp->next;
			}
			else if(tmp->prev && !tmp->next) {
				tmp->prev->next = tmp->next;

				link = NULL;
			}
			else if(!tmp->prev) {
				// exceptional condition:
				// there is no previous link, therefore we are the first link

				// in this case, make gc_objects point to next
				// and make sure it doesn't point back to us
				
				gc_objects = tmp->next;
				if(gc_objects) {
					gc_objects->prev = NULL;
				}

				link = gc_objects;
			}
			
			tmp->object = NULL;
			tmp->next = NULL;
			tmp->prev = NULL;
			free(tmp);
		}
		else {
			// we saw a black one
			// so just make it white and continue looping
			
			*(link->object) = set_hdr_color(hdr, HDR_WHITE);
      
			link = link->next;
		}
	}
}
