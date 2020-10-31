#ifndef _qed_comm_h_
#define _qed_comm_h_

/*
 * comm.h: several communikation functions.
 */

extern char		*global_str1;
extern char		*global_str2;
extern short	msgbuff[8]; 			/* buffer for send_msg */

/*
 * Send the global message buffer.
 * msgbuff[1] und msgbuff[2] are set to gl_apid/0
 */
bool		send_msg			(short id);

/*
 * Inform shell and other processes about a clipboard change.
 */
void 	send_clip_change	(void);

/*
 * SMU 7.01 Document-History.
 */
void		send_dhst			(char *filename);

/* initialize all protocols */
void 	init_comm		(void);

/* terminate all protocols */
void		term_comm		(void);

#endif
