#ifndef _qed_event_h_
#define _qed_event_h_

extern bool		abort_prog;

extern bool		check_for_abbruch	(void);
extern void		main_loop			(void);

void handle_msg(_WORD *msg);

#endif
