/*	$Id: common.c,v 1.30 2003/07/07 20:27:07 ragge Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "pass1.h"

# ifndef EXIT
# define EXIT exit
# endif

int nerrors = 0;  /* number of errors */

#ifndef WHERE
#define	WHERE(ch) fprintf(stderr, "%s, line %d: ", ftitle, lineno);
#endif

/*
 * nonfatal error message
 * the routine where is different for pass 1 and pass 2;
 * it tells where the error took place
 */
void
uerror(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	++nerrors;
	WHERE('u');
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
	if (nerrors > 30)
		cerror("too many errors");
	va_end(ap);
}

/*
 * compiler error: die
 */
void
cerror(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	WHERE('c');

	/* give the compiler the benefit of the doubt */
	if (nerrors && nerrors <= 30) {
		fprintf(stderr,
		    "cannot recover from earlier errors: goodbye!\n");
	} else {
		fprintf(stderr, "compiler error: ");
		vfprintf(stderr, s, ap);
		fprintf(stderr, "\n");
	}
	va_end(ap);
	EXIT(1);
}

/*
 * warning
 */
void
werror(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	WHERE('w');
	fprintf(stderr, "warning: ");
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
}

static NODE *freelink;
static int usednodes;

NODE *
talloc()
{
	extern int inlnodecnt, recovernodes;
	register NODE *p;

	if ((usednodes++ - inlnodecnt) > TREESZ)
		cerror("out of tree space; usednodes %d inlnodecnt %d",
		    usednodes, inlnodecnt);

	if (recovernodes)
		inlnodecnt++;
	if (freelink != NULL) {
		p = freelink;
		freelink = p->next;
		if (p->n_op != FREE)
			cerror("node not FREE: %p", p);
		if (nflag)
			printf("alloc node %p from freelist\n", p);
		return p;
	}

	p = permalloc(sizeof(NODE));
	p->n_op = FREE;
	if (nflag)
		printf("alloc node %p from memory\n", p);
	return p;
}

/*
 * ensure that all nodes have been freed
 */
void
tcheck()
{
	extern int inlnodecnt;

	if (nerrors)
		return;

	if ((usednodes - inlnodecnt) != 0)
		cerror("usednodes == %d, inlnodecnt %d", usednodes, inlnodecnt);
}

/*
 * free the tree p
 */
void
tfree(NODE *p)
{
	if (p->n_op != FREE)
		walkf(p, nfree);
}

void
nfree(NODE *p)
{
	extern int inlnodecnt, recovernodes;
//	NODE *q;

	if (p != NULL) {
		if (p->n_op == FREE)
			cerror("freeing FREE node", p);
#if 0
		q = freelink;
		while (q != NULL) {
			if (q == p)
				cerror("freeing free node %p", p);
			q = q->next;
		}
#endif

		if (nflag)
			printf("freeing node %p\n", p);
		p->n_op = FREE;
		p->next = freelink;
		freelink = p;
		usednodes--;
		if (recovernodes)
			inlnodecnt--;
	} else
		cerror("freeing blank node!");
}

void
fwalk(NODE *t, int (*f)(NODE *, int, int *, int *), int down)
{

	int down1, down2;

	more:
	down1 = down2 = 0;

	(*f)(t, down, &down1, &down2);

	switch (optype( t->n_op )) {

	case BITYPE:
		fwalk( t->n_left, f, down1 );
		t = t->n_right;
		down = down2;
		goto more;

	case UTYPE:
		t = t->n_left;
		down = down1;
		goto more;

	}
}

void
walkf(NODE *t, void (*f)(NODE *))
{
	int opty;

	opty = optype(t->n_op);

	if (opty != LTYPE)
		walkf( t->n_left, f );
	if (opty == BITYPE)
		walkf( t->n_right, f );
	(*f)(t);
}

int dope[DSIZE];
char *opst[DSIZE];

struct dopest {
	int dopeop;
	char opst[8];
	int dopeval;
} indope[] = {
	{ NAME, "NAME", LTYPE, },
	{ STRING, "STRING", LTYPE, },
	{ REG, "REG", LTYPE, },
	{ OREG, "OREG", LTYPE, },
	{ ICON, "ICON", LTYPE, },
	{ FCON, "FCON", LTYPE, },
	{ CCODES, "CCODES", LTYPE, },
	{ UNARY MINUS, "U-", UTYPE, },
	{ UNARY MUL, "U*", UTYPE, },
	{ UNARY AND, "U&", UTYPE, },
	{ UNARY CALL, "UCALL", UTYPE|CALLFLG, },
	{ UNARY FORTCALL, "UFCALL", UTYPE|CALLFLG, },
	{ NOT, "!", UTYPE|LOGFLG, },
	{ COMPL, "~", UTYPE, },
	{ FORCE, "FORCE", UTYPE, },
	{ INIT, "INIT", UTYPE, },
	{ SCONV, "SCONV", UTYPE, },
	{ PCONV, "PCONV", UTYPE, },
	{ PLUS, "+", BITYPE|FLOFLG|SIMPFLG|COMMFLG, },
	{ ASG PLUS, "+=", BITYPE|ASGFLG|ASGOPFLG|FLOFLG|SIMPFLG|COMMFLG, },
	{ MINUS, "-", BITYPE|FLOFLG|SIMPFLG, },
	{ ASG MINUS, "-=", BITYPE|FLOFLG|SIMPFLG|ASGFLG|ASGOPFLG, },
	{ MUL, "*", BITYPE|FLOFLG|MULFLG, },
	{ ASG MUL, "*=", BITYPE|FLOFLG|MULFLG|ASGFLG|ASGOPFLG, },
	{ AND, "&", BITYPE|SIMPFLG|COMMFLG, },
	{ ASG AND, "&=", BITYPE|SIMPFLG|COMMFLG|ASGFLG|ASGOPFLG, },
	{ QUEST, "?", BITYPE, },
	{ COLON, ":", BITYPE, },
	{ ANDAND, "&&", BITYPE|LOGFLG, },
	{ OROR, "||", BITYPE|LOGFLG, },
	{ CM, ",", BITYPE, },
	{ COMOP, ",OP", BITYPE, },
	{ ASSIGN, "=", BITYPE|ASGFLG, },
	{ DIV, "/", BITYPE|FLOFLG|MULFLG|DIVFLG, },
	{ ASG DIV, "/=", BITYPE|FLOFLG|MULFLG|DIVFLG|ASGFLG|ASGOPFLG, },
	{ MOD, "%", BITYPE|DIVFLG, },
	{ ASG MOD, "%=", BITYPE|DIVFLG|ASGFLG|ASGOPFLG, },
	{ LS, "<<", BITYPE|SHFFLG, },
	{ ASG LS, "<<=", BITYPE|SHFFLG|ASGFLG|ASGOPFLG, },
	{ RS, ">>", BITYPE|SHFFLG, },
	{ ASG RS, ">>=", BITYPE|SHFFLG|ASGFLG|ASGOPFLG, },
	{ OR, "|", BITYPE|COMMFLG|SIMPFLG, },
	{ ASG OR, "|=", BITYPE|COMMFLG|SIMPFLG|ASGFLG|ASGOPFLG, },
	{ ER, "^", BITYPE|COMMFLG|SIMPFLG, },
	{ ASG ER, "^=", BITYPE|COMMFLG|SIMPFLG|ASGFLG|ASGOPFLG, },
	{ INCR, "++", BITYPE|ASGFLG, },
	{ DECR, "--", BITYPE|ASGFLG, },
	{ STREF, "->", BITYPE, },
	{ CALL, "CALL", BITYPE|CALLFLG, },
	{ FORTCALL, "FCALL", BITYPE|CALLFLG, },
	{ EQ, "==", BITYPE|LOGFLG, },
	{ NE, "!=", BITYPE|LOGFLG, },
	{ LE, "<=", BITYPE|LOGFLG, },
	{ LT, "<", BITYPE|LOGFLG, },
	{ GE, ">", BITYPE|LOGFLG, },
	{ GT, ">", BITYPE|LOGFLG, },
	{ UGT, "UGT", BITYPE|LOGFLG, },
	{ UGE, "UGE", BITYPE|LOGFLG, },
	{ ULT, "ULT", BITYPE|LOGFLG, },
	{ ULE, "ULE", BITYPE|LOGFLG, },
	{ TYPE, "TYPE", LTYPE, },
	{ LB, "[", BITYPE, },
	{ CBRANCH, "CBRANCH", BITYPE, },
	{ FLD, "FLD", UTYPE, },
	{ PMCONV, "PMCONV", BITYPE, },
	{ PVCONV, "PVCONV", BITYPE, },
	{ RETURN, "RETURN", BITYPE|ASGFLG|ASGOPFLG, },
	{ CAST, "CAST", BITYPE|ASGFLG|ASGOPFLG, },
	{ GOTO, "GOTO", UTYPE, },
	{ STASG, "STASG", BITYPE|ASGFLG, },
	{ STARG, "STARG", UTYPE, },
	{ STCALL, "STCALL", BITYPE|CALLFLG, },
	{ UNARY STCALL, "USTCALL", UTYPE|CALLFLG, },

	{ -1,	"",	0 },
};

void
mkdope()
{
	struct dopest *q;

	for( q = indope; q->dopeop >= 0; ++q ){
		dope[q->dopeop] = q->dopeval;
		opst[q->dopeop] = q->opst;
	}
}

/*
 * output a nice description of the type of t
 */
void
tprint(TWORD t)
{
	static char * tnames[] = {
		"undef",
		"farg",
		"char",
		"uchar",
		"short",
		"ushort",
		"int",
		"unsigned",
		"long",
		"ulong",
		"longlong",
		"ulonglong",
		"float",
		"double",
		"ldouble",
		"strty",
		"unionty",
		"enumty",
		"moety",
		"void",
		"?", "?"
		};

	for(;; t = DECREF(t) ){

		if (ISPTR(t))
			printf("PTR ");
		else if (ISFTN(t))
			printf("FTN ");
		else if (ISARY(t))
			printf("ARY ");
		else {
			printf("%s", tnames[t]);
			return;
		}
	}
}

/*
 * Return a number for internal labels.
 */
int 
getlab()
{
        static int crslab = 10;
	return crslab++;
}

/*
 * Memory allocation routines.
 * Memory are allocated from the system in MEMCHUNKSZ blocks.
 * permalloc() returns a bunch of memory that is never freed.
 * Memory allocated through tmpalloc() will be released the
 * next time a function is ended (via tmpfree()).
 */

#define	MEMCHUNKSZ 8192	/* 8k per allocation */
#define	ROUNDUP(x) ((x) + (sizeof(int)-1)) & ~(sizeof(int)-1)

static char *allocpole;
static int allocleft;
static char *tmppole;
static int tmpleft;
int permallocsize, tmpallocsize, lostmem;

void *
permalloc(int size)
{
	void *rv;

//printf("permalloc: allocpole %p allocleft %d size %d ", allocpole, allocleft, size);
	if (size > MEMCHUNKSZ)
		cerror("permalloc");
	if (size <= 0)
		cerror("permalloc2");
	if (allocleft < size) {
		/* looses unused bytes */
		lostmem += allocleft;
//fprintf(stderr, "allocating perm\n");
		if ((allocpole = malloc(MEMCHUNKSZ)) == NULL)
			cerror("permalloc: out of memory");
		allocleft = MEMCHUNKSZ;
	}
	size = ROUNDUP(size);
	rv = &allocpole[MEMCHUNKSZ-allocleft];
//printf("rv %p\n", rv);
	allocleft -= size;
	permallocsize += size;
	return rv;
}

static char *tmplink;

void *
tmpalloc(int size)
{
	void *rv;

	if (size > MEMCHUNKSZ)
		cerror("tmpalloc");
	if (size <= 0)
		cerror("tmpalloc2");
//printf("tmpalloc: tmppole %p tmpleft %d size %d ", tmppole, tmpleft, size);
	if (tmpleft < size) {
		if ((tmppole = malloc(MEMCHUNKSZ)) == NULL)
			cerror("tmpalloc: out of memory");
//fprintf(stderr, "allocating tmp\n");
		tmpleft = MEMCHUNKSZ - sizeof(char *);
		*(char **)tmppole = tmplink;
		tmplink = tmppole;
	}
	size = ROUNDUP(size);
	rv = &tmppole[MEMCHUNKSZ-tmpleft];
//printf("rv %p\n", rv);
	tmpleft -= size;
	tmpallocsize += size;
	return rv;
}

void
tmpfree()
{
	char *f, *of;

	f = tmplink;
	if (f == NULL)
		return;
	if (*(char **)f == NULL) {
		tmpleft = MEMCHUNKSZ - sizeof(char *);
		return;
	}
	while (f != NULL) {
		of = f;
		f = *(char **)f;
		free(of);
	}
	tmplink = tmppole = NULL;
	tmpleft = 0;
//fprintf(stderr, "freeing tmp\n");
	/* XXX - nothing right now */
}

/*
 * Allocate space on the permanent stack for a string of length len+1
 * and copy it there.
 * Return the new address.
 */
char *
newstring(char *s, int len)
{
	char *u, *c;

	len++;
	if (allocleft < len) {
		u = c = permalloc(len);
	} else {
		u = c = &allocpole[MEMCHUNKSZ-allocleft];
		allocleft -= ROUNDUP(len+1);
	}
	while (len--)
		*c++ = *s++;
	return u;
}
