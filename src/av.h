#ifndef _qed_av_h_
#define _qed_av_h_

/*
 * Kommunikation mit dem AV-Server.
*/
#include "vaproto.h"


extern short	av_shell_id,		/* ID des Desktops */
				av_shell_status;	/* Welche VA_* kann der Desktop */


extern bool	send_avkey			(short ks, short kr);
/*
 * Schickt Tastendruck an AV-Server.
*/

extern void	send_avdrag			(short wh, short m_x, short m_y, short kstate, short data_type);
/*
 * Schickt AV_DRAG_TO_WIN.
*/

extern short check_avobj			(short x, short y);
/*
 * Ermittelt, was an <x,y> auf dem AV-Desktop liegt.
*/

extern void	send_avwinopen		(short handle);
extern void	send_avwinclose	(short handle);
/*
 * Fenster auf bzw. zu.
*/

extern void send_avpathupdate ( char *str );
/* fordert nach Speichern auf, das Directory zu aktualisieren. */

extern bool	call_help			(char *str);
/*
 * Schickt ein AC_HELP an das angemeldete Hilfe-ACC.
*/

extern bool	call_hyp				(char *data);
/*
 * Schickt data an ST-Guide.
*/

extern void	handle_av			(short msg[]);
/*
 * Verabeitet Messages vom Server.
*/

extern void init_av				(void);
extern void term_av				(void);

#endif
