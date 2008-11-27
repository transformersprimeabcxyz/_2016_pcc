/*	$Id: table.c,v 1.1 2008/11/27 05:25:15 mickey Exp $	*/
/*
 * Copyright (c) 2008 Michael Shalayeff
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


# include "pass2.h"

# define TLL TLONG|TULONG|TLONGLONG|TULONGLONG
# define ANYSIGNED TINT|TSHORT|TCHAR
# define ANYUSIGNED TUNSIGNED|TUSHORT|TUCHAR
# define ANYFIXED ANYSIGNED|ANYUSIGNED
# define TUWORD TUNSIGNED
# define TSWORD TINT
# define TWORD	TUWORD|TSWORD
#define	 SHINT	SAREG	/* short and int */
#define	 ININT	INAREG
#define	 SHCH	SBREG	/* shape for char */
#define	 INCH	INBREG
#define	 SHLL	SCREG	/* shape for long long */
#define	 INLL	INCREG
#define	 SHFL	SDREG	/* shape for float/double */
#define	 INFL	INDREG	/* shape for float/double */

struct optab table[] = {
/* First entry must be an empty entry */
{ -1, FOREFF, SANY, TANY, SANY, TANY, 0, 0, "", },

/* PCONVs are usually not necessary */
{ PCONV,	INAREG,
	SAREG,	TWORD|TPOINT,
	SAREG,	TWORD|TPOINT,
		0,	RLEFT,
		"", },

/*
 * A bunch conversions of integral<->integral types
 * There are lots of them, first in table conversions to itself
 * and then conversions from each type to the others.
 */

/* itself to itself, including pointers */

/* convert (u)char to (u)char. */
{ SCONV,	INCH,
	SHCH,	TCHAR|TUCHAR,
	SHCH,	TCHAR|TUCHAR,
		0,	RLEFT,
		"", },

/* convert pointer to (u)longlong. */
{ SCONV,	INLL,
	SHLL,	TLL|TPOINT,
	SHLL,	TLL,
		0,	RLEFT,
		"", },

/* convert double <-> float. nothing to do here */
{ SCONV,	INFL,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
		0,	RLEFT,
		"", },

/* convert pointers to pointers and ints. */
{ SCONV,	ININT,
	SHINT,	TPOINT|TWORD,
	SANY,	TPOINT,
		0,	RLEFT,
		"", },

/* char to something */

/* convert char to (unsigned) short. */
{ SCONV,	ININT,
	SBREG|SOREG|SNAME,	TCHAR,
	SAREG,	TSHORT|TUSHORT,
		NASL|NAREG,	RESC1,
		"	movsbw AL,A1\n", },

/* convert unsigned char to (u)short. */
{ SCONV,	ININT,
	SHCH|SOREG|SNAME,	TUCHAR,
	SAREG,	TSHORT|TUSHORT,
		NASL|NAREG,	RESC1,
		"	movzbw AL,A1\n", },

/* convert signed char to int (or pointer). */
{ SCONV,	ININT,
	SHCH|SOREG|SNAME,	TCHAR,
	SAREG,	TWORD|TPOINT,
		NASL|NAREG,	RESC1,
		"	movsbl AL,A1\n", },

/* convert unsigned char to (u)int. */
{ SCONV,	ININT,
	SHCH|SOREG|SNAME,	TUCHAR,
	SAREG,	TWORD,
		NASL|NAREG,	RESC1,
		"	movzbl AL,A1\n", },

/* convert char to (u)long long */
{ SCONV,	INLL,
	SHCH|SOREG|SNAME,	TCHAR,
	SANY,	TLL,
		NCREG|NCSL,	RESC1,
		"	movsbq AL,A1\n", },

/* convert unsigned char to (u)long long */
{ SCONV,	INLL,
	SHCH|SOREG|SNAME,	TUCHAR,
	SANY,			TLL,
		NCREG|NCSL,	RESC1,
		"	movzbq AL,A1\n", },

/* convert char (in register) to double XXX - use NTEMP */
{ SCONV,	INFL,
	SHCH|SOREG|SNAME,	TCHAR,
	SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		NAREG|NASL|NDREG,	RESC2,
		"	movsbl AL,A1\n	pushl A1\n"	/* XXX fpconv */
		"	fildl (%rsp)\n	addl $8,%rsp\n", },

/* convert (u)char (in register) to double XXX - use NTEMP */
{ SCONV,	INFL,
	SHCH|SOREG|SNAME,	TUCHAR,
	SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		NAREG|NASL|NDREG,	RESC2,
		"	movzbl AL,A1\n	pushl A1\n"	/* XXX fpconv */
		"	fildl (%rsp)\n	addl $8,%rsp\n", },

/* short to something */

/* convert short (in memory) to char */
{ SCONV,	INCH,
	SNAME|SOREG,	TSHORT|TUSHORT,
	SHCH,		TCHAR|TUCHAR,
		NBREG|NBSL,	RESC1,
		"	movb AL,A1\n", },

/* convert short (in reg) to char. */
{ SCONV,	INCH,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SHCH,			TCHAR|TUCHAR,
		NSPECIAL|NBREG|NBSL,	RESC1,
		"ZM", },

/* convert short to (u)int. */
{ SCONV,	ININT,
	SAREG|SOREG|SNAME,	TSHORT,
	SAREG,	TWORD,
		NASL|NAREG,	RESC1,
		"	movswl AL,A1\n", },

/* convert unsigned short to (u)int. */
{ SCONV,	ININT,
	SAREG|SOREG|SNAME,	TUSHORT,
	SAREG,	TWORD,
		NASL|NAREG,	RESC1,
		"	movzwl AL,A1\n", },

/* convert short to (u)long long */
{ SCONV,	INLL,
	SAREG|SOREG|SNAME,	TSHORT,
	SHLL,			TLL,
		NCREG|NCSL,	RESC1,
		"	movswq AL,A1\n", },

/* convert unsigned short to (u)long long */
{ SCONV,	INLL,
	SAREG|SOREG|SNAME,	TUSHORT,
	SHLL,			TLL,
		NCREG|NCSL,	RESC1,
		"	movzwq AL,A1\n", },

/* convert short (in memory) to float/double */
{ SCONV,	INFL,
	SOREG|SNAME,	TSHORT,
	SDREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		NDREG,	RESC1,
		"	fild AL\n", },	/* XXX fpconv */

/* convert short (in register) to float/double */
{ SCONV,	INFL,
	SAREG,	TSHORT,
	SDREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		NTEMP|NDREG,	RESC1,	/* XXX fpconv */
		"	pushw AL\n	fild (%rsp)\n	addl $8,%rsp\n", },

/* convert unsigned short to double XXX - use NTEMP */
{ SCONV,	INFL,
	SAREG|SOREG|SNAME,	TUSHORT,
	SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		NAREG|NASL|NDREG|NTEMP,	RESC2,
		"	movzwl AL,A1\n	pushl A1\n"	/* XXX fpconv */
		"	fildl (%rsp)\n	addl $8,%esp\n", },

/* int to something */

/* convert int to char. This is done when register is loaded */
{ SCONV,	INCH,
	SAREG,	TWORD|TPOINT,
	SANY,	TCHAR|TUCHAR,
		NSPECIAL|NBREG|NBSL,	RESC1,
		"ZM", },

/* convert int to short. Nothing to do */
{ SCONV,	INAREG,
	SAREG,	TWORD,
	SANY,	TSHORT|TUSHORT,
		0,	RLEFT,
		"", },

/* convert signed int to (u)long long */
{ SCONV,	INLL,
	SHINT,	TSWORD,
	SHLL,	TLL,
		NSPECIAL|NCREG|NCSL,	RESC1,
		"	cltd\n", },

/* convert unsigned int to (u)long long */
{ SCONV,	INLL,
	SHINT|SOREG|SNAME,	TUWORD|TPOINT,
	SHLL,	TLL,
		NCSL|NCREG,	RESC1,
		"	movzlq AL,A1\n\n", },

/* convert int (in memory) to double */
{ SCONV,	INFL,
	SOREG|SNAME,	TWORD,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
		NDREG,	RESC1,
		"	fildl AL\n", },

/* convert int (in register) to double */
{ SCONV,	INFL,
	SAREG,	TWORD,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
		NTEMP|NDREG,	RESC1,	/* XXX fpconv */
		"	pushl AL\n	fildl (%esp)\n	addl $8,%esp\n", },

/* long long to something */

/* convert (u)long long to (u)char (mem->reg) */
{ SCONV,	INCH,
	SOREG|SNAME,	TLL,
	SANY,	TCHAR|TUCHAR,
		NBREG|NBSL,	RESC1,
		"	movb AL,A1\n", },

/* convert (u)long long to (u)char (reg->reg, hopefully nothing) */
{ SCONV,	INCH,
	SHLL,	TLL,
	SANY,	TCHAR|TUCHAR,
		NBREG|NBSL,	RESC1,
		"ZS", },

/* convert (u)long long to (u)short (mem->reg) */
{ SCONV,	INAREG,
	SOREG|SNAME,	TLL,
	SAREG,	TSHORT|TUSHORT,
		NAREG|NASL,	RESC1,
		"	movw AL,A1\n", },

/* convert (u)long long to (u)short (reg->reg, hopefully nothing) */
{ SCONV,	INAREG,
	SHLL|SOREG|SNAME,	TLL,
	SAREG,	TSHORT|TUSHORT,
		NAREG|NASL,	RESC1,
		"ZS", },

/* convert long long to int (mem->reg) */
{ SCONV,	INAREG,
	SOREG|SNAME,	TLL,
	SAREG,	TWORD|TPOINT,
		NAREG|NASL,	RESC1,
		"	movl AL,A1\n", },

/* convert long long to int (reg->reg, hopefully nothing) */
{ SCONV,	INAREG,
	SHLL|SOREG|SNAME,	TLL,
	SAREG,	TWORD|TPOINT,
		NAREG|NASL,	RESC1,
		"ZS", },

/* convert long long (in memory) to floating */
{ SCONV,	INFL,
	SOREG|SNAME,	TLONGLONG,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
		NDREG,	RESC1,	/* XXX fpconv */
		"	fildq AL\n", },

/* convert long long (in register) to floating */
{ SCONV,	INFL,
	SHLL,	TLONGLONG,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
		NTEMP|NDREG,	RESC1,	/* XXX fpconv */
		"	pushq AL\n\n"
		"	fildq (%esp)\n	addl $8,%esp\n", },

/* convert unsigned long long to floating */
{ SCONV,	INFL,
	SCREG,	TULONGLONG,
	SDREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		NDREG,	RESC1,	/* XXX fpconv */
		"ZJ", },

/* float to something */

#if 0 /* go via int by adding an extra sconv in clocal() */
/* convert float/double to (u) char. XXX should use NTEMP here */
{ SCONV,	INCH,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
	SHCH,	TWORD|TSHORT|TUSHORT|TCHAR|TUCHAR,
		NBREG,	RESC1,	/* XXX fpconv */
		"	subl $4,%esp\n	fistpl (%esp)\n	popl A1\n", },

/* convert float/double to (u) int/short/char. XXX should use NTEMP here */
{ SCONV,	INCH,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
	SHCH,	TWORD|TSHORT|TUSHORT|TCHAR|TUCHAR,
		NCREG,	RESC1,	/* XXX fpconv */
		"	subl $4,%esp\n	fistpl (%esp)\n	popl A1\n", },
#endif

/* convert float/double to (u)int. XXX should use NTEMP here */
{ SCONV,	INAREG,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
	SAREG,	TWORD,
		NAREG,	RESC1,	/* XXX fpconv */
#ifdef notdef	/* Must round down and nothing else */
		"	subl $4,%esp\n	fistpl (%esp)\n	popl A1\n", },
#else
		"	subl $12,%esp\n"
		"	fnstcw (%esp)\n"
		"	fnstcw 4(%esp)\n"
		"	movb $12,1(%esp)\n"
		"	fldcw (%esp)\n"
		"	fistpl 8(%esp)\n"
		"	movl 8(%esp),A1\n"
		"	fldcw 4(%esp)\n"
		"	addl $12,%esp\n", },
#endif

/* convert float/double (in register) to (unsigned) long long */
/* XXX - unsigned is not handled correct */
{ SCONV,	INLL,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
	SHLL,	TLONGLONG|TULONGLONG,
		NCREG,	RESC1,	/* XXX fpconv */
#ifdef notdef	/* Must round down and nothing else */
		"	subl $8,%esp\n	fistpq (%esp)\n"
		"	popl A1\n	popl U1\n", },
#else
		"	subl $16,%esp\n"
		"	fnstcw (%esp)\n"
		"	fnstcw 4(%esp)\n"
		"	movb $12,1(%esp)\n"
		"	fldcw (%esp)\n"
		"	fistpq 8(%esp)\n"
		"	movl 8(%esp),A1\n"
		"	movl 12(%esp),U1\n"
		"	fldcw 4(%esp)\n"
		"	addl $16,%esp\n", },
#endif

/* slut sconv */

/*
 * Subroutine calls.
 */

{ CALL,		FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	0,
		"	call CL\nZC", },

{ UCALL,	FOREFF,
	SCON,	TANY,
	SAREG,	TWORD|TPOINT,
		0,	0,
		"	call CL\n", },

{ CALL,	INAREG,
	SCON,	TANY,
	SAREG,	TSHORT|TUSHORT|TWORD|TPOINT,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ UCALL,	INAREG,
	SCON,	TANY,
	SAREG,	TSHORT|TUSHORT|TWORD|TPOINT,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call CL\n", },

{ CALL,	INBREG,
	SCON,	TANY,
	SBREG,	TCHAR|TUCHAR,
		NBREG,	RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ UCALL,	INBREG,
	SCON,	TANY,
	SBREG,	TCHAR|TUCHAR,
		NBREG,	RESC1,	/* should be 0 */
		"	call CL\n", },

{ CALL,		INCREG,
	SCON,	TANY,
	SCREG,	TANY,
		NCREG|NCSL,	RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ UCALL,	INCREG,
	SCON,	TANY,
	SCREG,	TANY,
		NCREG|NCSL,	RESC1,	/* should be 0 */
		"	call CL\n", },

{ CALL,	INDREG,
	SCON,	TANY,
	SDREG,	TANY,
		NDREG|NDSL,	RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ UCALL,	INDREG,
	SCON,	TANY,
	SDREG,	TANY,
		NDREG|NDSL,	RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ CALL,		FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	0,
		"	call *AL\nZC", },

{ UCALL,	FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	0,
		"	call *AL\nZC", },

{ CALL,		INAREG,
	SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ UCALL,	INAREG,
	SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ CALL,		INBREG,
	SAREG,	TANY,
	SANY,	TANY,
		NBREG|NBSL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ UCALL,	INBREG,
	SAREG,	TANY,
	SANY,	TANY,
		NBREG|NBSL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ CALL,		INCREG,
	SAREG,	TANY,
	SANY,	TANY,
		NCREG|NCSL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ UCALL,	INCREG,
	SAREG,	TANY,
	SANY,	TANY,
		NCREG|NCSL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ CALL,		INDREG,
	SAREG,	TANY,
	SANY,	TANY,
		NDREG|NDSL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ UCALL,	INDREG,
	SAREG,	TANY,
	SANY,	TANY,
		NDREG|NDSL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

/* struct return */
{ USTCALL,	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		NAREG|NASL,	0,
		"	call CL\nZC", },

{ USTCALL,	INAREG,
	SCON,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ USTCALL,	INAREG,
	SNAME|SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ STCALL,	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		NAREG|NASL,	0,
		"	call CL\nZC", },

{ STCALL,	INAREG,
	SCON,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ STCALL,	INAREG,
	SNAME|SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"	call *AL\nZC", },

/*
 * The next rules handle all binop-style operators.
 */
{ PLUS,		INFL,
	SHFL,		TDOUBLE,
	SNAME|SOREG,	TDOUBLE,
		0,	RLEFT,
		"	faddl AR\n", },

{ PLUS,		INFL|FOREFF,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
		0,	RLEFT,
		"	faddp\n", },

{ PLUS,		INLL|FOREFF,
	SHLL|SNAME|SOREG,	TLL|TPOINT,
	SONE,	TANY,
		0,	RLEFT,
		"	incq AL\n", },

{ PLUS,		INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TWORD,
	SONE,	TANY,
		0,	RLEFT,
		"	incl AL\n", },

{ PLUS,		INLL,
	SCREG,	TLL|TPOINT,
	SCON,	TWORD,
		NCREG|NCSL,	RESC1,
		"	leaq CR(AL),A1\n", },

{ PLUS,		INAREG,
	SAREG,	TWORD,
	SCON,	TANY,
		NAREG|NASL,	RESC1,
		"	leal CR(AL),A1\n", },

{ PLUS,		INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SONE,	TANY,
		0,	RLEFT,
		"	incw AL\n", },

{ PLUS,		INCH|FOREFF,
	SHCH|SNAME|SOREG,	TCHAR|TUCHAR,
	SONE,	TANY,
		0,	RLEFT,
		"	incb AL\n", },

{ PLUS,		INAREG,
	SAREG,	TWORD,
	SAREG,	TWORD,
		NAREG|NASL|NASR,	RESC1,
		"	leal (AL,AR),A1\n", },

{ MINUS,	INLL|FOREFF,
	SCREG|SNAME|SOREG,	TLL|TPOINT,
	SONE,			TANY,
		0,	RLEFT,
		"	decq AL\n", },

{ MINUS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TWORD,
	SONE,			TANY,
		0,	RLEFT,
		"	decl AL\n", },

{ MINUS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SONE,			TANY,
		0,	RLEFT,
		"	decw AL\n", },

{ MINUS,	INCH|FOREFF,
	SHCH|SNAME|SOREG,	TCHAR|TUCHAR,
	SONE,	TANY,
		0,	RLEFT,
		"	decb AL\n", },

/* address as register offset, negative */
{ MINUS,	INAREG,
	SAREG,	TWORD|TPOINT,
	SPCON,	TANY,
		NAREG|NASL,	RESC1,
		"	leal -CR(AL),A1\n", },

{ MINUS,	INFL,
	SHFL,	TDOUBLE,
	SNAME|SOREG,	TDOUBLE,
		0,	RLEFT,
		"	fsubl AR\n", },

{ MINUS,	INFL|FOREFF,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
		0,	RLEFT,
		"	fsubZAp\n", },

/* Simple r/m->reg ops */
/* m/r |= r */
{ OPSIMP,	INLL|FOREFF|FORCC,
	SHLL|SNAME|SOREG,	TLL|TPOINT,
	SHLL,			TLL|TPOINT,
		0,	RLEFT|RESCC,
		"	Oq AR,AL\n", },

/* r |= r/m */
{ OPSIMP,	INLL|FOREFF|FORCC,
	SHLL,			TLL|TPOINT,
	SHLL|SNAME|SOREG,	TLL|TPOINT,
		0,	RLEFT|RESCC,
		"	Oq AR,AL\n", },

/* m/r |= r */
{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TWORD,
	SAREG,			TWORD,
		0,	RLEFT|RESCC,
		"	Ol AR,AL\n", },

/* r |= r/m */
{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG,			TWORD|TPOINT,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
		0,	RLEFT|RESCC,
		"	Ol AR,AL\n", },

/* m/r |= r */
{ OPSIMP,	INAREG|FOREFF|FORCC,
	SHINT|SNAME|SOREG,	TSHORT|TUSHORT,
	SHINT,		TSHORT|TUSHORT,
		0,	RLEFT|RESCC,
		"	Ow AR,AL\n", },

/* r |= r/m */
{ OPSIMP,	INAREG|FOREFF|FORCC,
	SHINT,		TSHORT|TUSHORT,
	SHINT|SNAME|SOREG,	TSHORT|TUSHORT,
		0,	RLEFT|RESCC,
		"	Ow AR,AL\n", },

/* m/r |= r */
{ OPSIMP,	INCH|FOREFF|FORCC,
	SHCH,		TCHAR|TUCHAR,
	SHCH|SNAME|SOREG,	TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"	Ob AR,AL\n", },

/* r |= r/m */
{ OPSIMP,	INCH|FOREFF|FORCC,
	SHCH,		TCHAR|TUCHAR,
	SHCH|SNAME|SOREG,	TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"	Ob AR,AL\n", },

/* m/r |= const */
{ OPSIMP,	INLL|FOREFF|FORCC,
	SCREG|SNAME|SOREG,	TLL,
	SCON,	TWORD,
		0,	RLEFT|RESCC,
		"	Oq AR,AL\n", },

{ OPSIMP,	INAREG|FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TWORD,
	SCON,	TWORD,
		0,	RLEFT|RESCC,
		"	Ol AR,AL\n", },

{ OPSIMP,	INAREG|FOREFF|FORCC,
	SHINT|SNAME|SOREG,	TSHORT|TUSHORT,
	SCON,	TANY,
		0,	RLEFT|RESCC,
		"	Ow AR,AL\n", },

{ OPSIMP,	INCH|FOREFF|FORCC,
	SHCH|SNAME|SOREG,	TCHAR|TUCHAR,
	SCON,	TANY,
		0,	RLEFT|RESCC,
		"	Ob AR,AL\n", },

/* r |= r/m */
{ OPSIMP,	INLL|FOREFF,
	SHLL,	TLL|TPOINT,
	SHLL|SNAME|SOREG,	TLL|TPOINT,
		0,	RLEFT,
		"	Oq AR,AL\n", },

/*
 * The next rules handle all shift operators.
 */
/* r/m <<= r */
{ LS,	INLL|FOREFF,
	SHLL|SNAME|SOREG,	TLL,
	SHLL,		TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	salq AR,AL\n", },

/* r/m <<= const */
{ LS,	INLL|FOREFF,
	SHLL|SNAME|SOREG,	TLL,
	SCON,	TANY,
		0,	RLEFT,
		"	salq AR,AL\n", },

/* r/m <<= r */
{ LS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TWORD,
	SHCH,		TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	sall AR,AL\n", },

/* r/m <<= const */
{ LS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TWORD,
	SCON,	TANY,
		0,	RLEFT,
		"	sall AR,AL\n", },

/* r/m <<= r */
{ LS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SHCH,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	shlw AR,AL\n", },

/* r/m <<= const */
{ LS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SCON,	TANY,
		0,	RLEFT,
		"	shlw AR,AL\n", },

{ LS,	INCH|FOREFF,
	SHCH|SNAME|SOREG,	TCHAR|TUCHAR,
	SHCH,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	salb AR,AL\n", },

{ LS,	INCH|FOREFF,
	SHCH|SNAME|SOREG,	TCHAR|TUCHAR,
	SCON,			TANY,
		0,	RLEFT,
		"	salb AR,AL\n", },

{ RS,	INLL|FOREFF,
	SHLL|SNAME|SOREG,	TLONG|TLONGLONG,
	SHCH,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	sarq AR,AL\n", },

{ RS,	INLL|FOREFF,
	SHLL|SNAME|SOREG,	TLONG|TLONGLONG,
	SCON,			TANY,
		0,		RLEFT,
		"	sarq AR,AL\n", },

{ RS,	INLL|FOREFF,
	SHLL|SNAME|SOREG,	TULONG|TULONGLONG,
	SHCH,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	shrq AR,AL\n", },

{ RS,	INLL|FOREFF,
	SHLL|SNAME|SOREG,	TULONG|TULONGLONG,
	SCON,			TANY,
		0,		RLEFT,
		"	shrq AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSWORD,
	SHCH,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	sarl AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSWORD,
	SCON,			TANY,
		0,		RLEFT,
		"	sarl AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TUWORD,
	SHCH,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	shrl AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TUWORD,
	SCON,			TANY,
		0,		RLEFT,
		"	shrl AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSHORT,
	SHCH,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	sarw AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TSHORT,
	SCON,			TANY,
		0,		RLEFT,
		"	sarw AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TUSHORT,
	SHCH,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	shrw AR,AL\n", },

{ RS,	INAREG|FOREFF,
	SAREG|SNAME|SOREG,	TUSHORT,
	SCON,			TANY,
		0,		RLEFT,
		"	shrw AR,AL\n", },

{ RS,	INCH|FOREFF,
	SHCH|SNAME|SOREG,	TCHAR,
	SHCH,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	sarb AR,AL\n", },

{ RS,	INCH|FOREFF,
	SHCH|SNAME|SOREG,	TCHAR,
	SCON,			TANY,
		0,		RLEFT,
		"	sarb AR,AL\n", },

{ RS,	INCH|FOREFF,
	SHCH|SNAME|SOREG,	TUCHAR,
	SHCH,			TCHAR|TUCHAR,
		NSPECIAL,	RLEFT,
		"	shrb AR,AL\n", },

{ RS,	INCH|FOREFF,
	SHCH|SNAME|SOREG,	TUCHAR,
	SCON,			TANY,
		0,		RLEFT,
		"	shrb AR,AL\n", },

/*
 * The next rules takes care of assignments. "=".
 */
{ ASSIGN,	FORCC|FOREFF|INLL,
	SHLL,		TLL|TPOINT,
	SMIXOR,		TANY,
		0,	RDEST,
		"	xorq AL,AL\n\n", },

{ ASSIGN,	FOREFF|INLL,
	SHLL,		TLL|TPOINT,
	SCON,		TANY,
		0,	RDEST,
		"	movabs AR,AL\n", },

{ ASSIGN,	FORCC|FOREFF|INAREG,
	SAREG,		TWORD,
	SMIXOR,		TANY,
		0,	RDEST,
		"	xorl AL,AL\n", },

{ ASSIGN,	FOREFF,
	SAREG|SNAME|SOREG,	TWORD,
	SCON,		TANY,
		0,	0,
		"	movl AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG,	TWORD,
	SCON,		TANY,
		0,	RDEST,
		"	movl AR,AL\n", },

{ ASSIGN,	FORCC|FOREFF|INAREG,
	SAREG,	TSHORT|TUSHORT,
	SMIXOR,		TANY,
		0,	RDEST,
		"	xorw AL,AL\n", },

{ ASSIGN,	FOREFF,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SCON,		TANY,
		0,	0,
		"	movw AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG,	TSHORT|TUSHORT,
	SCON,		TANY,
		0,	RDEST,
		"	movw AR,AL\n", },

{ ASSIGN,	FOREFF,
	SHCH|SNAME|SOREG,	TCHAR|TUCHAR,
	SCON,		TANY,
		0,	0,
		"	movb AR,AL\n", },

{ ASSIGN,	FOREFF|INCH,
	SHCH,		TCHAR|TUCHAR,
	SCON,		TANY,
		0,	RDEST,
		"	movb AR,AL\n", },

{ ASSIGN,	FOREFF|INLL,
	SHLL|SNAME|SOREG,	TLL,
	SHLL,			TLL,
		0,	RDEST,
		"	movq AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
	SAREG,		TWORD|TPOINT,
		0,	RDEST,
		"	movl AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG,			TWORD|TPOINT,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
		0,	RDEST,
		"	movl AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
	SAREG,		TSHORT|TUSHORT,
		0,	RDEST,
		"	movw AR,AL\n", },

{ ASSIGN,	FOREFF|INCH,
	SHCH|SNAME|SOREG,	TCHAR|TUCHAR,
	SHCH,		TCHAR|TUCHAR|TWORD,
		0,	RDEST,
		"	movb AR,AL\n", },

{ ASSIGN,	FOREFF|INBREG,
	SFLD,		TCHAR|TUCHAR,
	SBREG|SCON,	TCHAR|TUCHAR,
		NAREG|NBREG,	RDEST,
		"	movb AR,A2\n"
		"	movzbl A2,A1\n"
		"	andl $N,AL\n"
		"	sall $H,A1\n"
		"	andl $M,A1\n"
		"	orl A1,AL\n"
		"F	movb AR,AD\n"
		"FZE", },

{ ASSIGN,	FOREFF|INAREG,
	SFLD,		TSHORT|TUSHORT,
	SAREG|SCON,	TSHORT|TUSHORT,
		NAREG,	RDEST,
		"	movw AR,A1\n"
		"	movzwl A1,ZN\n"
		"	andl $N,AL\n"
		"	sall $H,ZN\n"
		"	andl $M,ZN\n"
		"	orl ZN,AL\n"
		"F	movw AR,AD\n"
		"FZE", },

{ ASSIGN,	FOREFF|INAREG,
	SFLD,		TWORD,
	SAREG|SNAME|SOREG|SCON,	TWORD,
		NAREG,	RDEST,
		"	movl AR,A1\n"
		"	andl $N,AL\n"
		"	sall $H,A1\n"
		"	andl $M,A1\n"
		"	orl A1,AL\n"
		"F	movl AR,AD\n"
		"FZE", },

{ ASSIGN,	INDREG|FOREFF,
	SHFL,	TFLOAT|TDOUBLE|TLDOUBLE,
	SHFL,	TFLOAT|TDOUBLE|TLDOUBLE,
		0,	RDEST,
		"", }, /* This will always be in the correct register */

/* order of table entries is very important here! */
{ ASSIGN,	INFL,
	SNAME|SOREG,	TLDOUBLE,
	SHFL,	TFLOAT|TDOUBLE|TLDOUBLE,
		0,	RDEST,
		"	fst AL\n", },

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TLDOUBLE,
	SHFL,	TFLOAT|TDOUBLE|TLDOUBLE,
		0,	0,
		"	fstpt AL\n", },

{ ASSIGN,	INFL,
	SNAME|SOREG,	TDOUBLE,
	SHFL,	TFLOAT|TDOUBLE|TLDOUBLE,
		0,	RDEST,
		"	fstl AL\n", },

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TDOUBLE,
	SHFL,	TFLOAT|TDOUBLE|TLDOUBLE,
		0,	0,
		"	fstpl AL\n", },

{ ASSIGN,	INFL,
	SNAME|SOREG,	TFLOAT,
	SHFL,	TFLOAT|TDOUBLE|TLDOUBLE,
		0,	RDEST,
		"	fsts AL\n", },

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TFLOAT,
	SHFL,	TFLOAT|TDOUBLE|TLDOUBLE,
		0,	0,
		"	fstps AL\n", },
/* end very important order */

{ ASSIGN,	INFL|FOREFF,
	SHFL,		TLDOUBLE,
	SHFL|SOREG|SNAME,	TLDOUBLE,
		0,	RDEST,
		"	fldt AR\n", },

{ ASSIGN,	INFL|FOREFF,
	SHFL,		TDOUBLE,
	SHFL|SOREG|SNAME,	TDOUBLE,
		0,	RDEST,
		"	fldl AR\n", },

{ ASSIGN,	INFL|FOREFF,
	SHFL,		TFLOAT,
	SHFL|SOREG|SNAME,	TFLOAT,
		0,	RDEST,
		"	flds AR\n", },

/* Do not generate memcpy if return from funcall */
#if 0
{ STASG,	INAREG|FOREFF,
	SOREG|SNAME|SAREG,	TPTRTO|TSTRUCT,
	SFUNCALL,	TPTRTO|TSTRUCT,
		0,	RRIGHT,
		"", },
#endif

{ STASG,	INAREG|FOREFF,
	SOREG|SNAME,	TANY,
	SAREG|SOREG|SNAME,	TPTRTO|TANY,
		NSPECIAL,	RRIGHT,
		"ZQ", },

/*
 * DIV/MOD/MUL 
 */
{ DIV,	INLL,
	SCREG,			TLONG|TLONGLONG,
	SCREG|SNAME|SOREG,	TLL,
		NSPECIAL,	RDEST,
		"	cltd\n	idivq AR\n", },

{ DIV,	INAREG,
	SAREG,			TULONG|TULONGLONG|TPOINT,
	SAREG|SNAME|SOREG,	TULONG|TULONGLONG|TPOINT,
		NSPECIAL,	RDEST,
		"	xorq %rdx,%rdx\n	divq AR\n", },

{ DIV,	INAREG,
	SAREG,			TSWORD,
	SAREG|SNAME|SOREG,	TWORD,
		NSPECIAL,	RDEST,
		"	cltd\n	idivl AR\n", },

{ DIV,	INAREG,
	SAREG,			TUWORD,
	SAREG|SNAME|SOREG,	TUWORD,
		NSPECIAL,	RDEST,
		"	xorl %edx,%edx\n	divl AR\n", },

{ DIV,	INAREG,
	SAREG,			TUSHORT,
	SAREG|SNAME|SOREG,	TUSHORT,
		NSPECIAL,	RDEST,
		"	xorl %edx,%edx\n	divw AR\n", },

{ DIV,	INCH,
	SHCH,			TUCHAR,
	SHCH|SNAME|SOREG,	TUCHAR,
		NSPECIAL,	RDEST,
		"	xorb %ah,%ah\n	divb AR\n", },

{ DIV,	INFL,
	SHFL,		TDOUBLE,
	SNAME|SOREG,	TDOUBLE,
		0,	RLEFT,
		"	fdivl AR\n", },

{ DIV,	INFL,
	SHFL,		TLDOUBLE|TDOUBLE|TFLOAT,
	SHFL,		TLDOUBLE|TDOUBLE|TFLOAT,
		0,	RLEFT,
		"	fdivZAp\n", },

{ MOD,	INLL,
	SCREG,			TLONG|TLONGLONG,
	SCREG|SNAME|SOREG,	TLONG|TLONGLONG,
		NCREG|NSPECIAL,	RESC1,
		"	cltd\n	idivq AR\n", },

{ MOD,	INLL,
	SCREG,			TLL|TPOINT,
	SCREG|SNAME|SOREG,	TULONG|TULONGLONG|TPOINT,
		NCREG|NSPECIAL,	RESC1,
		"	xorq %rdx,%rdx\n	divq AR\n", },

{ MOD,	INAREG,
	SAREG,			TSWORD,
	SAREG|SNAME|SOREG,	TSWORD,
		NAREG|NSPECIAL,	RESC1,
		"	cltd\n	idivl AR\n", },

{ MOD,	INAREG,
	SAREG,			TWORD,
	SAREG|SNAME|SOREG,	TUWORD,
		NAREG|NSPECIAL,	RESC1,
		"	xorl %edx,%edx\n	divl AR\n", },

{ MOD,	INAREG,
	SAREG,			TUSHORT,
	SAREG|SNAME|SOREG,	TUSHORT,
		NAREG|NSPECIAL,	RESC1,
		"	xorl %edx,%edx\n	divw AR\n", },

{ MOD,	INCH,
	SHCH,			TUCHAR,
	SHCH|SNAME|SOREG,	TUCHAR,
		NBREG|NSPECIAL,	RESC1,
		"	xorb %ah,%ah\n	divb AR\n", },

{ MUL,	INLL,
	SCREG,				TLL|TPOINT,
	SCREG|SNAME|SOREG|SCON,		TLL|TPOINT,
		0,	RLEFT,
		"	imulq AR,AL\n", },

{ MUL,	INAREG,
	SAREG,				TWORD,
	SAREG|SNAME|SOREG|SCON,		TWORD,
		0,	RLEFT,
		"	imull AR,AL\n", },

{ MUL,	INAREG,
	SAREG,			TSHORT|TUSHORT,
	SAREG|SNAME|SOREG,	TSHORT|TUSHORT,
		0,	RLEFT,
		"	imulw AR,AL\n", },

{ MUL,	INCH,
	SHCH,			TCHAR|TUCHAR,
	SHCH|SNAME|SOREG,	TCHAR|TUCHAR,
		NSPECIAL,	RDEST,
		"	imulb AR\n", },

{ MUL,	INFL,
	SHFL,		TDOUBLE,
	SNAME|SOREG,	TDOUBLE,
		0,	RLEFT,
		"	fmull AR\n", },

{ MUL,	INFL,
	SHFL,		TLDOUBLE|TDOUBLE|TFLOAT,
	SHFL,		TLDOUBLE|TDOUBLE|TFLOAT,
		0,	RLEFT,
		"	fmulp\n", },

/*
 * Indirection operators.
 */
{ UMUL,	INLL,
	SANY,	TANY,
	SOREG,	TLL|TPOINT,
		NCREG,	RESC1,
		"	movq AL,A1\n", },

{ UMUL,	INAREG,
	SANY,	TWORD,
	SOREG,	TWORD,
		NAREG|NASL,	RESC1,
		"	movl AL,A1\n", },

{ UMUL,	INCH,
	SANY,	TANY,
	SOREG,	TCHAR|TUCHAR,
		NBREG|NBSL,	RESC1,
		"	movb AL,A1\n", },

{ UMUL,	INAREG,
	SANY,	TANY,
	SOREG,	TSHORT|TUSHORT,
		NAREG|NASL,	RESC1,
		"	movw AL,A1\n", },

{ UMUL,	INFL,
	SANY,	TANY,
	SOREG,	TLDOUBLE,
		NDREG|NDSL,	RESC1,
		"	fldt AL\n", },

{ UMUL,	INFL,
	SANY,	TANY,
	SOREG,	TDOUBLE,
		NDREG|NDSL,	RESC1,
		"	fldl AL\n", },

{ UMUL,	INFL,
	SANY,	TANY,
	SOREG,	TFLOAT,
		NDREG|NDSL,	RESC1,
		"	flds AL\n", },

/*
 * Logical/branching operators
 */

/* Comparisions, take care of everything */
{ OPLOG,	FORCC,
	SCON,	TWORD,
	SHLL|SOREG|SNAME,	TLL|TPOINT,
		0, 	RESCC,
		"	cmpq AR,AL\n", },

{ OPLOG,	FORCC,
	SHLL,	TLL|TPOINT,
	SHLL|SOREG|SNAME,	TLL|TPOINT,
		0, 	RESCC,
		"	cmpq AR,AL\n", },

{ OPLOG,	FORCC,
	SHLL|SOREG|SNAME,	TLL|TPOINT,
	SHLL,			TLL|TPOINT,
		0,	RESCC,
		"	cmpq AR,AL\n", },

{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME,	TWORD,
	SCON|SAREG,	TWORD,
		0, 	RESCC,
		"	cmpl AR,AL\n", },

{ OPLOG,	FORCC,
	SCON|SAREG,	TWORD,
	SAREG|SOREG|SNAME,	TWORD,
		0, 	RESCC,
		"	cmpl AR,AL\n", },

{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME,	TSHORT|TUSHORT,
	SCON|SAREG,	TANY,
		0, 	RESCC,
		"	cmpw AR,AL\n", },

{ OPLOG,	FORCC,
	SBREG|SOREG|SNAME,	TCHAR|TUCHAR,
	SCON|SBREG,	TANY,
		0, 	RESCC,
		"	cmpb AR,AL\n", },

{ OPLOG,	FORCC,
	SDREG,	TLDOUBLE|TDOUBLE|TFLOAT,
	SDREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		NSPECIAL, 	0,
		"ZG", },

{ OPLOG,	FORCC,
	SOREG|SNAME,	TDOUBLE|TFLOAT,
	SDREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		NSPECIAL, 	0,
		"ZG", },

#if 0
/* Ppro and later only */
{ OPLOG,	FORCC,
	SDREG,	TLDOUBLE|TDOUBLE|TFLOAT,
	SDREG,	TLDOUBLE|TDOUBLE|TFLOAT,
		0, 	RESCC,
		"ZA	fucomip %st,%st(1)\n", },
#endif

{ OPLOG,	FORCC,
	SANY,	TANY,
	SANY,	TANY,
		REWRITE,	0,
		"diediedie!", },

/* AND/OR/ER/NOT */
{ AND,	INAREG|FOREFF,
	SCREG|SOREG|SNAME,	TLL,
	SCON,			TWORD,
		0,	RLEFT,
		"	andq AR,AL\n", },

{ AND,	INAREG|FOREFF,
	SCREG|SOREG|SNAME,	TLL,
	SCREG,			TLL,
		0,	RLEFT,
		"	andq AR,AL\n", },

{ AND,	INCREG|FOREFF,
	SCREG,			TLL,
	SCREG|SOREG|SNAME,	TLL,
		0,	RLEFT,
		"	andq AR,AL\n", },

{ AND,	INAREG|FOREFF,
	SAREG|SOREG|SNAME,	TWORD,
	SCON|SAREG,		TWORD,
		0,	RLEFT,
		"	andl AR,AL\n", },

{ AND,	INAREG|FOREFF,
	SAREG,			TWORD,
	SAREG|SOREG|SNAME,	TWORD,
		0,	RLEFT,
		"	andl AR,AL\n", },

{ AND,	INAREG|FOREFF,  
	SAREG|SOREG|SNAME,	TSHORT|TUSHORT,
	SCON|SAREG,		TSHORT|TUSHORT,
		0,	RLEFT,
		"	andw AR,AL\n", },

{ AND,	INAREG|FOREFF,  
	SAREG,			TSHORT|TUSHORT,
	SAREG|SOREG|SNAME,	TSHORT|TUSHORT,
		0,	RLEFT,
		"	andw AR,AL\n", },

{ AND,	INBREG|FOREFF,
	SBREG|SOREG|SNAME,	TCHAR|TUCHAR,
	SCON|SBREG,		TCHAR|TUCHAR,
		0,	RLEFT,
		"	andb AR,AL\n", },

{ AND,	INBREG|FOREFF,
	SBREG,			TCHAR|TUCHAR,
	SBREG|SOREG|SNAME,	TCHAR|TUCHAR,
		0,	RLEFT,
		"	andb AR,AL\n", },
/* AND/OR/ER/NOT */

/*
 * Jumps.
 */
{ GOTO, 	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"	jmp LL\n", },

#if defined(GCC_COMPAT) || defined(LANG_F77)
{ GOTO, 	FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"	jmp *AL\n", },
#endif

/*
 * Convert LTYPE to reg.
 */
{ OPLTYPE,	FORCC|INLL,
	SCREG,	TLL|TPOINT,
	SMIXOR,	TANY,
		NCREG,	RESC1,
		"	xorq A1,A1\n", },

{ OPLTYPE,	INLL,
	SANY,	TANY,
	SCREG|SCON|SOREG|SNAME,	TLL|TPOINT,
		NCREG,	RESC1,
		"	movq AL,A1\n", },

{ OPLTYPE,	FORCC|INAREG,
	SAREG,	TWORD,
	SMIXOR,	TANY,
		NAREG|NASL,	RESC1,
		"	xorl A1,A1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SCON|SOREG|SNAME,	TWORD,
		NAREG|NASL,	RESC1,
		"	movl AL,A1\n", },

{ OPLTYPE,	INBREG,
	SANY,	TANY,
	SBREG|SOREG|SNAME|SCON,	TCHAR|TUCHAR,
		NBREG,	RESC1,
		"	movb AL,A1\n", },

{ OPLTYPE,	FORCC|INAREG,
	SAREG,	TSHORT|TUSHORT,
	SMIXOR,	TANY,
		NAREG,	RESC1,
		"	xorw A1,A1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SOREG|SNAME|SCON,	TSHORT|TUSHORT,
		NAREG,	RESC1,
		"	movw AL,A1\n", },

{ OPLTYPE,	INDREG,
	SANY,		TLDOUBLE,
	SOREG|SNAME,	TLDOUBLE,
		NDREG,	RESC1,
		"	fldt AL\n", },

{ OPLTYPE,	INDREG,
	SANY,		TDOUBLE,
	SOREG|SNAME,	TDOUBLE,
		NDREG,	RESC1,
		"	fldl AL\n", },

{ OPLTYPE,	INDREG,
	SANY,		TFLOAT,
	SOREG|SNAME,	TFLOAT,
		NDREG,	RESC1,
		"	flds AL\n", },

/* Only used in ?: constructs. The stack already contains correct value */
{ OPLTYPE,	INDREG,
	SANY,	TFLOAT|TDOUBLE|TLDOUBLE,
	SDREG,	TFLOAT|TDOUBLE|TLDOUBLE,
		NDREG,	RESC1,
		"", },

/*
 * Negate a word.
 */

{ UMINUS,	INCREG|FOREFF,
	SCREG,	TLL|TPOINT,
	SCREG,	TLL|TPOINT,
		0,	RLEFT,
		"	negq AL\n", },

{ UMINUS,	INAREG|FOREFF,
	SAREG,	TWORD,
	SAREG,	TWORD,
		0,	RLEFT,
		"	negl AL\n", },

{ UMINUS,	INAREG|FOREFF,
	SAREG,	TSHORT|TUSHORT,
	SAREG,	TSHORT|TUSHORT,
		0,	RLEFT,
		"	negw AL\n", },

{ UMINUS,	INBREG|FOREFF,
	SBREG,	TCHAR|TUCHAR,
	SBREG,	TCHAR|TUCHAR,
		0,	RLEFT,
		"	negb AL\n", },

{ UMINUS,	INFL|FOREFF,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
		0,	RLEFT,
		"	fchs\n", },

{ COMPL,	INCREG,
	SCREG,	TLL,
	SANY,	TANY,
		0,	RLEFT,
		"	notq AL\n", },

{ COMPL,	INAREG,
	SAREG,	TWORD,
	SANY,	TANY,
		0,	RLEFT,
		"	notl AL\n", },

{ COMPL,	INAREG,
	SAREG,	TSHORT|TUSHORT,
	SANY,	TANY,
		0,	RLEFT,
		"	notw AL\n", },

{ COMPL,	INBREG,
	SBREG,	TCHAR|TUCHAR,
	SANY,	TANY,
		0,	RLEFT,
		"	notb AL\n", },

# define DF(x) FORREW,SANY,TANY,SANY,TANY,REWRITE,x,""

{ UMUL, DF( UMUL ), },

{ ASSIGN, DF(ASSIGN), },

{ STASG, DF(STASG), },

{ FLD, DF(FLD), },

{ OPLEAF, DF(NAME), },

/* { INIT, DF(INIT), }, */

{ OPUNARY, DF(UMINUS), },

{ OPANY, DF(BITYPE), },

{ FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	"help; I'm in trouble\n" },
};

int tablesize = sizeof(table)/sizeof(table[0]);
