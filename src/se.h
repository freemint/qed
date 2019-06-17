#ifndef _qed_se_h_
#define _qed_se_h_

/*
 * Kommunikation mit einer Programmiershell.
*/
#include "seproto.h"

#define SHELLANZ	7

/* globale Variablen */
extern SHELLENTRY	se_shells[];
extern bool			se_activ;
extern bool			se_autosave;
extern bool			se_autosearch;
extern bool     se_ignoreclose;


/* Heiko */
extern bool se_ignoreclose;

extern void	setup_semenu	(void);
/*
 * Setzt das Shell-MenÅ 
*/

extern void	handle_se		(short *msg);
/* 
 * Wertet die von der Shell kommenden Messages aus 
*/

extern void	handle_es		(short menuitem);
/* 
 * Wertet die im menÅ ausgewÑhlten Aktionen aus und schickt sie der Shell 
*/

extern void	timer_se			(void);
/*
 * Wird von der globalen Event-Schleife alls 600ms aufgerufen.
*/

void init_se_title(bool showbar);
extern void	init_se			(void);
extern void term_se			(void);

#endif
