#ifndef _qed_menu_h_
#define _qed_menu_h_

extern void		handle_menu		(short title, short item, bool ctrl);
extern void		set_menu			(short item, bool yes);
extern void		mark_menu		(short item, bool yes);
extern void		update_menu		(void);
extern void		fillup_menu		(short item, char *new_text, short start_pos);

extern void		set_overwrite	(bool mode);

extern void		do_action		(short action);
extern bool		key_global		(short kstate, short kreturn);

extern void		menu_help		(short title, short item);

/*
 * Programmende vorbereiten und durchfÅhren.
*/
extern bool 	prepare_quit	(void);
extern void 	do_quit			(void);

#endif
