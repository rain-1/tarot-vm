#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

void stack_trace();
#define info_assert(cond) do { \
		if(!(cond)) { \
			stack_trace(); \
			assert(cond); \
		} \
	} while(0)

typedef uint64_t scm;
typedef uint64_t vm_uint;
typedef int64_t num;

#define SCM_PTR(p) ((scm)((void*)(p)))
#define PTR_SCM(s) ((void*)(s))

typedef enum {
	TAG_ATOM = 0b000,
	TAG_NUMB = 0b100,
	// 010
	TAG_BLTN = 0b110,
	TAG_STRG = 0b001,
	TAG_CONS = 0b101,
	TAG_VECT = 0b011,
	TAG_CLOS = 0b111,
} scm_tag;

typedef enum {
	ATOM_FLS = 0b000000,
	ATOM_TRU = 0b100000,
	ATOM_NUL = 0b010000,
	ATOM_SYM = 0b110000,
	ATOM_CHR = 0b001000,
	ATOM_PRT = 0b101000,
	// 011
	// 111
	//---
	//---
} atom_tag;

static inline scm
scm_isptr(scm o)
{
	return (o & 0b11);
}

static inline scm*
scm_getptr(scm o)
{
	info_assert(scm_isptr(o));
	return (scm*)(o & ~0b111);
}

static inline scm
scm_gettag(scm o)
{
	// return the 3 tag bits
	// ..or if they are zero
	// return all 6 of the atom tag bits
	
	return (o & 0b111) ?
		(o & 0b111) :
		(o & 0b111111);
}

static inline scm
scm_puttag(scm *p, scm tag)
{
	return tag | ((scm)p);
}

// special object helpers

static inline scm
mk_bool(int b)
{
	return b ? ATOM_TRU : ATOM_FLS;
}

static inline scm
mk_numb(num raw)
{
	return (raw << 3)
		| TAG_NUMB;
}

static inline num
get_numb(scm obj)
{
	info_assert((obj & 0b111) == TAG_NUMB);
	return ((num)obj) >> 3;
}

static inline scm
mk_chr(int c)
{
	return (c << 6)
		| ATOM_CHR;
}

static inline int
get_chr(scm obj)
{
	info_assert((obj & 0b111111) == ATOM_CHR);
	return (obj >> 6);
}

static inline scm
mk_prt(scm c)
{
	return (c << 6)
		| ATOM_PRT;
}

static inline scm
get_prt(scm obj)
{
	info_assert((obj & 0b111111) == ATOM_PRT);
	return ((scm)obj >> 6);
}

static inline scm
mk_sym(scm c)
{
	return (c << 6)
		| ATOM_SYM;
}

static inline scm
get_sym(scm obj)
{
	info_assert((obj & 0b111111) == ATOM_SYM);
	return (obj >> 6);
}

static inline scm*
get_strg(scm s)
{
	info_assert(scm_gettag(s) == TAG_STRG);
	return (scm*)(s & ~0b111);
}

static inline scm
get_strg_len(scm s)
{
	scm *p;
	p = get_strg(s);
	return p[1];
}

static inline unsigned char *
get_strg_data(scm s)
{
	scm *p;
	p = get_strg(s);
	return (void*)(p+2);
}

static inline scm
get_cons_car(scm s)
{
	info_assert(scm_gettag(s) == TAG_CONS);
	scm *p = scm_getptr(s);
	return p[1];
}

static inline scm
get_cons_cdr(scm s)
{
	info_assert(scm_gettag(s) == TAG_CONS);
	scm *p = scm_getptr(s);
	return p[2];
}

static inline scm
set_cons_car(scm s, scm x)
{
	info_assert(scm_gettag(s) == TAG_CONS);
	scm *p = scm_getptr(s);
	return p[2] = x;
}

static inline scm
set_cons_cdr(scm s, scm x)
{
	info_assert(scm_gettag(s) == TAG_CONS);
	scm *p = scm_getptr(s);
	return p[2] = x;
}


static inline scm*
get_vect(scm v)
{
	info_assert(scm_gettag(v) == TAG_VECT);
	return scm_getptr(v);
}

// TODO: nicer way to do builtins
typedef scm (*bltn)(void);

static inline scm
mk_bltn(bltn f)
{
	return SCM_PTR(f) | TAG_BLTN;
}

static inline bltn
get_bltn(scm s)
{
	info_assert(scm_gettag(s) == TAG_BLTN);
	return (void*)scm_getptr(s);
}

static inline scm*
get_clos(scm s)
{
	info_assert(scm_gettag(s) == TAG_CLOS);
	return scm_getptr(s);
}

#endif
