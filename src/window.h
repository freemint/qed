#ifndef _qed_windows_h_
#define _qed_windows_h_

#define CLASS_ALL			0			/* Alle Klassen */
#define CLASS_EDIT		1			/* Textfenster */
#define CLASS_PROJEKT	2			/* Projektfenster */
#define CLASS_ICON		3			/* Iconifiziertes Fenster f�r AllIconify */

#define WI_NONE			0x0000	/* Keine Flags */
#define WI_OPEN			0x0001	/* Fenster ist offen */
#define WI_TEXT			0x0002	/* Fenster enth�lt Text mit TAB und CR */ 
#define WI_FONTSIZE		0x0004	/* Bei Font�nderung Gr��enanpassung */
#define WI_REDRAW			0x0008	/* Fenster ganz redrawen bei size */
#define WI_NOTDRAWED 	0x0010	/* Eine redraw-Msg wird geschluckt */
#define WI_ICONIFIED		0x0020	/* Fenster ist Icon */
#define WI_FULLED			0x0040	/* Fenster auf voller Gr��e */
#define WI_CHANGED		0x0080	/* Fenster wurde ver�ndert '*' */
#define WI_SHADED			0x0100	/* Fenster wurde ge'shaded (MagiC, WINX) */
#define WI_WCOWORK		0x0200

/* Aktionen auf Icons */
#define DO_UNDO			0
#define DO_CUT				1
#define DO_COPY			2
#define DO_PASTE			3
#define DO_SELALL			4
#define DO_CLOSE			5
#define DO_OPEN			6
#define DO_INFO			7
#define DO_HELP			8
#define DO_LEFT			9
#define DO_RIGHT			10
#define DO_FORMAT			11
#define DO_PRINT			12
#define DO_ABAND			13
#define DO_SAVE			14
#define DO_SAVENEW		15
#define DO_FIND			16
#define DO_FINDNEXT		17
#define DO_ADD				18
#define DO_GOTO			19
#define DO_STRIPLINES	20
#define DO_TAB2LZ			21
#define DO_UPDATE			22
#define DO_LZ2TAB			23
#define DO_LINECOPY		24
#define DO_ZEICHTAB		25
#define DO_SWAPCHAR		26
#define DO_BIG2SMALL		27
#define DO_SMALL2BIG		28
#define DO_CHNG_SMBG		29
#define DO_CAPS			30
#define DO_UMLAUT			31
#define DO_AUTOSAVE		32
#define DO_FEHLER			33
#define DO_DELETE			34
#define DO_SORT			35
#define DO_TOPLINE		36

#define DO_ANZ				37		/* bei �nderung -> menu_nr[MENU] anpassen */


#define HORIZONTAL	0x01	/* F�r Scrolling... */
#define VERTICAL		0x02	/* ...und Schieber setzen */

#define SLPOS			0x01	/* F�r Schieber setzen */
#define SLSIZE			0x02

/*
 * exportierte Variablen
*/

extern WINDOWP 	sel_window;		/* Zeiger auf selektiertes Fenster */
extern SET			sel_objs;		/* Menge selektierter Objekte */
extern bool			all_iconified;	/* sind wir komplett iconified?? */

/*
 * Funktionen
*/

extern short 		num_openwin			(short class);
extern WINDOWP get_window			(short handle);
extern void 	do_all_window		(short class, WIN_DOFUNC func);
extern WINDOWP winlist_top			(void);
extern WINDOWP winlist_classtop	(short class);
extern void 	get_realtop		(void);
extern WINDOWP real_top			(void);
extern bool 	rc_first		(short wh, GRECT *b, GRECT *r);
extern bool 	rc_next			(short wh, GRECT *r);
extern void 	clr_area 		(GRECT *area);
extern bool 	free_for_draw		(WINDOWP w);
extern void 	redraw_window		(WINDOWP w, GRECT *area);
extern void 	size_window		(WINDOWP w, GRECT *new, bool border);
extern void	repos_window		(WINDOWP w, GRECT *new);
extern void 	full_window		(WINDOWP w);
extern void 	iconify_window		(WINDOWP w, GRECT *new);
extern void 	uniconify_window	(WINDOWP w, GRECT *new);
extern void	all_iconify		(WINDOWP w, GRECT *new);
extern void 	all_uniconify		(WINDOWP w, GRECT *new);
extern void 	move_window			(WINDOWP w, GRECT *new);
extern void 	ontop_window		(WINDOWP w);
extern void 	untop_window		(WINDOWP w);
extern void 	top_window			(WINDOWP w);
extern void 	cycle_window		(void);
extern void 	bottom_window		(WINDOWP w, short which);
extern void		shade_window		(WINDOWP w, short mode);
extern void 	scroll_vertical	(GRECT *area, short delta);
extern void 	arrow_window		(WINDOWP w, short arrow, long amount);
extern void 	h_slider				(WINDOWP w, short new_value);
extern void 	v_slider				(WINDOWP w, short new_value);
extern void 	set_sliders			(WINDOWP w, short which, short mode);
extern void 	set_wtitle			(WINDOWP w, char *filename);
extern void 	set_winfo			(WINDOWP w, char *info);
extern void 	change_window 		(WINDOWP w, char *filename, bool changed);
extern WINDOWP create_window		(short kind, short class, WIN_CRTFUNC crt);
extern bool 	open_window			(WINDOWP w);
extern void 	close_window		(WINDOWP w);
extern void 	do_font_change		(WINDOWP w);
extern void 	click_window		(WINDOWP w, short m_x, short m_y, short bstate, short kstate, short breturn);
extern void 	unclick_window 	(void);
extern bool		key_window			(WINDOWP w, short kstate, short kreturn);
extern void		arrange_window		(short mode);

extern void 	init_window			(void);
extern void		term_window			(void);

#endif
