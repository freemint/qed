#ifndef _qed_winlist_h_
#define _qed_winlist_h_

extern WINDOWP	used_list;

extern void		move_to_top		(WINDOWP w);
extern void		move_to_end		(WINDOWP w);

extern void		add_winlist		(short class, GRECT *r);
extern void		save_winlist	(FILE *fd);

extern WINDOWP get_new_window	(short class);
extern void		free_window		(WINDOWP w);

extern void		init_winlist	(void);
extern void		term_winlist	(void);

#endif
