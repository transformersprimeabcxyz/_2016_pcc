/*      $Id: match.c,v 1.66 2006/05/26 06:21:32 ragge Exp $   */
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

# include "pass2.h"

#include <strings.h>

void prttype(int t);
void setclass(int tmp, int class);
int getclass(int tmp);

int fldsz, fldshf;

int s2debug = 0;

extern char *ltyp[], *rtyp[];

static char *srtyp[] = { "SRNOPE", "SRDIR", "SROREG", "SRREG" };

/*
 * return true if shape is appropriate for the node p
 * side effect for SFLD is to set up fldsz, etc
 *
 * Return values:
 * SRNOPE  Cannot match this shape.
 * SRDIR   Direct match, may or may not traverse down.
 * SRREG   Will match if put in a regster XXX - kill this?
 */
int
tshape(NODE *p, int shape)
{
	int o, mask;

	o = p->n_op;

#ifdef PCC_DEBUG
	if (s2debug)
		printf("tshape(%p, %s) op = %s\n", p, prcook(shape), opst[o]);
#endif

	if (shape & SPECIAL) {

		switch (shape) {
		case SZERO:
		case SONE:
		case SMONE:
		case SSCON:
		case SCCON:
			if (o != ICON || p->n_name[0])
				return SRNOPE;
			if (p->n_lval == 0 && shape == SZERO)
				return SRDIR;
			if (p->n_lval == 1 && shape == SONE)
				return SRDIR;
			if (p->n_lval == -1 && shape == SMONE)
				return SRDIR;
			if (p->n_lval > -257 && p->n_lval < 256 &&
			    shape == SCCON)
				return SRDIR;
			if (p->n_lval > -32769 && p->n_lval < 32768 &&
			    shape == SSCON)
				return SRDIR;
			return SRNOPE;

		case SSOREG:	/* non-indexed OREG */
			if (o == OREG && !R2TEST(p->n_rval))
				return SRDIR;
			return SRNOPE;

		default:
			return (special(p, shape));
		}
	}

	if (shape & SANY)
		return SRDIR;

	if ((shape&INTEMP) && shtemp(p)) /* XXX remove? */
		return SRDIR;

	if ((shape&SWADD) && (o==NAME||o==OREG))
		if (BYTEOFF(p->n_lval))
			return SRNOPE;

	switch (o) {

	case NAME:
		if (shape & SNAME)
			return SRDIR;
		break;

	case ICON:
		if (shape & SCON)
			return SRDIR;
		break;

	case FLD:
		if (shape & SFLD) {
			int sh;

			if ((sh = flshape(p->n_left)) == SRNOPE)
				return sh;
			/* it is a FIELD shape; make side-effects */
			/* XXX - this will not work for multi-matches */
			o = p->n_rval;
			fldsz = UPKFSZ(o);
# ifdef RTOLBYTES
			fldshf = UPKFOFF(o);
# else
			fldshf = SZINT - fldsz - UPKFOFF(o);
# endif
			return sh;
		}
		break;

	case CCODES:
		if (shape & SCC)
			return SRDIR;
		break;

	case REG:
#if 0
		if (p->n_rval == FPREG || p->n_rval == STKREG)
			break; /* XXX Fix when exclusion nodes are removed */
		/* FALLTHROUGH */
#endif
	case TEMP:
		mask = PCLASS(p);
		if (shape & mask)
			return SRDIR;
#ifdef ragge
		break;
#else
		return SRREG;
#endif

	case OREG:
		if (shape & SOREG)
			return SRDIR;
		break;

	case UMUL:
		/* may end up here if TEMPs involved */
		if (oregok(p, 0) && (shape & SOREG))	/* XXX fixa! */
			return SRDIR; /* converted early */
#ifdef ragge
		if (shumul(p->n_left) & shape)
			return SRDIR;	/* Call offstar to do an OREG */
#else
		if (shumul(p->n_left) & shape)
			return SROREG;	/* Call offstar to do an OREG */
#endif
		break;

	}
#ifndef ragge
	if (shape & PCLASS(p))
		return SRREG;	/* Can put in register XXX check this */
#endif

	return SRNOPE;
}

/*
 * does the type t match tword
 */
int
ttype(TWORD t, int tword)
{
	if (tword & TANY)
		return(1);

#ifdef PCC_DEBUG
	if (t2debug)
		printf("ttype(%o, %o)\n", t, tword);
#endif
	if (ISPTR(t) && ISFTN(DECREF(t)) && (tword & TFTN)) {
		/* For funny function pointers */
		return 1;
	}
	if (ISPTR(t) && (tword&TPTRTO)) {
		do {
			t = DECREF(t);
		} while (ISARY(t));
			/* arrays that are left are usually only
			 * in structure references...
			 */
		return (ttype(t, tword&(~TPTRTO)));
	}
	if (t != BTYPE(t))
		return (tword & TPOINT); /* TPOINT means not simple! */
	if (tword & TPTRTO)
		return(0);

	switch (t) {
	case CHAR:
		return( tword & TCHAR );
	case SHORT:
		return( tword & TSHORT );
	case STRTY:
	case UNIONTY:
		return( tword & TSTRUCT );
	case INT:
		return( tword & TINT );
	case UNSIGNED:
		return( tword & TUNSIGNED );
	case USHORT:
		return( tword & TUSHORT );
	case UCHAR:
		return( tword & TUCHAR );
	case ULONG:
		return( tword & TULONG );
	case LONG:
		return( tword & TLONG );
	case LONGLONG:
		return( tword & TLONGLONG );
	case ULONGLONG:
		return( tword & TULONGLONG );
	case FLOAT:
		return( tword & TFLOAT );
	case DOUBLE:
		return( tword & TDOUBLE );
	case LDOUBLE:
		return( tword & TLDOUBLE );
	}

	return(0);
}

/*
 * generate code by interpreting table entry
 */
void
expand(NODE *p, int cookie, char *cp)
{
	CONSZ val;

	for( ; *cp; ++cp ){
		switch( *cp ){

		default:
			PUTCHAR( *cp );
			continue;  /* this is the usual case... */

		case 'Z':  /* special machine dependent operations */
			zzzcode( p, *++cp );
			continue;

		case 'F':  /* this line deleted if FOREFF is active */
			if( cookie & FOREFF ) while( *++cp != '\n' ) ; /* VOID */
			continue;

		case 'S':  /* field size */
			printf( "%d", fldsz );
			continue;

		case 'H':  /* field shift */
			printf( "%d", fldshf );
			continue;

		case 'M':  /* field mask */
		case 'N':  /* complement of field mask */
			val = 1;
			val <<= fldsz;
			--val;
			val <<= fldshf;
			adrcon( *cp=='M' ? val : ~val );
			continue;

		case 'L':  /* output special label field */
			if (*++cp == 'C')
				printf(LABFMT, p->n_label);
			else
				printf(LABFMT, (int)getlr(p,*cp)->n_lval);
			continue;

		case 'O':  /* opcode string */
			hopcode( *++cp, p->n_op );
			continue;

		case 'B':  /* byte offset in word */
			val = getlr(p,*++cp)->n_lval;
			val = BYTEOFF(val);
			printf( CONFMT, val );
			continue;

		case 'C': /* for constant value only */
			conput(stdout, getlr( p, *++cp ) );
			continue;

		case 'I': /* in instruction */
			insput( getlr( p, *++cp ) );
			continue;

		case 'A': /* address of */
			adrput(stdout, getlr( p, *++cp ) );
			continue;

		case 'U': /* for upper half of address, only */
			upput(getlr(p, *++cp), SZLONG);
			continue;

			}

		}

	}

NODE resc[4];

NODE *
getlr(NODE *p, int c)
{
	NODE *q;

	/* return the pointer to the left or right side of p, or p itself,
	   depending on the optype of p */

	switch (c) {

	case '1':
	case '2':
	case '3':
	case 'D':
		if (c == 'D')
			c = 0;
		else
			c -= '0';
		q = &resc[c];
		q->n_op = REG;
		q->n_type = p->n_type; /* XXX should be correct type */
		q->n_rval = DECRA(p->n_reg, c);
		q->n_su = p->n_su;
		return q;

	case 'L':
		return( optype( p->n_op ) == LTYPE ? p : p->n_left );

	case 'R':
		return( optype( p->n_op ) != BITYPE ? p : p->n_right );

	}
	cerror( "bad getlr: %c", c );
	/* NOTREACHED */
	return NULL;
}

static char *tarr[] = {
	"CHAR", "SHORT", "INT", "LONG", "FLOAT", "DOUBLE", "POINT", "UCHAR",
	"USHORT", "UINT", "ULONG", "PTRTO", "ANY", "STRUCT", "LONGLONG",
	"ULONGLONG",
};

void
prttype(int t)
{
	int i, gone = 0;

	for (i = 0; i < 16; i++)
		if ((t >> i) & 1) {
			if (gone) putchar('|');
			gone++;
			printf("%s", tarr[i]);
		}
}


static int shltab[] = { 0, 0, LOREG, LREG };
static int shrtab[] = { 0, 0, ROREG, RREG };

#ifdef PCC_DEBUG
#define	F2DEBUG(x)	if (f2debug) printf x
#define	F2WALK(x)	if (f2debug) fwalk(x, e2print, 0)
#else
#define	F2DEBUG(x)
#define	F2WALK(x)
#endif

/*
 * Convert a node to REG or OREG.
 * Shape is register class where we want the result.
 * Returns register class if register nodes.
 * If w is: (should be shapes)
 *	- LREG - result in register, call geninsn().
 *	- LOREG - create OREG; call offstar().
 *	- 0 - clear su, walk down.
 */
static int
swmatch(NODE *p, int shape, int w)
{
#ifdef ragge
	int rv = 0;
	switch (w) {
	case LREG:
		rv = geninsn(p, shape);
		break;

	case LOREG:
		/* should be here only if op == UMUL */
		if (p->n_op != UMUL && p->n_op != FLD)
			comperr("swmatch %p", p);
		if (p->n_op == FLD) {
			(void)offstar(p->n_left->n_left, shape);
			p->n_left->n_su = 0;
		} else
			(void)offstar(p->n_left, shape);
		p->n_su = 0;
		rv = ffs(shape)-1;
		break;

	case 0:
		if (optype(p->n_op) == BITYPE)
			swmatch(p->n_right, 0, 0);
		if (optype(p->n_op) != LTYPE)
			swmatch(p->n_left, 0, 0);
		p->n_su = 0;
	}
	return rv;

#else
	if (w == LREG) /* Should be SRREG */
		return geninsn(p, shape);

	/* should be here only if op == UMUL */
	if (p->n_op != UMUL && p->n_op != FLD)
		comperr("swmatch %p", p);
	if (p->n_op == FLD) {
		(void)offstar(p->n_left->n_left, shape);
		p->n_left->n_su = DOWNL;
	} else
		(void)offstar(p->n_left, shape);
	p->n_su = DOWNL;
	return ffs(shape)-1;
#endif
}

/*
 * Help routines for find*() functions.
 * If the node will be a REG node and it will be rewritten in the
 * instruction, ask for it to be put in a register.
 */
static int
chcheck(NODE *p, int shape, int rew)
{
	if (tshape(p, shape) == SRNOPE) {
		if ((shape & INREGS) == 0)
			return SRNOPE;
		return SRREG;
	} else if (rew && (shape & INREGS) && /* Beh�vs shape??? */ isreg(p))
		return SRREG;
	return SRDIR;
}

/*
 * Check how to walk further down.
 * Merge with swmatch()?
 * 	sh - shape for return value (register class).
 *	p - node (for this leg)
 *	shape - given shape for this leg
 *	cookie - cookie given for parent node
 *	rv - switch key for traversing down
 *	returns register class.
 */
static int
shswitch(int sh, NODE *p, int shape, int cookie, int rew, int go)
{
	int lsh;

	switch (go) {
	case 0: /* direct match, just clear su */
		(void)swmatch(p, 0, 0);
		break;

	case SROREG: /* call offstar to prepare for OREG conversion */
		(void)swmatch(p, shape, LOREG);
		break;

	case SRREG: /* call geninsn() to get value into register */
		lsh = shape & INREGS;
		if (rew && cookie != FOREFF)
			lsh &= (cookie & INREGS);
		lsh = swmatch(p, lsh, LREG);
		if (rew)
			sh = lsh;
		break;
	}
	return sh;
}

/*
 * Find the best instruction to evaluate the given tree.
 * Best is to match both subnodes directly, second-best is if
 * subnodes must be evaluated into OREGs, thereafter if nodes 
 * must be put into registers.
 * Whether 2-op instructions or 3-op is preferred is depending on in
 * which order they are found in the table.
 * mtchno is set to the count of regs needed for its legs.
 */
int
findops(NODE *p, int cookie)
{
	extern int *qtable[];
	struct optab *q, *qq = NULL;
	int i, shl, shr, *ixp, sh;
#ifdef ragge
	int lvl = 10, idx = 0, gol = 0, gor = 0;
#else
	int rv = 0, mtchno = 3;
#endif
	NODE *l, *r;

	F2DEBUG(("findops node %p (%s)\n", p, prcook(cookie)));
	F2WALK(p);

	ixp = qtable[p->n_op];
	l = getlr(p, 'L');
	r = getlr(p, 'R');
	for (i = 0; ixp[i] >= 0; i++) {
		q = &table[ixp[i]];

		F2DEBUG(("findop: ixp %d\n", ixp[i]));
		if (ttype(l->n_type, q->ltype) == 0 ||
		    ttype(r->n_type, q->rtype) == 0)
			continue; /* Types must be correct */

		if ((cookie & q->visit) == 0)
			continue; /* must get a result */

		F2DEBUG(("findop got types\n"));
#ifdef ragge
		if ((shl = chcheck(l, q->lshape, q->rewrite & RLEFT)) == SRNOPE)
			continue;
#else
		if ((shl = tshape(l, q->lshape)) == SRNOPE)
			continue; /* useless */
		if (shl == SRDIR && (q->rewrite & RLEFT))
			shl = (q->lshape & INREGS) ? SRREG : SRNOPE;
#endif

		F2DEBUG(("findop lshape %d\n", shl));
		F2WALK(l);

#ifdef ragge
		if ((shr = chcheck(r, q->rshape, q->rewrite & RRIGHT))== SRNOPE)			continue;
#else
		if ((shr = tshape(r, q->rshape)) == SRNOPE)
			continue; /* useless */
		if (shr == SRDIR && (q->rewrite & RRIGHT))
			shr = (q->rshape & INREGS) ? SRREG : SRNOPE;
#endif

		F2DEBUG(("findop rshape %d\n", shr));
		F2WALK(r);

		if (q->needs & REWRITE)
			break;  /* Done here */

#ifndef ragge
		/* avoid clobbering of longlived regs */
		/* let register allocator coalesce */
		if (q->rewrite & RLEFT && shl == SRDIR && isreg(l))
			shl = SRREG;
		if (q->rewrite & RRIGHT && shr == SRDIR && isreg(r))
			shr = SRREG;
#endif

#ifdef ragge
		if (lvl <= (shl + shr))
			continue;
		lvl = shl + shr;
		qq = q;
		idx = ixp[i];
		gol = shl;
		gor = shr;
#else
		if (shl == SRDIR && shr== SRDIR ) {
			/*
			 * both shapes maches directly.
			 * best match, done
			 */
			mtchno = 0;
			rv = MKIDX(ixp[i], 0);
			break;
		}
		F2DEBUG(("second\n"));
		if (shl == SRDIR) {
			/*
			 * Left matches directly, if right can be put
			 * into a register do that.
			 */
			if (mtchno > 1 && (q->rshape & INREGS)) {
				mtchno = 1;
				rv = MKIDX(ixp[i], RREG);
			}
		} else if (shr == SRDIR) {
			/*
			 * Right matches directly, if left can be put
			 * into a register do that.
			 */
			if (mtchno > 1 && (q->lshape & INREGS)) {
				mtchno = 1;
				rv = MKIDX(ixp[i], LREG);
			}
		} else {
			/*
			 * None matches, if both can be put into register
			 * then ask for that.
			 */
			if (mtchno > 2 && (q->lshape & INREGS) &&
			    (q->rshape & INREGS)) {
				mtchno = 2;
				rv = MKIDX(ixp[i], LREG|RREG);
			}
		}
#endif
	}
#ifdef ragge
	if (lvl == 10) {
#else
	if (mtchno == 3) {
#endif
		F2DEBUG(("findops failed\n"));
		if (setbin(p))
			return FRETRY;
		return FFAIL;
	}

#ifndef ragge
	q = &table[TBLIDX(rv)];
#endif
#if 0
	if ((rv & LMASK) == 0 && p->n_left->n_op == TEMP
	    && getclass(p->n_left->n_lval) == 0)
		setclass(p->n_left->n_lval, ffs(q->lshape & INREGS)-1);
	if ((rv & RMASK) == 0 && p->n_right->n_op == TEMP
	    && getclass(p->n_right->n_lval) == 0)
		setclass(p->n_right->n_lval, ffs(q->rshape & INREGS)-1);
#endif

#ifdef ragge
	F2DEBUG(("findops entry %d(%s,%s)\n", idx, srtyp[gol], srtyp[gor]));
#else
	F2DEBUG(("findops entry %d(%s %s)\n",
	    TBLIDX(rv), ltyp[rv & LMASK], rtyp[(rv&RMASK)>>2]));
#endif

	sh = -1;
#ifdef ragge
	sh = shswitch(sh, p->n_left, qq->lshape, cookie,
	    qq->rewrite & RLEFT, gol);
	sh = shswitch(sh, p->n_right, qq->rshape, cookie,
	    qq->rewrite & RRIGHT, gor);
#else
	if (rv & LMASK) {
		int lsh = q->lshape & INREGS;
		if ((q->rewrite & RLEFT) && (cookie != FOREFF))
			lsh &= (cookie & INREGS);
		lsh = swmatch(p->n_left, lsh, rv & LMASK);
		if (q->rewrite & RLEFT)
			sh = lsh;
	}
	if (rv & RMASK) {
		int rsh = q->rshape & INREGS;
		if ((q->rewrite & RRIGHT) && (cookie != FOREFF))
			rsh &= (cookie & INREGS);
		rsh = swmatch(p->n_right, rsh, (rv & RMASK) >> 2);
		if (q->rewrite & RRIGHT)
			sh = rsh;
	}
#endif
	if (sh == -1)
		sh = ffs(cookie & qq->visit & INREGS)-1;
	F2DEBUG(("findops: node %p (%s)\n", p, prcook(1 << sh)));
#ifdef ragge
	p->n_su = MKIDX(idx, 0);
	SCLASS(p->n_su, sh);
#else
	SCLASS(rv, sh);
	p->n_su = rv;
#endif
	return sh;
}

/*
 * Find the best relation op for matching the two trees it has.
 * This is a sub-version of the function findops() above.
 * The instruction with the lowest grading is emitted.
 *
 * Level assignment for priority:
 *	left	right	prio
 *	-	-	-
 *	direct	direct	1
 *	direct	OREG	2	# make oreg
 *	OREG	direct	2	# make oreg
 *	OREG	OREG	2	# make both oreg
 *	direct	REG	3	# put in reg
 *	OREG	REG	3	# put in reg, make oreg
 *	REG	direct	3	# put in reg
 *	REG	OREG	3	# put in reg, make oreg
 *	REG	REG	4	# put both in reg
 */
int
relops(NODE *p)
{
	extern int *qtable[];
	struct optab *q;
	int i, shl, shr;
	NODE *l, *r;
	int *ixp;
	int rv = -1, mtchno = 10;

	F2DEBUG(("relops tree:\n"));
	F2WALK(p);

	ixp = qtable[p->n_op];
	for (i = 0; ixp[i] >= 0; i++) {
		q = &table[ixp[i]];

		F2DEBUG(("relops: ixp %d\n", ixp[i]));
		l = getlr(p, 'L');
		r = getlr(p, 'R');
		if (ttype(l->n_type, q->ltype) == 0 ||
		    ttype(r->n_type, q->rtype) == 0)
			continue; /* Types must be correct */

		F2DEBUG(("relops got types\n"));
#ifdef ragge
		if ((shl = chcheck(l, q->lshape, 0)) == SRNOPE)
			continue;
#else
		shl = tshape(l, q->lshape);
		if (shl == 0)
			continue; /* useless */
#endif
		F2DEBUG(("relops lshape %d\n", shl));
		F2WALK(p);
#ifdef ragge
		if ((shr = chcheck(r, q->rshape, 0)) == SRNOPE)
			continue;
#else
		shr = tshape(r, q->rshape);
		if (shr == 0)
			continue; /* useless */
#endif
		F2DEBUG(("relops rshape %d\n", shr));
		F2WALK(p);
		if (q->needs & REWRITE)
			break;	/* Done here */

		if (shl+shr < mtchno) {
			mtchno = shl+shr;
			rv = MKIDX(ixp[i], shltab[shl]|shrtab[shr]);
		}
	}
	if (rv == -1) {
		F2DEBUG(("relops failed\n"));
		if (setbin(p))
			return FRETRY;
		return FFAIL;
	}
	F2DEBUG(("relops entry %d(%s %s)\n",
	    TBLIDX(rv), ltyp[rv & LMASK], rtyp[(rv&RMASK)>>2]));

	q = &table[TBLIDX(rv)];
	if (rv & LMASK) {
		int lsh = q->lshape & INREGS;
		(void)swmatch(p->n_left, lsh, rv & LMASK);
	}
	if (rv & RMASK) {
		int rsh = q->rshape & INREGS;
		(void)swmatch(p->n_right, rsh, (rv & RMASK) >> 2);
	}
	F2DEBUG(("findops: node %p\n", p));
	SCLASS(rv, CLASSA); /* XXX */
	p->n_su = rv | LREG | RREG;
	return 0;
}

/*
 * Find a matching assign op.
 *
 * Level assignment for priority:
 *	left	right	prio
 *	-	-	-
 *	direct	direct	1
 *	direct	REG	2
 *	direct	OREG	3
 *	OREG	direct	4
 *	OREG	REG	5
 *	OREG	OREG	6
 */
int
findasg(NODE *p, int cookie)
{
	extern int *qtable[];
	struct optab *q;
	int i, sh, shl, shr, lvl = 10;
	NODE *l, *r;
	int *ixp;
#ifndef ragge
	int rv = -1;
	int lshape, rshape;
#endif
	struct optab *qq = NULL; /* XXX gcc */
#ifdef ragge
	int idx = 0, gol = 0, gor = 0;
#endif

#ifndef ragge
	lshape = rshape = 
#endif
	shl = shr = 0;

	F2DEBUG(("findasg tree: %s\n", prcook(cookie)));
	F2WALK(p);

	ixp = qtable[p->n_op];
	l = getlr(p, 'L');
	r = getlr(p, 'R');
	for (i = 0; ixp[i] >= 0; i++) {
		q = &table[ixp[i]];

		F2DEBUG(("asgop: ixp %d\n", ixp[i]));
		if (ttype(l->n_type, q->ltype) == 0 ||
		    ttype(r->n_type, q->rtype) == 0)
			continue; /* Types must be correct */

		if ((cookie & q->visit) == 0)
			continue; /* must get a result */

		F2DEBUG(("asgop got types\n"));
		if ((shl = tshape(l, q->lshape)) == SRNOPE)
			continue;

		if (p->n_left->n_op == TEMP)
			shl = SRDIR;
		else if (shl == SRREG)
			continue;

		F2DEBUG(("asgop lshape %d\n", shl));
		F2WALK(l);

#ifdef ragge
		if ((shr = chcheck(r, q->rshape, q->rewrite & RRIGHT))== SRNOPE)
			continue;
#else
		if ((shr = tshape(r, q->rshape)) == SRNOPE)
			continue; /* useless */
#endif

		F2DEBUG(("asgop rshape %d\n", shr));
		F2WALK(r);
		if (q->needs & REWRITE)
			break;	/* Done here */

		if (lvl <= (shl + shr))
			continue;
		lvl = shl + shr;
		qq = q;
#ifdef ragge
		idx = ixp[i];
		gol = shl;
		gor = shr;
#else
		lshape = q->lshape;
		rshape = q->rshape;
		rv = MKIDX(ixp[i], shltab[shl]|shrtab[shr]);
#endif
	}

	if (lvl == 10) {
		F2DEBUG(("findasg failed\n"));
		if (setasg(p, cookie))
			return FRETRY;
		return FFAIL;
	}
	F2DEBUG(("findasg entry %d(%s,%s)\n", idx, srtyp[gol], srtyp[gor]));

	sh = -1;
#ifdef ragge
	sh = shswitch(sh, p->n_left, qq->lshape, cookie,
	    qq->rewrite & RLEFT, gol);
#else
	if (rv & LMASK) {
		int lsh = qq->lshape & INREGS;
		if ((qq->rewrite & RLEFT) && (cookie != FOREFF))
			lsh &= (cookie & INREGS);
		lsh = swmatch(p->n_left, lsh, rv & LMASK);
		if (qq->rewrite & RLEFT)
			sh = lsh;
		rv |= LREG;
	}
#endif

#ifdef ragge
	sh = shswitch(sh, p->n_right, qq->rshape, cookie,
	    qq->rewrite & RRIGHT, gor);
#else
	if (rv & RMASK) {
		int rsh = qq->rshape & INREGS;
		if ((qq->rewrite & RRIGHT) && (cookie != FOREFF))
			rsh &= (cookie & INREGS);
		rsh = swmatch(p->n_right, rsh, (rv & RMASK) >> 2);
		if (qq->rewrite & RRIGHT)
			sh = rsh;
		rv |= RREG;
	}
#endif
	if (sh == -1) {
		if (cookie == FOREFF)
			sh = 0;
		else
			sh = ffs(cookie & qq->visit & INREGS)-1;
	}
	F2DEBUG(("findasg: node %p class %d\n", p, sh));
#ifdef ragge
	p->n_su = MKIDX(idx, 0);
	SCLASS(p->n_su, sh);
#else
	SCLASS(rv, sh);
	p->n_su = rv;
#endif
#if 0
	if (p->n_left->n_op == TEMP)
		setclass(p->n_left->n_lval, sh);
#endif
	return sh;
}

/*
 * Search for an UMUL table entry that can turn an indirect node into
 * a move from an OREG.
 */
int
findumul(NODE *p, int cookie)
{
	extern int *qtable[];
	struct optab *q = NULL; /* XXX gcc */
	int i, shl, shr, sh;
	int *ixp;
	int rv = -1;

	F2DEBUG(("findumul p %p (%s)\n", p, prcook(cookie)));
	F2WALK(p);

	ixp = qtable[p->n_op];
	for (i = 0; ixp[i] >= 0; i++) {
		q = &table[ixp[i]];

		F2DEBUG(("findumul: ixp %d\n", ixp[i]));
		if ((q->visit & cookie) == 0)
			continue; /* wrong registers */

		if (ttype(p->n_type, q->rtype) == 0)
			continue; /* Types must be correct */
		

		F2DEBUG(("findumul got types, rshape %s\n", prcook(q->rshape)));
		/*
		 * Try to create an OREG of the node.
		 * Fake left even though it's right node,
		 * to be sure of conversion if going down left.
		 */
#ifdef ragge
		if ((shl = chcheck(p, q->lshape, 0)) == SRNOPE)
			continue;
#else
		if ((shl = tshape(p, q->rshape)) == SRNOPE)
			continue;
#endif
		
		shr = 0;

		if (q->needs & REWRITE)
			break;	/* Done here */

		F2DEBUG(("findumul got shapes %d,%d\n", shl, shr));

		rv = MKIDX(ixp[i], shltab[shl]|shrtab[shr]);
		/* XXX search better matches */
		break;
	}
	if (rv < 0) {
		F2DEBUG(("findumul failed\n"));
		if (setuni(p, cookie))
			return FRETRY;
		return FFAIL;
	}
	F2DEBUG(("findumul entry %d(%s %s)\n",
	    TBLIDX(rv), ltyp[rv & LMASK], rtyp[(rv&RMASK)>>2]));

#ifdef ragge
	if ((rv & LMASK) == LREG || p->n_op == UMUL) {
#else
	if (rv & LMASK) {
#endif
		sh = swmatch(p, cookie & q->visit & INREGS, rv & LMASK);
	} else
		sh = ffs(cookie & q->visit & INREGS)-1;

	F2DEBUG(("findumul: node %p (%s)\n", p, prcook(1 << sh)));
	SCLASS(rv, sh);
	p->n_su = rv | LOREG;
	return sh;
}

/*
 * Find a leaf type node that puts the value into a register.
 */
int
findleaf(NODE *p, int cookie)
{
	extern int *qtable[];
	struct optab *q = NULL; /* XXX gcc */
	int i, shl, shr, sh;
	int *ixp;
	int rv = -1;

	F2DEBUG(("findleaf p %p (%s)\n", p, prcook(cookie)));
	F2WALK(p);

	ixp = qtable[p->n_op];
	for (i = 0; ixp[i] >= 0; i++) {
		q = &table[ixp[i]];

		F2DEBUG(("findleaf: ixp %d\n", ixp[i]));
		if ((q->visit & cookie) == 0)
			continue; /* wrong registers */

		if (ttype(p->n_type, q->rtype) == 0 ||
		    ttype(p->n_type, q->ltype) == 0)
			continue; /* Types must be correct */

		F2DEBUG(("findleaf got types, rshape %s\n", prcook(q->rshape)));
		if ((shr = tshape(p, q->rshape)) != SRDIR && 
		    (shr != SROREG || (p->n_op != TEMP && p->n_op != REG)))
			continue; /* shape must match */
		shl = 0;

		if (q->needs & REWRITE)
			break;	/* Done here */

		F2DEBUG(("findleaf got shapes %d,%d\n", shl, shr));

		rv = MKIDX(ixp[i], shltab[shl]|shrtab[shr]);
		break;
	}
	if (rv < 0) {
		F2DEBUG(("findleaf failed\n"));
		if (setuni(p, cookie))
			return FRETRY;
		return FFAIL;
	}
	F2DEBUG(("findleaf entry %d(%s %s)\n",
	    TBLIDX(rv), ltyp[rv & LMASK], rtyp[(rv&RMASK)>>2]));

	sh = ffs(cookie & q->visit & INREGS)-1;
	F2DEBUG(("findleaf: node %p (%s)\n", p, prcook(1 << sh)));
	SCLASS(rv, sh);
	p->n_su = rv;
	return sh;
}

/*
 * Find a UNARY op that satisfy the needs.
 * For now, the destination is always a register.
 * Both source and dest types must match, but only source (left)
 * shape is of interest.
 */
int
finduni(NODE *p, int cookie)
{
	extern int *qtable[];
	struct optab *q;
	NODE *l, *r;
	int i, shl, num = 4;
	int *ixp;
	int rv = -1;
	int sh, pmask;

	F2DEBUG(("finduni tree: %s\n", prcook(cookie)));
	F2WALK(p);

	l = getlr(p, 'L');
	if (p->n_op == CALL || p->n_op == FORTCALL || p->n_op == STCALL)
		r = p;
	else
		r = getlr(p, 'R');
	ixp = qtable[p->n_op];
	for (i = 0; ixp[i] >= 0; i++) {
		q = &table[ixp[i]];

		F2DEBUG(("finduni: ixp %d\n", ixp[i]));
		if (ttype(l->n_type, q->ltype) == 0)
			continue; /* Type must be correct */

		F2DEBUG(("finduni got left type\n"));
		if (ttype(r->n_type, q->rtype) == 0)
			continue; /* Type must be correct */

		F2DEBUG(("finduni got types\n"));
#ifdef ragge
		if ((shl = chcheck(l, q->lshape, q->rewrite & RLEFT)) == SRNOPE)
			continue;
#else
		if ((shl = tshape(l, q->lshape)) == SRNOPE)
			continue; /* shape must match */
#endif

		F2DEBUG(("finduni got shapes %d\n", shl));

		if ((cookie & q->visit) == 0)	/* check correct return value */
			continue;		/* XXX - should check needs */

		/* avoid clobbering of longlived regs */
		/* let register allocator coalesce */
		if ((q->rewrite & RLEFT) && (shl == SRDIR) && isreg(l))
			shl = SRREG;

		F2DEBUG(("finduni got cookie\n"));
		if (q->needs & REWRITE)
			break;	/* Done here */

		if (shl >= num)
			continue;
		num = shl;
		rv = MKIDX(ixp[i], shltab[shl]);

		if (shl == SRDIR)
			break;
	}

	if (rv == -1) {
		F2DEBUG(("finduni failed\n"));
	} else
		F2DEBUG(("finduni entry %d(%s %s)\n",
		    TBLIDX(rv), ltyp[rv & LMASK], rtyp[(rv&RMASK)>>2]));
	if (rv < 0) {
		if (setuni(p, cookie))
			return FRETRY;
		return FFAIL;
	}
	q = &table[TBLIDX(rv)];

	pmask = cookie & q->visit & INREGS;
#ifdef ragge
	if ((rv & LMASK) == LREG || l->n_op == UMUL) {
#else
	if (rv & LMASK) {
#endif
		sh = swmatch(l, q->lshape & INREGS, rv & LMASK);
		if ((q->rewrite & RLEFT) && (pmask & (1 << sh)))
			pmask = (1 << sh);
	}
	sh = ffs(pmask)-1;
	if (sh == -1)
		sh = 0; /* no registers */

	F2DEBUG(("finduni: node %p (%s)\n", p, prcook(1 << sh)));
	SCLASS(rv, sh);
	p->n_su = rv | LREG;
	return sh;
}
