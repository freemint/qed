#include "global.h"
#include "qed.h"
#include "memory.h"
/* Heiko */
#include "hl.h"

#undef MAGIC
#define MAGIC		0xFED1
#define MAX_ANZ		262 /*261*/ /* 132 */	/* 67 */ 	
#define BLOCK_SIZE	(MAX_ANZ * 4)
#define BLOCKANZ	50 /* 100 */  /* 200 */		/* Fr einen malloc */
#define MEMSIZE		((long)BLOCK_SIZE * BLOCKANZ)	/* fr einen malloc */

#define MEM_SIZE(x)	(((x)+2+3)&(~3))						/* 2 Bytes drauf fr den Kopf und auf long glattmachen */
#define MEM_ADD(x,d)	(BLOCK*)((char*)(x)+(d))
#define MEM_SUB(x,d)	(BLOCK*)((char*)(x)-(d))

typedef union tblock
{
	struct
	{
		unsigned short magic;		/* 2 Bytes */
		unsigned short size;		/* 2 Bytes */
		union tblock *next;		/* 4 Bytes */
		union tblock *prev;		/* 4 Bytes => 12 Bytes */
	} free;
	struct
	{
		unsigned short size;		/* 2 Bytes */
	} used;
} BLOCK;

static bool	mem_need_BS,			/* BS hat keinen Speicher mehr */
		mem_need_TQ;			/* TQ hat keinen Speicher mehr */

static BLOCK *	free_list[MAX_ANZ+1+1];		/* ein Dummy am Ende */
static void *	block_list;


static bool new_block(void)
{
	BLOCK *adr, *adr2, *pre;
	long  *ptr;
	long	anz, i;

	anz = (long) Malloc(-1L);
	if (anz >= 4 + MEMSIZE + 4)
	{
		ptr = (long *) Malloc(4 + MEMSIZE + 4);
		anz -= 4 + MEMSIZE + 4;
	}
	else
		ptr = NULL;

	if (anz < MEMSIZE + 20000L)
		mem_need_BS = TRUE;

	if (ptr == NULL)
	{
		if (anz >= 4 + BLOCK_SIZE + 4)
		{
			i = anz = (anz - 8)/(BLOCK_SIZE);
			anz = 4 + (anz * BLOCK_SIZE) + 4;
			ptr = (long *) Malloc(anz);
		}
		else
		{
			mem_need_TQ = TRUE;
			mem_need_BS = TRUE;
			return FALSE;
		}
		mem_need_TQ = TRUE;
	}
	else
	{
		i = BLOCKANZ;
		mem_need_TQ = FALSE;
	}

	*(void **)ptr = block_list;			/* In Liste einhngen */
	block_list = ptr;

	adr = MEM_ADD(ptr,4);
	free_list[MAX_ANZ] = adr;
	pre = NULL;
	while ((--i) > 0)
	{
		adr2 = MEM_ADD(adr, BLOCK_SIZE);
		*(long*)&adr->free.magic = (long)(MAGIC << 16) + BLOCK_SIZE;
		/*	adr->free.magic = MAGIC;	*/
		/*	adr->free.size = BLOCK_SIZE;	*/
		adr->free.next = adr2;
		adr->free.prev = pre;
		pre = adr;
		adr = adr2;
	}
	adr2 = MEM_ADD(adr, BLOCK_SIZE);
	*(long*)&adr->free.magic = (long)(MAGIC<<16)+BLOCK_SIZE;
	/*	adr->free.magic = MAGIC;	*/
	/*	adr->free.size = BLOCK_SIZE;	*/
	adr->free.next = NULL;
	adr->free.prev = pre;

	*(long*)adr2 = 0L;			/* damit bei FREE da kein MAGIC steht */

	return TRUE;
}

static void *
MALLOC(unsigned short size)
{
	BLOCK *adr, **feld;

	size = MEM_SIZE(size);
	if (size > BLOCK_SIZE)
	{
		debug("\aMalloc: size= %d (> %d)\n", size, BLOCK_SIZE);
		note(1, 0, FATALMEM);
		return NULL;
	}

	feld = (BLOCK**)((char*)free_list + size);
	if (*feld == NULL)						/* kein passender */
	{
		if (size + 16 <= BLOCK_SIZE)				/* greren Block suchen und teilen */
		{
			BLOCK *adr2;
			short	i;

			i = size + 16; feld += 4;			/* 16 Bytes kleinster Block zum Abspalten */
			while (TRUE)					/* greren Block suchen */
			{
				if (*feld++ != NULL) 
					break;
				i += 4;
			}
			feld--;
			if (i == (BLOCK_SIZE + 4))			/* kein Block vorhanden */
			{
				if (!new_block())
					return NULL;
				feld--;
				i -= 4;
			}
			adr = *feld;					/* Zeiger auf Block */
			*feld = adr->free.next;				/* Aushngen */
			if (*feld != NULL)
				(*feld)->free.prev = NULL;
			adr->used.size = size;

			/* Rest in die free_list einhngen */
			adr2 = MEM_ADD(adr,size);			/* Zeiger auf Restblock */
			(char*)feld -= size;
			i -= size;					/* Restgre */
			adr2->free.magic = MAGIC;			/* Einhngen */
			adr2->free.size = i;
			adr2->free.next = *feld;
			adr2->free.prev = NULL;
			if (*feld != NULL) (*feld)->free.prev = adr2;
			*feld = adr2;
		}
		else							/* grten Block nehmen */
		{
			feld = (BLOCK**)(&free_list[MAX_ANZ]);
			if (*feld == NULL)				/* kein Block vorhanden */
				if (!new_block())
					return NULL;
			adr = *feld;					/* Zeiger auf Block */
			*feld = adr->free.next;				/* Aushngen */
			if (*feld!=NULL) (*feld)->free.prev = NULL;
			adr->used.size = BLOCK_SIZE;
		}
	}
	else								/* passend vorhanden */
	{
		adr = *feld;						/* Zeiger auf Block */
		*feld = adr->free.next;					/* Aushngen */
		if (*feld != NULL) (*feld)->free.prev = NULL;
		adr->used.size = size;
	}
	
	if (free_list[MAX_ANZ]==NULL)
		mem_need_TQ = TRUE;
	
	return(MEM_ADD(adr,2));
}

static void FREE(void *adr)
{
	short	size1, size2;
	BLOCK *adr1, *adr2, **feld;

	adr1 = MEM_SUB(adr,2);						/* adr1 auf ersten Block */
	size1 = adr1->used.size;
	adr2 = MEM_ADD(adr1,size1);					/* adr2 auf Nachfolger */
	size2 = adr2->free.size;
	if (adr2->free.magic==MAGIC && (size1+size2<=BLOCK_SIZE))
	/* freier Block dahinter => Zusammenfgen */
	{
		BLOCK	*pre;

		pre = adr2->free.prev;						/* Aushngen */
		if (pre==NULL)							/* erster in der Liste */
			*(BLOCK**)((char*)free_list+size2) = adr2->free.next;
		else
			pre->free.next = adr2->free.next;
		if (adr2->free.next!=NULL)					/* es gibt Nachfolger */
			adr2->free.next->free.prev = pre;
		size1 += size2;
	}
	feld = (BLOCK**)((char*)free_list+size1);				/* Einhngen */
	adr1->free.magic = MAGIC;
	adr1->free.size = size1;
	adr1->free.next = *feld;
	adr1->free.prev = NULL;
	if (*feld!=NULL) (*feld)->free.prev = adr1;
	*feld = adr1;

	if (size1 == BLOCK_SIZE)
		mem_need_TQ = FALSE;
}

/* =========================================================== */

void INSERT(LINEP *a, short pos, short delta, char *str)
{
	char	*ptr;
	
	ptr = REALLOC(a,pos,delta);
	memcpy(ptr, str, delta);
}

/* =========================================================== */

char *
REALLOC(LINEP *a, short pos, short delta)
{
	LINEP	col;
	short		new_len;
	BLOCK 	*adr;

	col = *a;
	if (delta == 0) 
		return (TEXT(col) + pos);

	new_len = col->len + delta;
	if (new_len < 0 || new_len > MAX_LINE_LEN)
	{
		debug("\aREALLOC: new_len= %d (> %d)\n", new_len, MAX_LINE_LEN);
		note(1, 0, FATALMEM);
		return NULL;
	}
	adr = MEM_SUB(col, 2);
	if (MEM_SIZE((unsigned short)sizeof(LINE)+new_len+1) != adr->used.size)
	{
		LINEP	new;

		new = MALLOC( (short) sizeof(LINE) + new_len + 1);
		if (new == NULL)
			return NULL;
		memcpy(new, col, sizeof(LINE)+pos);
		if (delta<0)
		{
			new->len = new_len;
			memcpy(TEXT(new) + pos, TEXT(col) + pos - delta, new_len - pos + 1);
		}
		else
		{
			memcpy(TEXT(new) + pos + delta, TEXT(col) + pos, col->len - pos + 1);
			new->len = new_len;
		}
		new->is_longest = col->is_longest;
		FREE(col);
		new->prev->next = new;
		new->next->prev = new;
		*a = new;
		new->exp_len = -1;
		return(TEXT(new)+pos);
	}
	else	/* Der Platzt in der Zeile reicht */
	{
		if (delta < 0)
		{
			col->len = new_len;
			memcpy(TEXT(col) + pos, TEXT(col) + pos - delta, new_len - pos + 1);
		}
		else
		{
			memmove(TEXT(col) + pos + delta, TEXT(col) + pos, col->len - pos + 1);
			col->len = new_len;
		}
		col->exp_len = -1;
		return(TEXT(col)+pos);
	}
}

LINEP new_col(char *str, short l)
{
	LINEP 	a;
	
	a = (LINEP)MALLOC( (short)sizeof(LINE)+l+1);
	if (a != NULL)
	{
		a->info = 0;
		a->len = l;
		a->exp_len = -1;
		a->is_longest = FALSE;
		a->hl_handle = NULL;
		memcpy(TEXT(a), str, l);
		TEXT(a)[l] = EOS;
	}
	return(a);
}

void free_col(RINGP rp, LINEP col)
{
	FREE(col);
}

/* Zeile nach WO einfgen	  */
LINEP col_insert(RINGP rp, LINEP wo, LINEP was)
{
	LINEP help;

	help = wo->next;
	help->prev = was;
	was->next = help;
	was->prev = wo;
	wo->next = was;
	if (rp)
		hl_insert( rp, was );
	return was;
}

/* Zeile am Ende anhngen */
void col_append(RINGP rp, LINEP was)
{
	LINEP help;

	help = LAST(rp);
	was->prev = help;
	was->next = help->next;
	help->next = was;
	rp->tail.prev = was;
	rp->lines++;
	hl_insert( rp, was );
}

void col_delete(RINGP rp, LINEP was)
{
	hl_remove( rp, was );
	was->prev->next = was->next;
	was->next->prev = was->prev;
	FREE(was);
	rp->lines--;
}

void col_concate(RINGP rp, LINEP *wo)
{
	LINEP	help, col;
	bool	absatz;

	col = *wo;
	help = col->next;
	if (col->len)
	{
		absatz = help->info&ABSATZ;
		INSERT(&col, col->len, help->len, TEXT(help));
		help->next->prev = col;
		col->next = help->next;
		if (rp) {
			hl_remove( rp, help );
			hl_update_zeile( rp, col );
		}
		FREE(help);
		if (absatz)
			col->info |= ABSATZ;
		else
			col->info &= ~ABSATZ;
		*wo = col;
	}
	else
	{
		col->prev->next = help;
		help->prev = col->prev;
		if (rp)
			hl_remove( rp, col );
		FREE(col);
		*wo = help;
	}
}

void col_split(RINGP rp, LINEP *col,short pos)
{
	LINEP	new,help;
	short	anz;
	bool	absatz, overlen;

	help = *col;
	absatz = IS_ABSATZ(help);
	help->info &= ~ABSATZ;
	overlen = IS_OVERLEN(help);
	help->info &= ~OVERLEN;
	if (pos==0)
	{
		new = new_col("",0);
		col_insert(rp, help->prev, new);
		*col = new;
	}
	else if (pos<help->len)
	{
		if (rp)
			hl_remove( rp, help );
		anz = help->len-pos;
		new = new_col(TEXT(help)+pos, anz);
		col_insert (NULL, help, new);
		REALLOC(&help,pos,-anz);
		if (rp) {
			hl_insert( rp, help );
			hl_insert( rp, new );
		}
		*col = help;
	}
	else
	{
		new = new_col("",0);
		col_insert(rp, help,new);
	}
	if (absatz) 
		(*col)->next->info |= ABSATZ;
	else 
		(*col)->next->info &= ~ABSATZ;
	if (overlen) 
		(*col)->next->info |= OVERLEN;
}

/* 
 * Wieviel WhiteSpace-Zeichen stehen am Anfang der Zeile?
*/
short col_offset(RINGP rp, LINEP col)
{
	short	pos;
	char c, *str;

	pos = 0;
	str = TEXT(col);
	c = *str++;
	while ((c=='\t' || c==' ') && pos<col->len)
	{
		pos++;
		c = *str++;
	}
	return pos;
}

short col_einrucken(RINGP rp, LINEP *col)
{
	LINEP	vor_col;
	short	length;

	vor_col = (*col)->prev;
	if (!IS_HEAD(vor_col))
	{
		length = col_offset(rp, vor_col);
		if ((*col)->len + length > MAX_LINE_LEN)
		{
			inote(1, 0, TOOLONG, MAX_LINE_LEN);
			return 0;
		}
		INSERT(col, 0, length, TEXT(vor_col));
	}
	else
		length = 0;
	return(length);
}

LINEP get_line(RINGP r, long y)
{
	LINEP	line;

	if (y < 0 || y >= r->lines) return NULL;
	if (y < (r->lines >> 1))
	{
		line = FIRST(r);
		while ((--y) >=0) NEXT(line);
	}
	else
	{
		line = LAST(r);
		y = r->lines-y;
		while ((--y) >0) PREV(line);
	}
	return line;
}

/*=========================================================================*/

/*
 * Ein Text ist als doppelt verkettete Liste implementiert
 * Am Anfang und Ende befindet sich je ein Zeilenkopf der
 * auf NULL zeigt
*/

void init_textring(RINGP r)
{
	LINEP	a;

	r->head.info = HEAD;
	r->tail.info = TAIL;
	a = new_col("",0);
	a->next = &r->tail;
	a->prev = &r->head;
	LAST(r) = a;
	r->tail.next = NULL;
	FIRST(r) = a;
	r->head.prev = NULL;
	r->lines = 1;
	r->ending = lns_tos;
	r->max_line_len = MAX_LINE_LEN;
	r->hl_anchor = NULL;
}

long textring_bytes(RINGP r)
{
	LINEP	line, ende;
	long		bytes, overlen = 0;

	line = FIRST(r);
	ende = LAST(r);
	bytes = line->len;
	while(line!=ende)
	{
		NEXT(line);
		bytes += line->len;
		if (IS_OVERLEN(line))
			overlen++;
	}

	if (r->ending != lns_binmode)
	{
		/* plus Zeilenenden: 1 Zeichen fr alle */
		bytes += r->lines - 1 - overlen;
		if (r->ending == lns_tos)
			/* fr TOS noch ein Zeichen */
			bytes += r->lines - 1 - overlen;
	}
	return bytes;
}

void free_textring(RINGP r)
{
	LINEP line, frei, ende;

	frei = LAST(r);				/* letzte Zeile */
	line = frei->prev;			/* vorletzte Zeile */
	ende = FIRST(r);				/* erste Zeile */
	while (frei!=ende)
	{
		FREE(frei);
		frei = line;
		PREV(line);
	}
	LAST(r) = frei;
	frei->next = &r->tail;
	REALLOC(&frei,0,-(frei->len));
	frei->info = 0;
	r->lines = 1;
}

void kill_textring(RINGP r)
{
	free_textring(r);
	FREE(FIRST(r));
	FIRST(r) = NULL;
	LAST(r) = NULL;
	r->lines = 0;
}

bool doppeln(RINGP old, RINGP new)
{
	LINEP	line, neu, a;
	long	lines, anz;
	bool	erg;

	erg = TRUE;
	free_textring(new);
	a	= FIRST(new);
	line	= FIRST(old);
	anz	= old->lines;

	INSERT(&a, 0, line->len, TEXT(line));
	a->info = line->info;
	NEXT(line);
	NEXT(a);
	lines = 1L;
	while (lines<anz)
	{
		neu = new_col(TEXT(line),line->len);
		neu->info = line->info;			/* ABSATZ mit kopieren */
		col_insert(NULL,a->prev, neu);
		NEXT(line);
		lines++;
		if (!ist_mem_frei())
		{
			erg = FALSE;
			break;
		}
	}
	new->lines = lines;
	new->ending = old->ending;
	new->max_line_len = old->max_line_len;
	return erg;
}

bool ist_leer(RINGP r)
{
	return(r->lines==1 && FIRST(r)->len==0);
}

/*=========================================================================*/

void kill_memory(void)
{
	short	i;
	BLOCK	**ptr;
	void	*help;
	
	for (i = MAX_ANZ + 1, ptr = free_list; (--i) >= 0; )
		*ptr++ = NULL;

	while (block_list != NULL)
	{
		help = *(void **)block_list;	/* nchster Block */
		Mfree(block_list);
		block_list = help;
	}
	mem_need_TQ = mem_need_BS = FALSE;
}

bool ist_mem_frei(void)
{
	if (mem_need_BS && mem_need_TQ)
	{
		note(1, 0, NOMEMORY);
		return (FALSE);
	}
	else
		return (TRUE);
}

void init_memory(void)
{
	short	i;
	BLOCK **ptr;

	mem_need_TQ = mem_need_BS = FALSE;
	for (i = MAX_ANZ + 1, ptr = free_list; (--i) >= 0; )
		*ptr++ = NULL;
	*ptr = (void *)-1L;	/* fr schleifenende */
	block_list = NULL;
}


/* Debug-Funktionen **********************************************************/

void dump_freelist(void)
{
	if (debug_level)
	{
		short	i;
		
		debug("dump_freelist...\n");
		for (i = 0; i < MAX_ANZ + 1; i++)
		{
			if ((free_list[i] != NULL) && (free_list[i]->free.magic == MAGIC))
			{
				BLOCK	*p;
				short	d;
				
				p = free_list[i];
				d = 0;
				while (p)
				{
					d++;
					p = p->free.next;
				}
				debug(" [%d].size: %d, Anzahl: %d\n", i, free_list[i]->free.size, d);
			}
		}
	}
}


void dump_ring(RINGP r)
{
	if (debug_level)
	{
		LINEP	line;
		char		*txt;
			
		line = FIRST(r);
		while (!IS_TAIL(line))
		{
			txt = TEXT(line);
			debug("%s\n", txt);
			NEXT(line);
		}
	}
}
