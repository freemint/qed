#ifndef _qed_memory_h_
#define _qed_memory_h_

#define MAX_LINE_LEN	1023

#define TEXT(x)		((char *)(x) + sizeof(LINE))
#define NEXT(x)		(x = x->next)
#define PREV(x)		(x = x->prev)
#define FIRST(x)	((x)->head.next)
#define LAST(x)		((x)->tail.prev)
#define IS_FIRST(x)	((x->prev->info & HEAD) !=0)
#define IS_LAST(x)	((x->next->info & TAIL) !=0)

/* LINE->info */
#define MARKED		1
#define HEAD		2
#define TAIL		4
#define ABSATZ		8
#define OVERLEN		16

#define IS_MARKED(x)	((x->info & MARKED) !=0)
#define IS_HEAD(x)	((x->info & HEAD) !=0 )
#define IS_TAIL(x)	((x->info & TAIL) !=0 )
#define IS_ABSATZ(x)	((x->info & ABSATZ) !=0)
#define IS_OVERLEN(x)	((x->info & OVERLEN) !=0)


extern LINEP 	new_col		(char *str, short l);
extern void	free_col			(RINGP t, LINEP col);
extern LINEP	col_insert	(RINGP t, LINEP wo, LINEP was);
extern void	col_append	(RINGP t, LINEP was);
extern void	col_delete	(RINGP t, LINEP was);

extern void	col_concate	(RINGP t, LINEP *wo);
extern void	col_split	(RINGP t, LINEP *col,short pos);
extern short	col_offset	(RINGP t, LINEP col);
extern short	col_einrucken	(RINGP t, LINEP *col);

extern void	INSERT		(LINEP *a, short pos, short delta, char *str);
extern char	*REALLOC	(LINEP *a, short pos, short delta);
extern LINEP 	get_line	(RINGP r, long y);

extern void	init_textring	(RINGP r);
extern long	textring_bytes	(RINGP r);
extern void	free_textring	(RINGP r);
extern void	kill_textring	(RINGP r);
extern bool	doppeln		(RINGP old, RINGP new);
extern bool	ist_leer	(RINGP r);

extern bool	ist_mem_frei	(void);
extern void	init_memory	(void);
extern void	kill_memory	(void);

void dump_freelist(void);
void dump_ring(RINGP r);

#endif
