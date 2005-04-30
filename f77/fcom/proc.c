/*	$Id: proc.c,v 1.5 2005/04/30 07:55:35 ragge Exp $	*/
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
#include "defs.h"

LOCAL void doentry(struct entrypoint *ep);
LOCAL void retval(int t);
LOCAL void epicode(void);
LOCAL void procode(void);
LOCAL int nextarg(int);
LOCAL int nextarg(int);
LOCAL void dobss(void);
LOCAL void docommon(void);
LOCAL void docomleng(void);


/* start a new procedure */

void
newproc()
{
if(parstate != OUTSIDE)
	{
	execerr("missing end statement");
	endproc();
	}

parstate = INSIDE;
procclass = CLMAIN;	/* default */
}



/* end of procedure. generate variables, epilogs, and prologs */

void
endproc()
{
struct labelblock *lp;

if(parstate < INDATA)
	enddcl();
if(ctlstack >= ctls)
	err("DO loop or BLOCK IF not closed");
for(lp = labeltab ; lp < labtabend ; ++lp)
	if(lp->stateno!=0 && lp->labdefined==NO)
		err1("missing statement number %s", convic(lp->stateno) );

epicode();
procode();
dobss();
prdbginfo();

#if FAMILY == SCJ
	putbracket();
#endif

procinit();	/* clean up for next procedure */
}



/* End of declaration section of procedure.  Allocate storage. */

void
enddcl()
{
register struct entrypoint *p;

parstate = INEXEC;
docommon();
doequiv();
docomleng();
#if TARGET == PDP11
/* fake jump to start the optimizer */
if(procclass != CLBLOCK)
	putgoto( fudgelabel = newlabel() );
#endif
for(p = entries ; p ; p = p->nextp)
	doentry(p);
}

/* ROUTINES CALLED WHEN ENCOUNTERING ENTRY POINTS */

/* Main program or Block data */

void
startproc(progname, class)
struct extsym * progname;
int class;
{
register struct entrypoint *p;

p = ALLOC(entrypoint);
if(class == CLMAIN)
	puthead("MAIN__");
else
	puthead(NULL);
if(class == CLMAIN)
	newentry( mkname(5, "MAIN_") );
p->entryname = progname;
p->entrylabel = newlabel();
entries = p;

procclass = class;
retlabel = newlabel();
fprintf(diagfile, "   %s", (class==CLMAIN ? "MAIN" : "BLOCK DATA") );
if(progname)
	fprintf(diagfile, " %s", nounder(XL, procname = progname->extname) );
fprintf(diagfile, ":\n");
}

/* subroutine or function statement */

struct extsym *newentry(v)
#ifdef NEWSTR
register struct bigblock *v;
#else
register struct nameblock *v;
#endif
{
register struct extsym *p;

p = mkext( varunder(VL, v->b_name.varname) );

if(p==NULL || p->extinit || ! ONEOF(p->extstg, M(STGUNKNOWN)|M(STGEXT)) )
	{
	if(p == 0)
		dclerr("invalid entry name", v);
	else	dclerr("external name already used", v);
	return(0);
	}
v->vstg = STGAUTO;
v->b_name.vprocclass = PTHISPROC;
v->vclass = CLPROC;
p->extstg = STGEXT;
p->extinit = YES;
return(p);
}

void
entrypt(class, type, length, entry, args)
int class, type;
ftnint length;
struct extsym *entry;
chainp args;
{
#ifdef NEWSTR
register struct bigblock *q;
#else
register struct nameblock *q;
#endif
register struct entrypoint *p;

if(class != CLENTRY)
	puthead( varstr(XL, procname = entry->extname) );
if(class == CLENTRY)
	fprintf(diagfile, "       entry ");
fprintf(diagfile, "   %s:\n", nounder(XL, entry->extname));
q = mkname(VL, nounder(XL,entry->extname) );

if( (type = lengtype(type, (int) length)) != TYCHAR)
	length = 0;
if(class == CLPROC)
	{
	procclass = CLPROC;
	proctype = type;
	procleng = length;

	retlabel = newlabel();
	if(type == TYSUBR)
		ret0label = newlabel();
	}

p = ALLOC(entrypoint);
entries = hookup(entries, p);
p->entryname = entry;
p->arglist = args;
p->entrylabel = newlabel();
p->enamep = q;

if(class == CLENTRY)
	{
	class = CLPROC;
	if(proctype == TYSUBR)
		type = TYSUBR;
	}

q->vclass = class;
q->b_name.vprocclass = PTHISPROC;
settype(q, type, (int) length);
/* hold all initial entry points till end of declarations */
if(parstate >= INDATA)
	doentry(p);
}

/* generate epilogs */

int multitypes = 0; /* XXX */

LOCAL void
epicode()
{
register int i;

if(procclass==CLPROC)
	{
	if(proctype==TYSUBR)
		{
		putlabel(ret0label);
		if(substars)
			putforce(TYINT, ICON(0) );
		putlabel(retlabel);
		goret(TYSUBR);
		}
	else	{
		putlabel(retlabel);
		if(multitypes)
			{
			typeaddr = autovar(1, TYADDR, NULL);
			putbranch( cpexpr(typeaddr) );
			for(i = 0; i < NTYPES ; ++i)
				if(rtvlabel[i] != 0)
					{
					putlabel(rtvlabel[i]);
					retval(i);
					}
			}
		else
			retval(proctype);
		}
	}

else if(procclass != CLBLOCK)
	{
	putlabel(retlabel);
	goret(TYSUBR);
	}
}


/* generate code to return value of type  t */

LOCAL void
retval(t)
register int t;
{
#ifdef NEWSTR
register struct bigblock *p;
#else
register struct addrblock *p;
#endif

switch(t)
	{
	case TYCHAR:
	case TYCOMPLEX:
	case TYDCOMPLEX:
		break;

	case TYLOGICAL:
		t = tylogical;
	case TYADDR:
	case TYSHORT:
	case TYLONG:
		p = cpexpr(retslot);
		p->vtype = t;
		putforce(t, p);
		break;

	case TYREAL:
	case TYDREAL:
		p = cpexpr(retslot);
		p->vtype = t;
		putforce(t, p);
		break;

	default:
		fatal1("retval: impossible type %d", t);
	}
goret(t);
}


/* Allocate extra argument array if needed. Generate prologs. */

LOCAL void
procode()
{
register struct entrypoint *p;
struct addrblock *argvec;

#if TARGET==GCOS
	argvec = autovar(lastargslot/FSZADDR, TYADDR, NULL);
#else
	if(lastargslot>0 && nentry>1)
		argvec = autovar(lastargslot/FSZADDR, TYADDR, NULL);
	else
		argvec = NULL;
#endif


#if TARGET == PDP11
	/* for the optimizer */
	if(fudgelabel)
		putlabel(fudgelabel);
#endif

for(p = entries ; p ; p = p->nextp)
	prolog(p, argvec);

#if FAMILY == SCJ
	putrbrack(procno);
#endif

prendproc();
}

/*
   manipulate argument lists (allocate argument slot positions)
 * keep track of return types and labels
 */

LOCAL void
doentry(ep)
struct entrypoint *ep;
{
register int type;
#ifdef NEWSTR
register struct bigblock *np, *q;
#else
register struct nameblock *np;
register struct nameblock *q;
#endif
chainp p;

++nentry;
if(procclass == CLMAIN)
	{
	putlabel(ep->entrylabel);
	return;
	}
else if(procclass == CLBLOCK)
	return;

impldcl( np = mkname(VL, nounder(XL, ep->entryname->extname) ) );
type = np->vtype;
if(proctype == TYUNKNOWN)
	if( (proctype = type) == TYCHAR)
#ifdef NEWSTR
		procleng = (np->vleng ? np->vleng->b_const.fconst.ci : (ftnint) 0);
#else
		procleng = (np->vleng ? np->vleng->constblock.fconst.ci : (ftnint) 0);
#endif

if(proctype == TYCHAR)
	{
	if(type != TYCHAR)
		err("noncharacter entry of character function");
#ifdef NEWSTR
	else if( (np->vleng ? np->vleng->b_const.fconst.ci : (ftnint) 0) != procleng)
#else
	else if( (np->vleng ? np->vleng->constblock.fconst.ci : (ftnint) 0) != procleng)
#endif
		err("mismatched character entry lengths");
	}
else if(type == TYCHAR)
	err("character entry of noncharacter function");
else if(type != proctype)
	multitype = YES;
if(rtvlabel[type] == 0)
	rtvlabel[type] = newlabel();
ep->typelabel = rtvlabel[type];

if(type == TYCHAR)
	{
	if(chslot < 0)
		{
		chslot = nextarg(TYADDR);
		chlgslot = nextarg(TYLENG);
		}
	np->vstg = STGARG;
#ifdef NEWSTR
	np->b_name.vardesc.varno = chslot;
#else
	np->vardesc.varno = chslot;
#endif
	if(procleng == 0)
		np->vleng = mkarg(TYLENG, chlgslot);
	}
else if( ISCOMPLEX(type) )
	{
	np->vstg = STGARG;
	if(cxslot < 0)
		cxslot = nextarg(TYADDR);
#ifdef NEWSTR
	np->b_name.vardesc.varno = cxslot;
#else
	np->vardesc.varno = cxslot;
#endif
	}
else if(type != TYSUBR)
	{
	if(nentry == 1)
		retslot = autovar(1, TYDREAL, NULL);
	np->vstg = STGAUTO;
#ifdef NEWSTR
	np->b_name.voffset = retslot->memoffset->b_const.fconst.ci;
#else
	np->voffset = retslot->memoffset->constblock.fconst.ci;
#endif
	}

for(p = ep->arglist ; p ; p = p->chain.nextp)
#ifdef NEWSTR
	if(! ((q = p->chain.datap)->b_name.vdcldone) )
		q->b_name.vardesc.varno = nextarg(TYADDR);
#else
	if(! ((q = p->chain.datap)->vdcldone) )
		q->vardesc.varno = nextarg(TYADDR);
#endif

for(p = ep->arglist ; p ; p = p->chain.nextp)
	if(! ((q = p->chain.datap)->b_name.vdcldone) )
		{
		impldcl(q);
		q->b_name.vdcldone = YES;
		if(q->vtype == TYCHAR)
			{
			if(q->vleng == NULL)	/* character*(*) */
				q->vleng = mkarg(TYLENG, nextarg(TYLENG) );
			else if(nentry == 1)
				nextarg(TYLENG);
			}
		else if(q->vclass==CLPROC && nentry==1)
			nextarg(TYLENG) ;
		}

putlabel(ep->entrylabel);
}



LOCAL int
nextarg(type)
int type;
{
int k;
k = lastargslot;
lastargslot += typesize[type];
return(k);
}

/* generate variable references */

LOCAL void
dobss()
{
register struct hashentry *p;
register struct bigblock *q;
register int i;
int align;
ftnint leng, iarrl;

pruse(asmfile, USEBSS);

for(p = hashtab ; p<lasthash ; ++p)
    if((q = p->varp))
	{
	if( (q->vclass==CLUNKNOWN && q->vstg!=STGARG) ||
	    (q->vclass==CLVAR && q->vstg==STGUNKNOWN) )
		warn1("local variable %s never used", varstr(VL,q->b_name.varname) );
	else if(q->vclass==CLVAR && q->vstg==STGBSS)
		{
		align = (q->vtype==TYCHAR ? ALILONG : typealign[q->vtype]);
		if(bssleng % align != 0)
			{
			bssleng = roundup(bssleng, align);
			preven(align);
			}
		prlocvar( memname(STGBSS, q->b_name.vardesc.varno), iarrl = iarrlen(q) );
		bssleng += iarrl;
		}
	else if(q->vclass==CLPROC && q->b_name.vprocclass==PEXTERNAL && q->vstg!=STGARG)
		mkext(varunder(VL, q->b_name.varname)) ->extstg = STGEXT;

	if(q->vclass==CLVAR && q->vstg!=STGARG)
		{
		if(q->b_name.vdim && !ISICON(q->b_name.vdim->nelt) )
			dclerr("adjustable dimension on non-argument", q);
		if(q->vtype==TYCHAR && (q->vleng==NULL || !ISICON(q->vleng)))
			dclerr("adjustable leng on nonargument", q);
		}
	}

for(i = 0 ; i < nequiv ; ++i)
	if(eqvclass[i].eqvinit==NO && (leng = eqvclass[i].eqvleng)!=0 )
		{
		bssleng = roundup(bssleng, ALIDOUBLE);
		preven(ALIDOUBLE);
		prlocvar( memname(STGEQUIV, i), leng);
		bssleng += leng;
		}
}



void
doext()
{
struct extsym *p;

for(p = extsymtab ; p<nextext ; ++p)
	prext( varstr(XL, p->extname), p->maxleng, p->extinit);
}




ftnint iarrlen(q)
register struct bigblock *q;
{
ftnint leng;

leng = typesize[q->vtype];
if(leng <= 0)
	return(-1);
if(q->b_name.vdim) {
	if( ISICON(q->b_name.vdim->nelt) )
		leng *= q->b_name.vdim->nelt->b_const.fconst.ci;
	else	return(-1);
}
if(q->vleng) {
	if( ISICON(q->vleng) )
		leng *= q->vleng->b_const.fconst.ci;
	else 	return(-1);
}
return(leng);
}

LOCAL void 
docommon()
{
register struct extsym *p;
register chainp q;
struct dimblock *t;
bigptr neltp;
register struct bigblock *v;
ftnint size;
int type;

for(p = extsymtab ; p<nextext ; ++p)
	if(p->extstg==STGCOMMON)
		{
		for(q = p->extp ; q ; q = q->chain.nextp)
			{
			v = q->chain.datap;
			if(v->b_name.vdcldone == NO)
				vardcl(v);
			type = v->vtype;
			if(p->extleng % typealign[type] != 0)
				{
				dclerr("common alignment", v);
				p->extleng = roundup(p->extleng, typealign[type]);
				}
			v->b_name.voffset = p->extleng;
			v->b_name.vardesc.varno = p - extsymtab;
			if(type == TYCHAR)
				size = v->vleng->b_const.fconst.ci;
			else	size = typesize[type];
			if((t = v->b_name.vdim)) {
				if( (neltp = t->nelt) && ISCONST(neltp) )
					size *= neltp->b_const.fconst.ci;
				else
					dclerr("adjustable array in common", v);
			}
			p->extleng += size;
			}

		frchain( &(p->extp) );
		}
}





LOCAL void
docomleng()
{
register struct extsym *p;

for(p = extsymtab ; p < nextext ; ++p)
	if(p->extstg == STGCOMMON)
		{
		if(p->maxleng!=0 && p->extleng!=0 && p->maxleng!=p->extleng &&
		    !eqn(XL,"_BLNK__ ",p->extname) )
			warn1("incompatible lengths for common block %s",
				nounder(XL, p->extname) );
		if(p->maxleng < p->extleng)
			p->maxleng = p->extleng;
		p->extleng = 0;
	}
}




/* ROUTINES DEALING WITH AUTOMATIC AND TEMPORARY STORAGE */
void
frtemp(p)
struct bigblock *p;
{
holdtemps = mkchain(p, holdtemps);
}




/* allocate an automatic variable slot */

struct bigblock *autovar(nelt, t, lengp)
register int nelt, t;
bigptr lengp;
{
ftnint leng;
register struct bigblock *q;

if(t == TYCHAR)
	if( ISICON(lengp) )
		leng = lengp->b_const.fconst.ci;
	else	{
		fatal("automatic variable of nonconstant length");
		}
else
	leng = typesize[t];
autoleng = roundup( autoleng, typealign[t]);

#ifdef NEWSTR
q = BALLO();
#else
q = ALLOC(addrblock);
#endif
q->tag = TADDR;
q->vtype = t;
if(t == TYCHAR)
	q->vleng = ICON(leng);
q->vstg = STGAUTO;
q->b_addr.ntempelt = nelt;
#if TARGET==PDP11 || TARGET==VAX
	/* stack grows downward */
	autoleng += nelt*leng;
	q->b_addr.memoffset = ICON( - autoleng );
#else
	q->b_addr.memoffset = ICON( autoleng );
	autoleng += nelt*leng;
#endif

return(q);
}


struct bigblock *mktmpn(nelt, type, lengp)
int nelt;
register int type;
bigptr lengp;
{
ftnint leng;
chainp p, oldp;
register struct bigblock *q;

if(type==TYUNKNOWN || type==TYERROR)
	fatal1("mktmpn: invalid type %d", type);

if(type==TYCHAR) {
	if( ISICON(lengp) )
		leng = lengp->b_const.fconst.ci;
	else	{
		err("adjustable length");
		return( errnode() );
		}
}
for(oldp = &templist ; (p = oldp->chain.nextp) ; oldp = p)
	{
	q = p->chain.datap;
	if(q->vtype==type && q->b_addr.ntempelt==nelt &&
	    (type!=TYCHAR || q->vleng->b_const.fconst.ci==leng) )
		{
		oldp->chain.nextp = p->chain.nextp;
		free(p);
		return(q);
		}
	}
q = autovar(nelt, type, lengp);
q->b_addr.istemp = YES;
return(q);
}




struct bigblock *fmktemp(type, lengp)
int type;
bigptr lengp;
{
return( mktmpn(1,type,lengp) );
}

/* VARIOUS ROUTINES FOR PROCESSING DECLARATIONS */

struct extsym *comblock(len, s)
register int len;
register char *s;
{
struct extsym *p;

if(len == 0)
	{
	s = BLANKCOMMON;
	len = strlen(s);
	}
p = mkext( varunder(len, s) );
if(p->extstg == STGUNKNOWN)
	p->extstg = STGCOMMON;
else if(p->extstg != STGCOMMON)
	{
	err1("%s cannot be a common block name", s);
	return(0);
	}

return( p );
}

void
incomm(c, v)
struct extsym *c;
struct bigblock *v;
{
if(v->vstg != STGUNKNOWN)
	dclerr("incompatible common declaration", v);
else
	{
	v->vstg = STGCOMMON;
	c->extp = hookup(c->extp, mkchain(v,NULL) );
	}
}



void
settype(v, type, length)
register struct bigblock * v;
register int type;
register int length;
{
if(type == TYUNKNOWN)
	return;

if(type==TYSUBR && v->vtype!=TYUNKNOWN && v->vstg==STGARG)
	{
	v->vtype = TYSUBR;
	frexpr(v->vleng);
	}
else if(type < 0)	/* storage class set */
	{
	if(v->vstg == STGUNKNOWN)
		v->vstg = - type;
	else if(v->vstg != -type)
		dclerr("incompatible storage declarations", v);
	}
else if(v->vtype == TYUNKNOWN)
	{
	if( (v->vtype = lengtype(type, length))==TYCHAR && length!=0)
		v->vleng = ICON(length);
	}
else if(v->vtype!=type || (type==TYCHAR && v->vleng->b_const.fconst.ci!=length) )
	dclerr("incompatible type declarations", v);
}




int
lengtype(type, length)
register int type;
register int length;
{
switch(type)
	{
	case TYREAL:
		if(length == 8)
			return(TYDREAL);
		if(length == 4)
			goto ret;
		break;

	case TYCOMPLEX:
		if(length == 16)
			return(TYDCOMPLEX);
		if(length == 8)
			goto ret;
		break;

	case TYSHORT:
	case TYDREAL:
	case TYDCOMPLEX:
	case TYCHAR:
	case TYUNKNOWN:
	case TYSUBR:
	case TYERROR:
		goto ret;

	case TYLOGICAL:
		if(length == 4)
			goto ret;
		break;

	case TYLONG:
		if(length == 0)
			return(tyint);
		if(length == 2)
			return(TYSHORT);
		if(length == 4)
			goto ret;
		break;
	default:
		fatal1("lengtype: invalid type %d", type);
	}

if(length != 0)
	err("incompatible type-length combination");

ret:
	return(type);
}




void
setintr(v)
register struct bigblock * v;
{
register int k;

if(v->vstg == STGUNKNOWN)
	v->vstg = STGINTR;
else if(v->vstg!=STGINTR)
	dclerr("incompatible use of intrinsic function", v);
if(v->vclass==CLUNKNOWN)
	v->vclass = CLPROC;
if(v->b_name.vprocclass == PUNKNOWN)
	v->b_name.vprocclass = PINTRINSIC;
else if(v->b_name.vprocclass != PINTRINSIC)
	dclerr("invalid intrinsic declaration", v);
if((k = intrfunct(v->b_name.varname)))
	v->b_name.vardesc.varno = k;
else
	dclerr("unknown intrinsic function", v);
}


void
setext(v)
register struct bigblock * v;
{
if(v->vclass == CLUNKNOWN)
	v->vclass = CLPROC;
else if(v->vclass != CLPROC)
	dclerr("invalid external declaration", v);

if(v->b_name.vprocclass == PUNKNOWN)
	v->b_name.vprocclass = PEXTERNAL;
else if(v->b_name.vprocclass != PEXTERNAL)
	dclerr("invalid external declaration", v);
}




/* create dimensions block for array variable */
void
setbound(v, nd, dims)
register struct bigblock * v;
int nd;
struct uux dims[ ];
{
register bigptr q, t;
register struct dimblock *p;
int i;

if(v->vclass == CLUNKNOWN)
	v->vclass = CLVAR;
else if(v->vclass != CLVAR)
	{
	dclerr("only variables may be arrays", v);
	return;
	}

v->b_name.vdim = p = (struct dimblock *) ckalloc( sizeof(int) + (3+2*nd)*sizeof(bigptr) );
p->ndim = nd;
p->nelt = ICON(1);

for(i=0 ; i<nd ; ++i)
	{
	if( (q = dims[i].ub) == NULL)
		{
		if(i == nd-1)
			{
			frexpr(p->nelt);
			p->nelt = NULL;
			}
		else
			err("only last bound may be asterisk");
		p->dims[i].dimsize = ICON(1);;
		p->dims[i].dimexpr = NULL;
		}
	else
		{
		if(dims[i].lb)
			{
			q = mkexpr(OPMINUS, q, cpexpr(dims[i].lb));
			q = mkexpr(OPPLUS, q, ICON(1) );
			}
		if( ISCONST(q) )
			{
			p->dims[i].dimsize = q;
			p->dims[i].dimexpr = NULL;
			}
		else	{
			p->dims[i].dimsize = autovar(1, tyint, NULL);
			p->dims[i].dimexpr = q;
			}
		if(p->nelt)
			p->nelt = mkexpr(OPSTAR, p->nelt, cpexpr(p->dims[i].dimsize));
		}
	}

q = dims[nd-1].lb;
if(q == NULL)
	q = ICON(1);

for(i = nd-2 ; i>=0 ; --i)
	{
	t = dims[i].lb;
	if(t == NULL)
		t = ICON(1);
	if(p->dims[i].dimsize)
		q = mkexpr(OPPLUS, t, mkexpr(OPSTAR, cpexpr(p->dims[i].dimsize), q) );
	}

if( ISCONST(q) )
	{
	p->baseoffset = q;
	p->basexpr = NULL;
	}
else
	{
	p->baseoffset = autovar(1, tyint, NULL);
	p->basexpr = q;
	}
}
