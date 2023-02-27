#ifndef BIGMATH_H
#define BIGMATH_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <limits.h>

#include "utils.h"

/*
 *	Useful macros
 */
#define freearr(ptr)		{if (((void*) ptr != (void*) _zero_val) && ((void*) ptr != (void*) _one_val)) free((void*) ptr);}
#define freeval(big)		{freearr((big).vals); (big).vals=NULL;}
#define copyval(from, to)	memcpy((to).vals, (from).vals, (from).len * sizeof(ul))

#define iszero(big)			(((big).len == 1) && (*(big).vals == 0))
#define isone(big)			(((big).len == 1) && (*(big).vals == 1))
#define isleqone(big)		(((big).len == 1) && (*(big).vals <= 1))

#define digitc(dig)			(((dig <= 9) ? '0' : 55) + dig)

/*
 *	Typedefs
 */
typedef unsigned long ul;
typedef unsigned long long ull;

typedef struct {
	ul* vals;
	long	len;
} BigInt;

/*
 *	Quick way to split uul into high ul and low ul without shifts
 */
typedef union {
	ull value;
	struct {
		ul low;
		ul high;
	} svals;
} BIGUNION;

/*
 *	Constants
 */
extern ul _zero_val[1], _one_val[1];
extern BigInt _zero, _one;
extern jmp_buf exception;


ul* alloc(long len);
void bigcpy(BigInt* from, BigInt* to);

void bigadd(BigInt* a, BigInt* b, BigInt* res);

void bigmul(BigInt* a, BigInt* b, BigInt* res);

void bigdiv(BigInt* a, BigInt* b, BigInt* quo, BigInt* rem);
void bigquo(BigInt* a, BigInt* b, BigInt* res);
void bigmod(BigInt* a, BigInt* b, BigInt* res);

void bigsqr(BigInt* big, BigInt* res);
void bigpow(BigInt* a, BigInt* b, BigInt* res);

void bigtrim(BigInt* big);

void stobig(char* str, ul base, BigInt* result);
void bigprint(BigInt* big, ul base, FILE* result);

void cleanup(void);

#endif /* !BIGMATH_H */