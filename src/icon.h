#ifndef _qed_icon_h_
#define _qed_icon_h_

#define MAX_ICON_ANZ 30

#define CLIP_ICON		0
#define SUB_ICON		1024

#define ALL_TYPES		-1


extern short		all_icons		(short *c_obj_nr);

extern short		decl_icon_type	(bool (*test)(short,short),
			 							 short	(*edit)(short,short),
										 void	(*exist)(short,SET),
										 bool	(*drag)(short,short));
extern bool		add_icon			(short type_id, short icon);
extern void		del_icon			(short icon);
extern short		icon_anz			(short type_id);

/* <0 : Fehler bei der AusfÅhrung	*/
/*	=0 : Nicht mîglich					*/
/* >0 : Erfolgreich ausgefÅhrt		*/
extern short		do_icon			(short icon, short action);
extern void		do_all_icon		(short type_id, short action);
extern bool		icon_test		(short icon, short action);
extern short		icon_edit		(short icon, short action);
extern void		icon_exist		(short icon, SET exist);

extern bool		icon_drag		(short dest_icon, short src_icon);

extern void		init_icon		(void);

#endif
