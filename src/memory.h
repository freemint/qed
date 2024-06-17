#ifndef _qed_memory_h_
#define _qed_memory_h_

#define MAX_LINE_LEN	1023

#define TEXT(x)		((char *)(x) + sizeof(ZEILE))
#define NEXT(x)		(x=x->nachf)
#define VORG(x)		(x=x->vorg)
#define FIRST(x)		((x)->head.nachf)
#define LAST(x)		((x)->tail.vorg)
#define IS_FIRST(x)	((x->vorg->info&HEAD)!=0)
#define IS_LAST(x)	((x->nachf->info&TAIL)!=0)

/* ZEILE->info */
#define MARKED				1
#define HEAD				2
#define TAIL				4
#define ABSATZ				8
#define OVERLEN			16

#define IS_MARKED(x)		((x->info&MARKED)!=0)
#define IS_HEAD(x)		((x->info&HEAD)!=0)
#define IS_TAIL(x)		((x->info&TAIL)!=0)
#define IS_ABSATZ(x)		((x->info&ABSATZ)!=0)
#define IS_OVERLEN(x)	((x->info&OVERLEN)!=0)


extern ZEILEP 	new_col			(char *str, short l);
extern void		free_col			(RINGP t, ZEILEP col);
extern ZEILEP	col_insert		(RINGP t, ZEILEP wo, ZEILEP was);
extern void		col_append		(RINGP t, ZEILEP was);
extern void		col_delete		(RINGP t, ZEILEP was);

extern void		col_concate		(RINGP t, ZEILEP *wo);
extern void		col_split		(RINGP t, ZEILEP *col,short pos);
extern short	col_offset		(RINGP t, ZEILEP col);
extern short	col_einrucken	(RINGP t, ZEILEP *col);

extern void		INSERT			(ZEILEP *a, short pos, short delta, char *str);
extern char		*REALLOC			(ZEILEP *a, short pos, short delta);
extern ZEILEP 	get_line			(RINGP r, long y);

extern void		init_textring	(RINGP r);
extern long		textring_bytes	(RINGP r);
extern void		free_textring	(RINGP r);
extern void		kill_textring	(RINGP r);
extern bool		doppeln			(RINGP old, RINGP new);
extern bool		ist_leer			(RINGP r);

extern bool		ist_mem_frei	(void);
extern void		init_memory		(void);
extern void		kill_memory		(void);

void dump_freelist(void);
void dump_ring(RINGP r);

#endif
