#include <support.h>
#include <time.h>

#include "global.h"
#include "ausgabe.h"
#include "av.h"
#include "block.h"
#include "clipbrd.h"
#include "comm.h"
#include "dd.h"
#include "edit.h"
#include "error.h"
#include "event.h"
#include "file.h"
#include "find.h"
#include "hl.h"
#include "icon.h"
#include "kurzel.h"
#include "makro.h"
#include "memory.h"
#include "menu.h"
#include "olga.h"
#include "options.h"
#include "poslist.h"
#include "printer.h"
#include "projekt.h"
#include "rsc.h"
#include "set.h"
#include "sort.h"
#include "tasten.h"
#include "text.h"
#include "umbruch.h"
#include "window.h"

/* Exportierte Variablen ***************************************************/
short	edit_type;

/****** DEFINES ************************************************************/

#define KIND	(NAME|INFO|CLOSER|FULLER|MOVER|SIZER|UPARROW|DNARROW|VSLIDE|LFARROW|RTARROW|HSLIDE|SMALLER)
#define FLAGS	(WI_TEXT|WI_FONTSIZE|WI_REDRAW)

/* Anzahl der Žnderungen in allen Texten bis zum restore_edit */
#define MAX_CHG 30

#define TEMP_LINK 101

/****** TYPES **************************************************************/

typedef struct
{
	short	link; 		/* Nummer von Text und Window */
	short	c; 		/* Art der Žnderung */
	long	y; 		/* y-Position */
} TCHANGE;

/* lokale Variablen ********************************************************/
static SET	used_info;
static short	chg_anz, find_erg, ascii_wert;
static TCHANGE 	chg[MAX_CHG];
static SET	chg_links;
static POSENTRY	*lastpos_list = NULL;

/* lokale Prototypen *******************************************************/

static void 	e_icon_exist		(short icon, SET actions);
static bool 	e_icon_test		(short icon, short action);
static short	e_icon_edit		(short icon, short action);
static bool 	e_icon_drag		(short icon, short source);
static void 	wi_draw			(WINDOWP window, GRECT *d);
static void 	wi_click 		(WINDOWP window, short m_x, short m_y, short bstate, short kstate, short breturn);
static bool 	wi_key			(WINDOWP window, short kstate, short kreturn);
static void 	wi_top			(WINDOWP window);
static void	wi_iconify		(WINDOWP window);
static void	wi_uniconify		(WINDOWP window);
static void 	destruct 		(short icon);

static void 	lz2tab			(TEXTP t_ptr);
static void 	tab2lz			(TEXTP t_ptr);
static void 	goto_line		(TEXTP t_ptr, short x, long y);
static void 	make_undo		(TEXTP t_ptr);
static void 	print_edit		(TEXTP t_ptr);
static bool 	open_edit		(short icon);
static void 	crt_edit 		(WINDOWP window);
static short  	crt_new_text		(char *filename, bool bin);

/***************************************************************************/

static short col_lz2tab(ZEILEP col, char *t, short tab_size)
{
	char	*str, c;
	short	i, tabH, lz, len;
	bool	changes;

	str = TEXT(col);
	changes = FALSE;
	tabH = tab_size;
	lz = 0;
	len = 0;
	for (i=col->len; (--i)>=0;)
	{
		c = *str++;
		if (c==' ')
		{
			if ((--tabH)==0)
			{
				if (lz>0)			/* Leerzeichen ersetzen */
				{
					c = '\t';
					t -= lz;
					len -= lz;
					changes = TRUE;
				}
				tabH = tab_size;
				lz = 0;
			}
			else
				lz++;
		}
		else
		{
			lz = 0;
			if ((--tabH)==0) tabH = tab_size;
		}
		*t++ = c;
		len++;
	}
	*t = EOS;
	if (changes)
		return (len);
	return (-1);
}

static void lz2tab(TEXTP t_ptr)
{
	ZEILEP 	lauf;
	short	x, i, tabsize;
	char	str[MAX_LINE_LEN + 1];

	graf_mouse(HOURGLASS, NULL);
	tabsize = t_ptr->loc_opt->tabsize;
	x = bild_pos(t_ptr->xpos,t_ptr->cursor_line,TRUE,tabsize);
	lauf = FIRST(&t_ptr->text);
	while (!IS_TAIL(lauf))
	{
		i = col_lz2tab(lauf, str, tabsize);
		if (i != -1)									/* Zeile ver„ndert */
		{
			REALLOC (&lauf, 0, i-lauf->len);
			memcpy(TEXT(lauf), str, (short) strlen(str));
			t_ptr->moved++;
		}
		NEXT(lauf);
	}
	t_ptr->cursor_line = get_line(&t_ptr->text,t_ptr->ypos);
	t_ptr->xpos = inter_pos(x,t_ptr->cursor_line,TRUE,tabsize);
	make_chg(t_ptr->link,POS_CHANGE,0); 	/* immer: Damit Infozeile einen '*' bekommt */
	restore_edit();
	graf_mouse(ARROW, NULL);
}

static short col_tab2lz(ZEILEP col, char *t, short tab_size)
{
	bool	with_tab = FALSE;
	short	tabH, len, i;
	char	*str, c;

	str = TEXT(col);
	tabH = tab_size;
	for (i = col->len,len = 0; (--i) >= 0 && (len < (MAX_LINE_LEN+1));)
	{
		c = *str++;
		if (c == '\t')
		{
			with_tab = TRUE;
			len += tabH;
			if (len > (MAX_LINE_LEN+1))
				tabH -= (len - (MAX_LINE_LEN+1));
			do
			{
				*t++ = ' ';
			}
			while (--tabH);
			tabH = tab_size;
		}
		else
		{
			*t++ = c;
			if ((--tabH)==0)
				tabH = tab_size;
			len++;
		}
	}
	*t = EOS;
	if (with_tab)
		return(len);
	else
		return(-1);
}

static void tab2lz(TEXTP t_ptr)
{
	ZEILEP 	lauf;
	short	i, x, tabsize;
	char	str[MAX_LINE_LEN + 1];

	graf_mouse(HOURGLASS, NULL);
	tabsize = t_ptr->loc_opt->tabsize;
	x = bild_pos(t_ptr->xpos,t_ptr->cursor_line,TRUE,tabsize);
	lauf = FIRST(&t_ptr->text);
	while (!IS_TAIL(lauf))
	{
		i = col_tab2lz (lauf,str,tabsize);
		if (i != -1)									/* Zeile ver„ndert */
		{
			REALLOC (&lauf, 0, i-lauf->len);
			memcpy(TEXT(lauf), str, (short)strlen(str));
			t_ptr->moved++;
		}
		NEXT(lauf);
	}
	t_ptr->cursor_line = get_line(&t_ptr->text,t_ptr->ypos);
	t_ptr->xpos = inter_pos(x,t_ptr->cursor_line,TRUE,tabsize);
	make_chg(t_ptr->link,POS_CHANGE,0); 	/* immer: Damit Infozeile einen '*' bekommt */
	restore_edit();
	graf_mouse(ARROW, NULL);
}

static void goto_line(TEXTP t_ptr, short x, long y)
{
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (y >= t_ptr->text.lines)
		y = t_ptr->text.lines - 1L;
	t_ptr->cursor_line = get_line(&t_ptr->text, y);
	t_ptr->ypos = y;
	t_ptr->xpos = inter_pos(x,t_ptr->cursor_line,t_ptr->loc_opt->tab,t_ptr->loc_opt->tabsize);
	make_chg(t_ptr->link,POS_CHANGE, 0);
}

static void make_undo(TEXTP t_ptr)
{
	short undo;

	undo = get_undo();
	if (undo == NO_UNDO) 
		return;

	/*
	 * Vorher an undo_pos springen
	 * Weil sonst u.U. restore zu schwierig
	 * Wird auch von do_undo_col vorausgesetzt
	*/
	t_ptr->cursor_line = get_line(&t_ptr->text,undo_y);
	t_ptr->ypos = undo_y;
	t_ptr->xpos = 0;
	make_chg(t_ptr->link,POS_CHANGE,0);
	restore_edit();

	do
	{
		if (undo == COL_ANDERS)
		{
			do_undo_col(t_ptr,undo);
			hl_update(t_ptr);
			restore_edit();
		}
		else
		{
			blk_undo(t_ptr,undo);
			restore_edit();
		}
		undo = get_undo();
	}
	while (undo != NO_UNDO);
}

/***************************************************************************/

static void do_absatz(WINDOWP window)
{
	if (window->flags & WI_TEXT) 					/* Text-Fenster */
	{
		TEXTP	t_ptr = get_text(window->handle);
		
		make_absatz(t_ptr);
		get_longestline(t_ptr);
		if (window->doc.w != t_ptr->max_line->exp_len)
		{
			window->doc.w = t_ptr->max_line->exp_len;
			set_sliders(window, HORIZONTAL, SLPOS+SLSIZE);
		}	
		redraw_window(window, &window->work);
	}
}


/*
 * Es wurde Zeilenumbruch ein oder ausgeschaltet und/oder Tab ge„ndert 
*/
void absatz_edit(void)
{
	do_all_window(CLASS_EDIT, do_absatz);
}


/***************************************************************************/

static void chg_edit_name(short icon)
{
	WINDOWP	window = get_window(icon);
	TEXTP 	t_ptr = get_text(icon);

	set_wtitle(window, t_ptr->filename);
}

/***************************************************************************/
/* Anlegen einer neuen Textdatei 														*/
/***************************************************************************/

short new_edit(void)
{
	short	icon;
	TEXTP t_ptr;

	icon = crt_new_text("", FALSE);
	if (icon < 0)
	{
		note(1, 0, NOTEXT);
		return -1;
	}
	t_ptr = get_text(icon);
	if (t_ptr->loc_opt->umbrechen)
		make_absatz(t_ptr);
	if (do_icon(icon,DO_OPEN) < 0)
	{
		note(1, 0, NOWINDOW);
		icon_edit(icon, DO_DELETE);
		icon = -3;
	}
	return icon;
} /* new_edit */

/***************************************************************************/

short load_edit(char *name, bool bin)
/* return: <=0 wurde nicht geladen */
/* 		  =0	weitere Texte versuchen sinnvoll */
/* 		  <0	weiter Texte versuchen nicht sinnvoll */
{
	WINDOWP 	window;
	TEXTP 		t_ptr;
	FILENAME	datei;
	PATH		path;
	short		err, icon;

	store_path(name);

	if (!bin && is_bin_name(name))
		bin = TRUE;

	split_filename(name, path, datei);
	if ((icon = text_still_loaded(name)) > 0)			/* schon geladen */
	{
		if (do_icon(icon, DO_OPEN) < 0)					/* nur Fenster auf */
			note(1, 0, NOWINDOW);
		return icon;
	}

	icon = crt_new_text(name, bin);						/* neuen Text anlegen */
	if (icon < 0)
	{
		note(1, 0, NOTEXT);
		return -1;												/* hat keinen Zweck mehr */
	}
	t_ptr = get_text(icon);
	
	if ((err = load(t_ptr, TRUE)) == -33)				/* File not Found */
	{
		if (path_exists(path))
		{
			if (snote(1, 2, NEWTEXT, datei) == 2)		/* neue Datei anlegen */
			{
				icon_edit(icon, DO_DELETE);
				return 0;										/* naechsten versuche */
			}
		}
		else
		{
			snote(1, 0, READERR, datei);
			icon_edit(icon, DO_DELETE);
			return 0;											/* naechsten versuchen */
		}
	}
	else if (err)												/* anderer Fehler */
	{
		snote(1, 0, READERR, datei);
		icon_edit(icon, DO_DELETE);
		return 0;
	}
	if (t_ptr->loc_opt->umbrechen)
	{
		make_absatz(t_ptr);
		if (t_ptr->loc_opt->format_by_load)
			total_format(t_ptr);
	}
	window = get_window(icon);
	window->doc.x = 0;
	window->doc.y = 0;
	window->doc.w = get_longestline(t_ptr);
	window->doc.h = t_ptr->text.lines;
	if (do_icon(icon, DO_OPEN) < 0)
	{
		note(1, 0, NOWINDOW);
		icon_edit(icon, DO_DELETE);
		icon = -2;
	}
	if (t_ptr->moved) 										
	{
		if (t_ptr->loc_opt->umbrechen)					/* format_by_load */
			set_info(t_ptr, rsc_string(UMBRUCHSTR));
		else														/*	Nullbytes */
		{
			make_chg(t_ptr->link,TOTAL_CHANGE,0);
			set_info(t_ptr, rsc_string(NULLBYTESTR));
		}
		change_window(window, t_ptr->filename, TRUE);
		restore_edit();
		Bconout(2, 7);
	}

	if (find_poslist(lastpos_list, name, &desire_x, &desire_y) != NULL)
		icon_edit(icon, DO_GOTO);
	else
		insert_poslist(&lastpos_list, name, 0, 0);

	return icon;
}

/***************************************************************************/

static void print_edit (TEXTP t_ptr)
{
	FILENAME name;
	bool	 print_block;
	
	if (t_ptr->namenlos)
		strcpy(name, t_ptr->filename);
	else
		file_name(t_ptr->filename, name, FALSE);
	print_block = t_ptr->block;
	if (prn_start_dial(&print_block))
	{
		if (print_block)
			blk_drucken(name, t_ptr);
		else
			txt_drucken(name, t_ptr);
	}
} /* print_edit */


void close_edit(char *mask, short flag)
{
	short i, min;
	
	min = setmin(used_info);
	for (i = setmax(used_info); i >= min; i--)
	{
		if (setin(used_info, i))
		{
			TEXTP t_ptr = get_text(i);

			if (filematch(t_ptr->filename, mask, t_ptr->filesys))
			{
				switch (flag)
				{
					case 0 :							/* sichern ohne schliežen */
						if (t_ptr->moved)			/* nur wenn n”tig! */
							do_icon(i, DO_SAVE);
						break;

					case 1 :							/* sichern und schliežen */
						do_icon(i, DO_DELETE);
						break;

					case 2 :							/* schliežen ohne sichern */
						t_ptr->moved = 0;
						do_icon(i, DO_DELETE);
						break;

					default:
						debug("close_edit: Unknown SE_CLOSE Flag %d\n", flag);
						break;
				}
			}
		}
	}
}


static bool delete_edit(short icon, TEXTP t_ptr)
{
	short	 antw;
	FILENAME name;

	if (t_ptr->moved != 0)
	{
		if (quick_close)
			antw = 1;
		else
		{
			if (t_ptr->namenlos)
				strcpy(name, t_ptr->filename);
			else
				file_name(t_ptr->filename, name, FALSE);
			antw = snote(1, 3, MOVED, name);
		}
		if (antw == 1)
		{
			if (do_icon(icon,DO_SAVE) < 0)
				return (FALSE);
		}
		if (antw == 3)
			return(FALSE);
	}
	return (TRUE);
}

/***************************************************************************/
/* Fenster angeclickt																		*/
/***************************************************************************/

/* Ermittelt aus einer Mauspos (my) die zugeh”rige y-Position im Text */
static long get_ypos(WINDOWP window, short my)
{
	TEXTP	t_ptr = get_text(window->handle);
	long	y;
	
	y = (my - window->work.g_y);
	if (y < 0)
		y -= font_hcell;
	y /= font_hcell;
	y += window->doc.y;
	if (y >= t_ptr->text.lines)
		y = t_ptr->text.lines-1;
	else if (y < 0)
		y = 0;

	return y;
}

/* Ermittelt aus einer Mauspos (mx) die zugeh”rige x-Position im Text */
static short get_xpos(WINDOWP window, long ypos, short mx)
{
	TEXTP	t_ptr = get_text(window->handle);
	short	x;
	ZEILEP	col;

	col = get_line(&t_ptr->text, ypos);

	if (font_prop)
	{
		short	i, x_soll, s, e, xl; 
		short	save_xpos;
		
		save_xpos = t_ptr->xpos;
		
		x_soll = (mx - window->work.g_x) + ((short) window->doc.x * font_wcell);

		/* Zur groben Positionierung machen wir "Halbierungs-Algorithmus" */
		s = 0;
		e = col->len;
		xl = -1;
		while (TRUE)
		{
			x = s + ((e - s) / 2);
			if (x == xl)
				break;
			t_ptr->xpos = x;
			i = cursor_xpos(t_ptr, t_ptr->xpos, NULL);
			if (i > x_soll)
				e = x;			/* in linker H„lfte */
			else
				s = x;			/* in rechter H„lfte */
			xl = x;				
		}
		/* jetzt Fein-Positionierung zeichenweise heranbewegen */
		for (x = s; x <= e; x++)
		{
			t_ptr->xpos = x;
			i = cursor_xpos(t_ptr, t_ptr->xpos, NULL);
			if (i > x_soll) 
				break;
		}
		x--;

		t_ptr->xpos = save_xpos;
	}
	else
	{

		x = mx - window->work.g_x;
		if (x > 0)
		{
			x /= font_wcell;
			x += (short) window->doc.x;
		}
		else if (window->doc.x > 0)
			x = (short) window->doc.x - 1;
		else
			x = 0;

		x = inter_pos(x, col, t_ptr->loc_opt->tab, t_ptr->loc_opt->tabsize);
	}

	return x;	
}

static void set_cursor(WINDOWP window, short mx, short my)
{
	TEXTP 	t_ptr = get_text(window->handle);
	long	y;
	ZEILEP	col;

	y = get_ypos(window, my);
	col = get_line(&t_ptr->text, y);
	t_ptr->ypos = y;
	t_ptr->cursor_line = col;
	t_ptr->xpos = get_xpos(window, y, mx);

	y -= window->doc.y;
	if (y > 0)
	{
		if (y > window->w_height)
			make_chg(t_ptr->link,MOVE_UP, y-window->w_height);
	}
	else
		make_chg(t_ptr->link,MOVE_DOWN, -y);
	make_chg(t_ptr->link,POS_CHANGE, 0);
}

/*
 * Prft, ob Mauspos (x,y) innerhalb der Blockselektion liegt.
*/
static bool click_in_blk(WINDOWP window, short x, short y)
{
	TEXTP	t_ptr = get_text(window->handle);
	long	line;
	short	col;
	
	if (!t_ptr->block)
		return FALSE;

	line = get_ypos(window,y);
	col = get_xpos(window, line, x);

	if (line == t_ptr->z1 && line == t_ptr->z2)			/* nur eine Zeile */
	{
		if (col >= t_ptr->x1 && col < t_ptr->x2)
			return TRUE;
		else
			return FALSE;
	}
	else
	{
		if ((line == t_ptr->z1 && col >= t_ptr->x1) ||	/* rechts vom Anfang */
			(line == t_ptr->z2 && col < t_ptr->x2) ||		/* links vom Ende */
			(line > t_ptr->z1 && line < t_ptr->z2))		/* zwischen Anfang und Ende (Zeilen) */
			return TRUE;
		else
			return FALSE;
	}
}


#define LINE_MODE		1
#define WORD_MODE		2
#define KEY_MODE		3
#define BRACE_MODE	4

static void wi_click(WINDOWP window, short m_x, short m_y, short bstate, short kstate, short breturn)
{
	short	event, mode, kreturn;
	TEXTP 	t_ptr = get_text(window->handle);
	GRECT	*s = &window->work;
	
	/* Infomeldung l”schen */
	clear_info(t_ptr);

	if (bstate & 2) 											/* Rechtsclick */
	{
		if (strlen(error[0]) > 0)
		{
			blk_demark(t_ptr);
			set_cursor(window, m_x, m_y);
			restore_edit();
			handle_error(t_ptr);
		}
		return;													/* und wech... */
	}

	if (!inside(m_x, m_y, s))
		return;
	t_ptr->blk_mark_mode = FALSE;
	unclick_window();
	if (breturn == 2)											/* Doppelklick */
	{
		blk_demark(t_ptr);
		set_cursor(window, m_x, m_y);
		restore_edit();
		if (kstate & (K_RSHIFT|K_LSHIFT) ||				/* Ganze Zeile markieren */
			 t_ptr->xpos==t_ptr->cursor_line->len)
		{
			t_ptr->xpos = 0;
			blk_mark(t_ptr, 0);
			if (IS_LAST(t_ptr->cursor_line))
				t_ptr->xpos = t_ptr->cursor_line->len;
			else
			{
				NEXT(t_ptr->cursor_line);
				t_ptr->ypos++;
			}
			blk_mark(t_ptr, 1);
			restore_edit();
			mode = LINE_MODE;
		}
		else if (blk_mark_brace(t_ptr))					/* Klammer-Selektion */
		{
			restore_edit();
			mode = BRACE_MODE;
		}
		else														/* wortweise */
		{
			blk_mark_word(t_ptr);
			mode = WORD_MODE;
		}
	}
	else															/* Einfachklick */
	{
		if (kstate & (K_RSHIFT|K_LSHIFT))
		{
			if (!t_ptr->block)
				blk_mark(t_ptr,0);							/* Anfang = alte Cursorpos */
			set_cursor(window, m_x, m_y);
			blk_mark(t_ptr, 1);
			restore_edit();
		}
		else														/* Zieh-Aktion auf Block?  */
		{
			graf_mkstate(&m_x, &m_y, &bstate, &kstate);

			if (click_in_blk(window, m_x, m_y) && bstate & 1)
			{
				short		win_id;
				WINDOWP	qed_win;
				RING		t;
				short		x, y, w, h, d;
				GRECT		r;
				
				/* Rahmen ermitteln */
				r = window->work;
				if (t_ptr->z1 == t_ptr->z2)	/* Spezialfall: nur eine Zeile */
				{
					x = cursor_xpos(t_ptr, t_ptr->x1, NULL);
					w = cursor_xpos(t_ptr, t_ptr->x2, NULL) - x;
					x += r.g_x;
				}
				else
				{
					x = r.g_x;
					w = r.g_w;
				}
				y = (short)((t_ptr->z1 - window->doc.y) * font_hcell) + r.g_y;
				if (t_ptr->x2 == 0)
					h = ((short)(t_ptr->z2 - t_ptr->z1)) * font_hcell;
				else
					h = ((short)(t_ptr->z2 - t_ptr->z1) + 1) * font_hcell;
				
				/* Clipping auf Fenstergr”že */
				if (y < r.g_y)
				{
					h -= (r.g_y - y); 
					y = r.g_y;
				}
				if (y + h > (r.g_y + r.g_h - 1))
					h = (r.g_y + r.g_h - 1) - y;
				
				/* Box verschieben */
				graf_mouse(TEXT_CRSR/*FLAT_HAND*/, NULL);
				graf_dragbox(w, h, x, y, gl_desk.g_x, gl_desk.g_y, gl_desk.g_w, gl_desk.g_h, &d, &d);
				graf_mouse(ARROW, NULL);
				graf_mkstate(&m_x, &m_y, &bstate, &kstate);

				/* markierten Text merken */
				init_textring(&t);
				block_copy(t_ptr, &t);

				/* wohin wurde gezogen? */
				win_id = wind_find(m_x, m_y);
				qed_win = get_window(win_id);

				if (qed_win)											/* eigenes Fenster */
				{
					/* Ziel muž Textfenster sein */
					if (qed_win->class == CLASS_EDIT)
					{
						if (qed_win != window)						/* ein anderes */
						{
							t_ptr = get_text(qed_win->handle);
							blk_paste(t_ptr, &t);
						}
						else												/* das selbe */
						{
							Bconout(2, 7);
#if 0
							blk_delete(t_ptr);
							set_cursor(window, m_x, m_y);
							blk_paste(t_ptr, &t);
#endif
						}
						restore_edit();
					}
					else
						Bconout(2, 7);
				}
				else														/* fremdes Fenster */
					send_dd(win_id, m_x, m_y, kstate, &t);				
				kill_textring(&t);
			}
			else
			{
				blk_demark(t_ptr);
				set_cursor(window, m_x, m_y);
				blk_mark(t_ptr, 0);
				restore_edit();
			}
		}
		mode = KEY_MODE;
	}
	graf_mkstate(&m_x, &m_y, &bstate, &kstate);
	if (bstate & 1)													/* immernoch gedrckt */
	{
		graf_mouse(POINT_HAND, NULL);
		wind_update(BEG_MCTRL);
		while(TRUE)
		{
			event = evnt_multi((MU_BUTTON | MU_M1 | MU_M2),
									  1, 0x01, 0x00,
									  TRUE, m_x, m_y, 1, 1,
									  TRUE, s->g_x, s->g_y, s->g_w, s->g_h,
									  NULL, 0L,
									  &m_x, &m_y, &bstate, &kstate, &kreturn, &breturn);

			if (event & MU_BUTTON) 
				break;
			if (event & (MU_M1 | MU_M2))
			{
				set_cursor(window, m_x, m_y);
				if (mode == WORD_MODE)
				{
					long	y;
					short	x, xpos,len;
					char	*str;

					xpos = t_ptr->xpos;
					str = TEXT(t_ptr->cursor_line) + xpos;
					len = t_ptr->cursor_line->len;
					get_blk_mark(t_ptr, &y, &x);
					if (t_ptr->ypos > y || t_ptr->xpos > x) 		/* nach rechts */
					{
						while(xpos<=len && setin(t_ptr->loc_opt->wort_set,*str))
						{
							xpos++;
							str++;
						}
					}
					else														/* nach links */
					{
						while(xpos>=0 && setin(t_ptr->loc_opt->wort_set,*str))
						{
							xpos--;
							str--;
						}
						str++; xpos++;
					}
					t_ptr->xpos = xpos;
				}
				else if (mode == LINE_MODE)
				{
					long	y;
					short	x;

					get_blk_mark(t_ptr, &y, &x);
					t_ptr->xpos = 0;
					if (y == t_ptr->ypos && !IS_LAST(t_ptr->cursor_line))
					{
						NEXT(t_ptr->cursor_line);
						t_ptr->ypos++;
					}
				}
				blk_mark(t_ptr, 1);
				restore_edit();
			}
		}
		wind_update(END_MCTRL);
		graf_mouse(ARROW, NULL);
	}
}


static bool wi_key (WINDOWP window, short kstate, short kreturn)
{
	TEXTP t_ptr = get_text(window->handle);

	/* Infomeldung l”schen */
	clear_info(t_ptr);

	if (edit_key(t_ptr, window, kstate, kreturn))
	{
		restore_edit();
		return TRUE;
	}
	return FALSE;
}


static void wi_draw(WINDOWP window, GRECT *d)
{
	TEXTP t_ptr = get_text(window->handle);

	set_clip(TRUE, d);
	if (d->g_x == window->work.g_x && d->g_w == window->work.g_w)
	{
		if (d->g_y == window->work.g_y + window->work.g_h - window->yfac &&
			 d->g_h == window->yfac)
		{
			/* Letzte Zeile */
			line_out(window, t_ptr, window->w_height - 1);
		}
		else if (d->g_y == window->work.g_y && d->g_h == window->yfac)
		{
			/* Erste Zeile */
			line_out(window, t_ptr, 0);
		}
		else
			bild_out(window, t_ptr);
	}
	else
		bild_out(window,t_ptr);
}


static void wi_top(WINDOWP window)
{
	/* Krzel/Schreibschutz „ndern */
	do_icon(window->handle, DO_UPDATE);
}

static	void wi_iconify(WINDOWP window)
{
	TEXTP 		t_ptr = get_text(window->handle);
	FILENAME	short_name;

	make_shortpath(t_ptr->filename, short_name, 8);
	set_wtitle(window, short_name);
}

static	void wi_uniconify(WINDOWP window)
{
	TEXTP t_ptr = get_text(window->handle);

	set_wtitle(window, t_ptr->filename);
}

/***************************************************************************/
/* Operation vorhanden ?																	*/
/***************************************************************************/

static void e_icon_exist(short icon, SET actions)
{
	TEXTP	t_ptr = get_text(icon);
	WINDOWP	window = get_window(icon);

	setclr(actions);

	if ((window->flags & WI_ICONIFIED) || (window->flags & WI_SHADED))
	{
		/* Einzige m”gliche Aktion: */
		setincl(actions, DO_DELETE);
		return;
	}

	if (any_undo())
		setincl(actions, DO_UNDO);
	if (t_ptr->block)
	{
		setincl(actions, DO_CUT);
		setincl(actions, DO_COPY);
		setincl(actions, DO_LEFT);
		setincl(actions, DO_RIGHT);
		setincl(actions, DO_BIG2SMALL);
		setincl(actions, DO_SMALL2BIG);
		setincl(actions, DO_CHNG_SMBG);
		setincl(actions, DO_CAPS);
		setincl(actions, DO_SORT);
	}
	else
	{
		setincl(actions, DO_LINECOPY);
		setincl(actions, DO_SWAPCHAR);
	}
	if (t_ptr->loc_opt->tab)
	{
		setincl(actions, DO_TAB2LZ);
		setincl(actions, DO_LZ2TAB);
	}
	if (t_ptr->loc_opt->umbrechen)
		setincl(actions, DO_FORMAT);
	else
		setincl(actions, DO_STRIPLINES);
	if (window->flags & WI_OPEN)
		setincl(actions, DO_CLOSE);
	setincl(actions, DO_DELETE);
	setincl(actions, DO_PASTE);
	setincl(actions, DO_SELALL);
	setincl(actions, DO_OPEN);
	setincl(actions, DO_INFO);
	setincl(actions, DO_HELP);
	setincl(actions, DO_PRINT);
	if (!t_ptr->namenlos)
		setincl(actions,DO_ABAND);
	setincl(actions, DO_SAVE);
	setincl(actions, DO_SAVENEW);
	setincl(actions, DO_FIND);
	setincl(actions, DO_FINDNEXT);
	setincl(actions, DO_GOTO);
	setincl(actions, DO_ADD);
	setincl(actions, DO_UPDATE);
	setincl(actions, DO_ZEICHTAB);
	setincl(actions, DO_UMLAUT);
	if (t_ptr->moved)
		setincl(actions, DO_AUTOSAVE);
	setincl(actions, DO_FEHLER);
	if ((t_ptr->ypos - window->doc.y) > 0)
		setincl(actions, DO_TOPLINE);
}

/***************************************************************************/
/* Operation testen																			*/
/***************************************************************************/

static bool e_icon_test(short icon, short action)
{
	bool	 erg;
	TEXTP 	 t_ptr = get_text(icon);
	FILENAME name;

	switch(action)
	{
		case DO_UNDO	:
			erg = any_undo();
			break;
		case DO_CUT 	:
			erg = t_ptr->block;
 			break;
		case DO_COPY	:
			erg = t_ptr->block;
			break;
		case DO_LINECOPY:
			erg = !(t_ptr->block);
			break;
		case DO_PASTE	:
			erg = TRUE;
			break;
		case DO_SELALL :
			erg = TRUE;
			break;
		case DO_CLOSE	:
		case DO_DELETE	:
			erg = delete_edit(icon, t_ptr);
			break;
		case DO_OPEN	:
			erg = TRUE;
			break;
		case DO_INFO	:
			erg = TRUE;
			break;
		case DO_HELP	:
			erg = TRUE;
			break;
		case DO_LEFT	:
		case DO_RIGHT	:
		case DO_BIG2SMALL	:
		case DO_SMALL2BIG	:
		case DO_CHNG_SMBG	:
		case DO_CAPS		:
		case DO_SORT 		:
			erg = t_ptr->block;
			break;
		case DO_FORMAT :
			erg = t_ptr->loc_opt->umbrechen;
			break;
		case DO_PRINT	:
			erg = TRUE;
			break;
		case DO_ABAND	:
			if (t_ptr->namenlos)
				erg = FALSE;
			else
			{
				erg = TRUE;
				if (!ist_leer(&t_ptr->text) && t_ptr->moved!=0)
				{
					if (t_ptr->namenlos)
						strcpy(name, t_ptr->filename);
					else
						file_name(t_ptr->filename, name, FALSE);
					erg = (snote(1, 2, ABANDON, name) == 1);
				}
			}
			break;
		case DO_SAVE	:
			erg = TRUE;
			break;
		case DO_SAVENEW:
			erg = TRUE;
			break;
		case DO_FIND :
			if (t_ptr->block)
			{
				RING	r;
	
				/* copy to R_STR */
				block_copy(t_ptr, &r);
				if (strlen(TEXT(FIRST(&r))) > 0)
				{
					strncpy(s_str, TEXT(FIRST(&r)), HIST_LEN);
					kill_textring(&r);
					s_str[HIST_LEN] = EOS;
				}
			}
			find_erg = replace_dial();
			erg = (find_erg!=0);
			break;
		case DO_FINDNEXT:
			erg = TRUE;
			break;
		case DO_ADD 	:
			erg = TRUE;
			break;
		case DO_GOTO	:
			erg = goto_line_dial();
			break;
		case DO_STRIPLINES:
			erg = !t_ptr->loc_opt->umbrechen;
			break;
		case DO_TAB2LZ :
			erg = t_ptr->loc_opt->tab;
			break;
		case DO_LZ2TAB :
			erg = t_ptr->loc_opt->tab;
			break;
		case DO_UPDATE	:
			erg = TRUE;
			break;
		case DO_ZEICHTAB:
			ascii_wert = ascii_table(font_id, 13);
			erg = (ascii_wert != -1);
			break;
		case DO_UMLAUT:
			erg = umlaut_dial();
			break;
		case DO_SWAPCHAR:
			erg = !t_ptr->block;
			break;
		case DO_AUTOSAVE :
			if (as_text && t_ptr->moved)
			{
				long	min;
				short	btn;

				min = (short)((time(NULL) - t_ptr->asave) / 60L);
				if (min >= as_text_min)
				{
					if (as_text_ask)				/* Nachfrage ? */
					{
						if (t_ptr->namenlos)
							strcpy(name, t_ptr->filename);
						else
							file_name(t_ptr->filename, name, FALSE);
						Bconout(2, 7);
						btn = snote(2, 3, ASAVEASK, name);
						if (btn == 1)
							as_text = FALSE;
					}
					else
						btn = 2;

					t_ptr->asave = time(NULL);
					erg = (btn == 2);
				}
				else
					erg = FALSE;
			}
			else
			{
				t_ptr->asave = time(NULL);
				erg = FALSE;
			}
			break;
		case DO_FEHLER :
			erg = TRUE;
			break;
		case DO_TOPLINE :
			erg = TRUE;
			break;
		default	:
			erg = FALSE;
	}
	return erg;
}

/***************************************************************************/
/* Operation durchfhren																	*/
/***************************************************************************/

static short e_icon_edit(short icon, short action)
{
	PATH	name = "";
	TEXTP	t_ptr = get_text(icon);
	RING	r;
	WINDOWP	window;
	short	erg;
	bool	ok, bin, shift;
	ZEILEP	lauf;

	shift = shift_pressed();
	window = get_window(icon);
	erg = 0;
	switch(action)
	{
		case DO_UNDO	:
			t_ptr->blk_mark_mode = FALSE;
			blk_demark(t_ptr);
			make_undo(t_ptr);
			erg = 1;
			break;
		case DO_CUT 	:
			t_ptr->blk_mark_mode = FALSE;
			blk_cut(t_ptr);
			restore_edit();
			erg = 1;
			break;
		case DO_COPY	:
			t_ptr->blk_mark_mode = FALSE;
			blk_copy(t_ptr);
			restore_edit();
			erg = 1;
			break;
		case DO_LINECOPY:
			line_copy(t_ptr);
			restore_edit();
			erg = 1;
			break;
		case DO_PASTE	:
			t_ptr->blk_mark_mode = FALSE;
			load_clip();
			if (!ist_leer(&clip_text))
			{
				blk_paste(t_ptr, &clip_text);
				if (t_ptr->loc_opt->umbrechen && t_ptr->loc_opt->format_by_paste)
					format(t_ptr);
				restore_edit();
			}
			erg = 1;
			break;
		case DO_SELALL :
			t_ptr->blk_mark_mode = FALSE;
			blk_mark_all(t_ptr);
			restore_edit();
			erg = 1;
			break;
		case DO_CLOSE	:
		case DO_DELETE	:
			t_ptr->blk_mark_mode = FALSE;
			destruct(icon);
			erg = 1;
			break;
		case DO_OPEN	:
			t_ptr->blk_mark_mode = FALSE;
			if (!open_edit(icon))
				erg = -1;
			else
				erg = 1;
			break;
		case DO_INFO	:
			t_ptr->blk_mark_mode = FALSE;
			if (t_ptr->block)
				block_info(t_ptr);
			else
				info_edit(icon);
			erg = 1;
			break;
		case DO_HELP	:
			t_ptr->blk_mark_mode = FALSE;
			if (!t_ptr->block)
				blk_mark_word(t_ptr);
			if (t_ptr->block)
			{
				block_copy(t_ptr, &r);
				if (strlen(TEXT(FIRST(&r))) > 0)
					erg = call_help(TEXT(FIRST(&r)));
				else
					erg = call_hyp("main");
				kill_textring(&r);
			}
			else
				erg = call_hyp("main");
			break;
		case DO_LEFT	:
			t_ptr->blk_mark_mode = FALSE;
			blk_left(t_ptr);
			restore_edit();
			erg = 1;
			break;
		case DO_RIGHT	:
			t_ptr->blk_mark_mode = FALSE;
			blk_right(t_ptr);
			restore_edit();
			erg = 1;
			break;
		case DO_SMALL2BIG	:
			t_ptr->blk_mark_mode = FALSE;
			blk_upplow(t_ptr, BLK_UPPER);
			restore_edit();
			erg = 1;
			break;
		case DO_BIG2SMALL	:
			t_ptr->blk_mark_mode = FALSE;
			blk_upplow(t_ptr, BLK_LOWER);
			restore_edit();
			erg = 1;
			break;
		case DO_CHNG_SMBG	:
			t_ptr->blk_mark_mode = FALSE;
			blk_upplow(t_ptr, BLK_CH_UPLO);
			restore_edit();
			erg = 1;
			break;
		case DO_CAPS	:
			t_ptr->blk_mark_mode = FALSE;
			blk_upplow(t_ptr, BLK_CAPS);
			restore_edit();
			erg = 1;
			break;
		case DO_FORMAT :
			t_ptr->blk_mark_mode = FALSE;
			blk_demark(t_ptr);
			if (shift)
				total_format(t_ptr);
			else
				format(t_ptr);
			restore_edit();
			erg = 1;
			break;
		case DO_PRINT	:
			t_ptr->blk_mark_mode = FALSE;
			print_edit(t_ptr);
			erg = 1;
			break;
		case DO_ABAND	:
abandon:	strcpy(name, t_ptr->filename);
			bin = (t_ptr->text.ending == lns_binmode);
			destruct(icon);
			icon = load_edit(name, bin);
			if (icon > 0)
			{
				t_ptr = get_text(icon);
				erg = -1;
				if (t_ptr != NULL)
				{
					if (open_edit(icon))
					{
						memset(msgbuff, 0, (short) sizeof(msgbuff));
						msgbuff[0] = WM_TOPPED;
						msgbuff[3] = window->handle;
						send_msg(gl_apid);
						erg = 1;
					}
				}
			}
			clr_undo();
			break;
		case DO_SAVE	:
			if (!t_ptr->namenlos)
			{
				t_ptr->blk_mark_mode = FALSE;
				if (t_ptr->loc_opt->umbrechen)
					save_absatz(t_ptr);					/* Zeilenende korrigieren */
				if (save(t_ptr)<0)
					erg = -1;
				else
					erg = 1;

				make_chg(icon, WT_CHANGE, 0);			/* nur Fenstertitel schreiben */
				restore_edit();
				break;
			}
			/* Bei Namenlos zu DO_SAVENEW */
		case DO_SAVENEW:
			t_ptr->blk_mark_mode = FALSE;
			if (t_ptr->block)
			{
				strcpy(name, "");
				if (save_new(name, "", rsc_string(SAVEBLKSTR)))
				{
					TEXTP temp_ptr;

					temp_ptr = new_text(TEMP_LINK);
					if (t_ptr->loc_opt->umbrechen)	/* Zeilenende korrigieren */
						save_absatz(t_ptr);
					block_copy(t_ptr,&temp_ptr->text);	/* Block rauskopieren */
					temp_ptr->cursor_line = FIRST(&t_ptr->text);
					temp_ptr->loc_opt = t_ptr->loc_opt;
					erg = save_as(temp_ptr,name);
					destruct_text(temp_ptr);
					if (erg==0)
						erg = 1;
				}
				else
					erg = -1;
			}
			else
			{
				strcpy(name, t_ptr->filename);
				if (save_new(name, "", rsc_string(SAVEASSTR)))
				{
					bool umb_old = t_ptr->loc_opt->umbrechen;
					
					if (t_ptr->loc_opt->umbrechen)	/* Zeilenende korrigieren */
						save_absatz(t_ptr);
					if (save_as(t_ptr, name) == 0)
					{
						if (t_ptr->namenlos || note(1, 2, GETNAME) == 1)
						{
							/* OLGA informieren */
							do_olga(OLGA_RENAME, t_ptr->filename, name);
							do_olga(OLGA_UPDATE, name, NULL);
							set_text_name(t_ptr, name, FALSE);
							hl_init_text(t_ptr);
								
							chg_edit_name(icon);
							t_ptr->moved = 0;
							t_ptr->file_date_time = file_time(name,NULL,NULL);
							t_ptr->readonly = file_readonly(name);
						}
						make_chg(icon, TOTAL_CHANGE, 0); /* ggf. neue lok. Optionen! */
						make_chg(icon, WT_CHANGE, 0);		/* Headline schreiben */
						ch_kurzel(t_ptr->loc_opt->kurzel, FALSE);
						if (umb_old != t_ptr->loc_opt->umbrechen)
							make_absatz(t_ptr);
						restore_edit();
						erg = 1;
					}
					else
						erg = -1;
				}
				else
					erg = -1;
			}
			break;
		case DO_FIND	:
			t_ptr->blk_mark_mode = FALSE;
			if (find_erg == 1)
			{
				if (start_find(t_ptr,FALSE)==0)
				{
					Bconout(2, 7);
					end_play();
				}
			}
			else
			{
				if (find_erg == 2)
				{
					if (start_replace(t_ptr) == 0)
					{
						Bconout(2, 7);
						end_play();
					}
				}
			}
			erg = 1;
			break;
		case DO_FINDNEXT:
			t_ptr->blk_mark_mode = FALSE;
			if (t_ptr->block)
				find_selection(t_ptr);
			else
			{
				if (do_next(t_ptr) != 1)
				{
					Bconout(2, 7);
					end_play();
				}
			}
			erg = 1;
			break;
		case DO_GOTO	:
			t_ptr->blk_mark_mode = FALSE;
			blk_demark(t_ptr);
			goto_line(t_ptr, desire_x, desire_y);
			restore_edit();
			erg = 1;
			break;
		case DO_STRIPLINES:
			t_ptr->blk_mark_mode = FALSE;
			blk_demark(t_ptr);
			graf_mouse(HOURGLASS, NULL);
			if (strip_endings(t_ptr))
			{
				t_ptr->max_line = NULL;
				lauf = t_ptr->cursor_line = get_line(&t_ptr->text, t_ptr->ypos);
				if (t_ptr->xpos > lauf->len)
					t_ptr->xpos = lauf->len;
				make_chg(t_ptr->link, POS_CHANGE, 0);		/* '*' in Titel */
			}
			graf_mouse(ARROW, NULL);
			hl_update_text(t_ptr);
			restore_edit();
			erg = 1;
			break;
		case DO_TAB2LZ :
			t_ptr->blk_mark_mode = FALSE;
			blk_demark(t_ptr);
			tab2lz(t_ptr);			
			hl_update_text(t_ptr);
			erg = 1;
			break;
		case DO_LZ2TAB :
			t_ptr->blk_mark_mode = FALSE;
			blk_demark(t_ptr);
			lz2tab(t_ptr);
			hl_update_text(t_ptr);
			erg = 1;
			break;
		case DO_ADD 	:
			if (shift)
				ok = select_single(name, "", rsc_string(INSNAMESTR));
			else
				ok = select_single(name, "", rsc_string(MERGESTR));
			if (ok)
			{
				if (shift)				/* Dateinamen einfgen */
				{
					RING		temp_ring;
					ZEILEP	col;

					init_textring(&temp_ring);
					col = new_col(name, (short)strlen(name));
					col_insert(&temp_ring,&(temp_ring.head), col);
					blk_paste(t_ptr, &temp_ring);
					restore_edit();
					kill_textring(&temp_ring);
				}
				else								/* Dateiinhalt einfgen */
				{
					TEXTP temp_ptr = new_text(TEMP_LINK);
					short	antw;
					
					if (temp_ptr!=NULL)
					{
						set_text_name(temp_ptr, name, FALSE);
						antw = load(temp_ptr, TRUE);
						if (antw == 0)
						{
							if (t_ptr->loc_opt->umbrechen)
							{
								temp_ptr->loc_opt = t_ptr->loc_opt;
								make_absatz(temp_ptr);
								if (temp_ptr->loc_opt->format_by_load)
									total_format(temp_ptr);
							}
							blk_paste(t_ptr,&(temp_ptr->text));
							restore_edit();
						}
						else
							open_error(name, antw);
						destruct_text(temp_ptr);
					}
				}
			}
			erg = 1;
			break;
		case DO_UPDATE	:
			/* Schreibschutz oder Datei ver„ndert? */
			if (file_exists(t_ptr->filename))
			{
				bool	read_only = file_readonly(t_ptr->filename);

				if (read_only!=t_ptr->readonly)
				{
					t_ptr->readonly = read_only;
					make_chg(icon,POS_CHANGE,0);		/* Headline schreiben */
					restore_edit();
				}
				if (t_ptr->file_date_time!=-1L)
				{
					long date_time = file_time(t_ptr->filename,NULL,NULL);

					if (date_time!=t_ptr->file_date_time)
					{
						file_name(t_ptr->filename, name, FALSE);
						if (snote(1, 2, MOVED3, name) == 1)
							goto abandon;
						else
							t_ptr->file_date_time = date_time;
					}
				}
			}
			/* Krzel updaten */
			ch_kurzel(t_ptr->loc_opt->kurzel, FALSE);
			erg = 1;
			break;
		case DO_ZEICHTAB:
			if (ascii_wert != -1)
			{
				char_insert(t_ptr, ascii_wert);
				hl_update(t_ptr);
			}
			restore_edit();
			erg = 1;
			break;
		case DO_UMLAUT:
			change_umlaute(t_ptr);
			hl_update_text(t_ptr);
			restore_edit();
			erg = 1;
			break;
		case DO_SWAPCHAR:
			char_swap(t_ptr);
			hl_update(t_ptr);
			restore_edit();
			break;
		case DO_AUTOSAVE:
			e_icon_edit(icon, DO_SAVE);
			break;
		case DO_FEHLER :
			if (error[0][0] != EOS)
			{
				blk_demark(t_ptr);
				restore_edit();
				handle_error(t_ptr);
			}
			break;
		case DO_TOPLINE :
			arrow_window(window, WA_DNLINE, t_ptr->ypos - window->doc.y);
			break;
		case DO_SORT :
			sort_block(t_ptr);
			break;
	}
	return erg;
}

/***************************************************************************/
/* Es wurde etwas auf ein Textfenster geschoben										*/
/***************************************************************************/

static bool e_icon_drag(short icon, short source)
{
	WINDOWP	w = get_window(icon);
	TEXTP 	t_ptr;
	bool	ret = FALSE;

 	if ((w->flags & WI_ICONIFIED) || (w->flags & WI_SHADED))
 		return FALSE;

	switch (source)
	{
		case DRAGDROP_FILE :						/* Inhalt von drag_filename einfgen */
			t_ptr = get_text(icon);
			if (drag_filename[0] != EOS && t_ptr != NULL)
			{
				TEXTP temp_ptr = new_text(TEMP_LINK);
				short	antw;
				
				if (temp_ptr!=NULL)
				{
					set_text_name(temp_ptr, drag_filename, FALSE);
					antw = load(temp_ptr, TRUE);
					if (antw == 0)
					{
						blk_paste(t_ptr,&(temp_ptr->text));
						restore_edit();
					}
					else
						open_error(drag_filename, antw);
					destruct_text(temp_ptr);
					ret = TRUE;
				}
			}
			drag_filename[0] = EOS;
			break;

		case DRAGDROP_PATH :						/* Text aus drag_filename einfgen */
			t_ptr = get_text(icon);
			if (drag_filename[0] != EOS && t_ptr != NULL)
			{
				if ((t_ptr->cursor_line->len + (short)strlen(drag_filename)) < MAX_LINE_LEN)
				{
					RING		temp_ring;
					ZEILEP	col;
						
					init_textring(&temp_ring);
					col = new_col(drag_filename, (short)strlen(drag_filename));
					col_insert(&temp_ring,&(temp_ring.head), col);
					if (drag_data_size > 1)
					{
						/* mehr als ein ARGS -> Zeilenvorschub */
						col = new_col("", 0);
						col_append(&temp_ring, col);
					}
					blk_paste(t_ptr, &temp_ring);
					restore_edit();
					kill_textring(&temp_ring);
					ret = TRUE;
				}
				else
					inote(1, 0, TOOLONG, MAX_LINE_LEN);
			}
			drag_filename[0] = EOS;
			break;

		case DRAGDROP_DATA :						/* Textdaten einfgen */
			if (drag_data_size == DDS_RINGP)
			{
				RINGP	t;
				
				t_ptr = get_text(icon);
				t = (RINGP)drag_data;
				blk_paste(t_ptr, t);
				restore_edit();
				kill_textring(t);
				drag_data = NULL;
				drag_data_size = 0;
			}
			else
			if (drag_data_size > 0 && drag_data != NULL)
			{
				RING		temp_ring;
				ZEILEP	col;
				char		*p1, *p2, *zeile;
				long		delta;
				ZEILEP	lauf;

debug("hier ist edit.DragDrop_DATA!!!\n");
				t_ptr = get_text(icon);
				init_textring(&temp_ring);
				lauf = &temp_ring.head;
				p1 = drag_data;
				p2 = strchr(p1, '\r');
				if (p2 != NULL)							/* mehrere Zeilen? */
				{
					zeile = (char *) malloc(MAX_LINE_LEN);
					while (p2 != NULL)
					{
						delta = p2 - p1;
						strncpy(zeile, p1, delta);
						zeile[delta] = EOS;
						col = new_col(zeile, (short)strlen(zeile));
						col_insert(&temp_ring,lauf, col);
						NEXT(lauf);
						p1 = p2 + 2;						/* \r\n berspringen */
						p2 = strchr(p1, '\r');
						temp_ring.lines++;
					}
					free(zeile);
				}
				else											/* nur eine Zeile ohne \r\n */
				{
					col = new_col(drag_data, (short)strlen(drag_data));
					col_insert(&temp_ring,lauf, col);
				}
				blk_paste(t_ptr, &temp_ring);
				restore_edit();
				kill_textring(&temp_ring);
				free(drag_data);							/* Speicher wieder freigeben */
				drag_data_size = 0L;
				ret = TRUE;
			}
			break;

		default:
			if (debug_level)
				debug("edit.e_icon_drag(): Unbekannter Mode %d\n", source);
	}

	return ret;
}

/***************************************************************************/

void blink_edit(void)
{
	WINDOWP	window;
	TEXTP 	t_ptr;

	window = winlist_top();
	if (window!=NULL && window->class==CLASS_EDIT)
	{
		t_ptr = get_text(window->handle);
		if (t_ptr->cursor)
		{
			if (t_ptr->blink) 		/* gerade wg. Blinken aus */
				t_ptr->blink = FALSE;
			else
				t_ptr->blink = TRUE;
			cursor(window,t_ptr);
		}
	}
}

void onblink_edit(void)
{
	WINDOWP	window;
	TEXTP 	t_ptr;

	window = winlist_top();
	if (window!=NULL && window->class==CLASS_EDIT)
	{
		t_ptr = get_text(window->handle);
		if (t_ptr->cursor)
		{
			cursor(window,t_ptr);
			t_ptr->blink = FALSE;
		}
	}
}

void offblink_edit(void)
{
	WINDOWP	window;
	TEXTP 	t_ptr;

	window = winlist_top();
	if (window!=NULL && window->class==CLASS_EDIT)
	{
		t_ptr = get_text(window->handle);
		if (t_ptr->cursor && !t_ptr->blink)
		{
			cursor(window,t_ptr);
		}
	}
}

/***************************************************************************/
/* Tastenverarbeitung																		*/
/***************************************************************************/
void make_chg (short link, short change, long ypos)
{
/*
	SCROLL_UP 		Alles unter der aktuellen Zeile wird hochgescrollt,
						die letzte Zeile wird natrlich neu geschrieben.
		  				(Cntrl-Y, und Delete am Ende der Zeile).
	SCROLL_DOWN		Alles unterhalb der aktuellen Zeile und diese Zeile werden
						nach unten gescrollt, die aktuelle Zeile wird neu
						geschrieben. Ist auch LINE_CHANGE gesetzt wird auch die
						Zeile vor der aktuellen Zeile neu geschrieben.
						(RETURN, last_out_klemm)
	MOVE_UP,
	MOVE_DOWN		Das Fenster wird hoch und runter gescrollt um anz Zeilen
	BLK_CHANGE		Die Blockmarkierung wurde ge„ndert.
	WT_CHANGE		Fenstertitel ('*') l”schen (nach save).
*/
	short i;

	if (change==TOTAL_CHANGE)
	{
		for (i=chg_anz; (--i)>=0; )			/* unntze Žnderung */
			if (chg[i].link==link)
			{
				if (chg[i].c==LINE_CHANGE && chg[i].y>=ypos)
					chg[i].c = NOP_CHANGE;
				else if (chg[i].c==SCROLL_DOWN && chg[i].y>=ypos)
					chg[i].c = NOP_CHANGE;
				else if (chg[i].c==SCROLL_UP && chg[i].y>=ypos)
					chg[i].c = NOP_CHANGE;
				else if (chg[i].c==TOTAL_CHANGE)
				{
					if (chg[i].y>=ypos)
						chg[i].y = ypos;
					return;
				}
			}
	}
	if (change==LINE_CHANGE)
	{	
		for (i=chg_anz; (--i)>=0; )			/* gleiche Žnderung */
			if (chg[i].link==link && chg[i].c==change && chg[i].y==ypos)
				return;			
	}
	if (change==POS_CHANGE)
	{
		for (i=chg_anz; (--i)>=0; )
			if (chg[i].link==link && chg[i].c==change)
				return;
	}
	if (chg_anz>=MAX_CHG)
	{
		inote(1, 0, FATALERR,0);
		return;
	}
	chg[chg_anz].link = link;
	chg[chg_anz].c = change;
	chg[chg_anz].y = ypos;
	setincl(chg_links,link);
	chg_anz++;
}

void pos_korr(WINDOWP window, TEXTP t_ptr)
{
	short	x_new;
	long	y_new;

	y_new = t_ptr->ypos - window->doc.y;
	if (y_new < 0L)
	{
		if (y_new == -1L)
			arrow_window(window, WA_UPLINE, 1);
		else
			arrow_window(window, WA_UPLINE, window->w_height/2-y_new);
	}
	else if (y_new >= window->w_height)
	{
		if (y_new == window->w_height)
			arrow_window(window, WA_DNLINE, 1);
		else
			arrow_window(window, WA_DNLINE, y_new-window->w_height/2);
	}

	x_new = cursor_xpos(t_ptr, t_ptr->xpos, NULL) - (short) window->doc.x * font_wcell;
	if (x_new < 0)
		arrow_window(window, WA_LFLINE, -(x_new/font_wcell) + window->w_width/2);
	else if (x_new >= window->work.g_w)
		arrow_window(window, WA_RTLINE, (x_new-window->work.g_w)/font_wcell + window->w_width/2);
}

/*
 * Neuzeichnen, wenn Fenster teilweise verdeckt.
 * z.B. bei D&D
*/
static void restore_offdesk(WINDOWP window, TCHANGE *c, short c_anz, TEXTP t_ptr)
{
	short	y_screen;
	bool	finished = FALSE;
	GRECT	a;

	for (; (--c_anz)>=0; c++)
	{
		switch (c->c)
		{
			case WT_CHANGE :	/* fr save(): nur '*' l”schen, sonst nix */
				change_window(window, t_ptr->filename, (t_ptr->moved!=0));
				break;

			case POS_CHANGE	 :
				if (window->class==CLASS_EDIT)
				{
					a = window->work;
					a.g_h = gl_hchar;
					redraw_window(window, &a);
				}
				change_window(window, t_ptr->filename, (t_ptr->moved!=0));
				pos_korr(window, t_ptr);
				break;

			case LINE_CHANGE:
				y_screen = (short) (c->y - window->doc.y);
				if (y_screen>=0 && y_screen<window->w_height)
				{
					a = window->work;
					a.g_y += y_screen * window->yfac;
					a.g_h = window->yfac;
					redraw_window(window, &a);
				}
				break;

			case TOTAL_CHANGE  :
			case BLK_CHANGE	 :
			case SCROLL_UP 	 :
			case SCROLL_DOWN :
				if (!finished)
					redraw_window(window, &window->work);
				finished = TRUE;
				break;

			case MOVE_UP:
				arrow_window(window, WA_DNLINE, c->y);
				break;

			case MOVE_DOWN:
				arrow_window(window, WA_UPLINE, c->y);
				break;
		}
	}
}

/*
 * Neuzeichnen, wenn Fenster komplett frei.
*/
static void restore_indesk(WINDOWP window, TCHANGE *c, short c_anz, TEXTP t_ptr)
{
	GRECT	a;
	short	y_screen, help;
	long	z1,z2;

	for (; (--c_anz)>=0; c++)
	{
		set_clip(TRUE, &(window->work));
		switch (c->c)
		{
			case WT_CHANGE:				/* nur '*' „ndern, sonst nix */
				change_window(window, t_ptr->filename, (t_ptr->moved!=0));
				break;

			case POS_CHANGE:
				if (window->class == CLASS_EDIT)
					head_out(window, t_ptr);
				change_window(window, t_ptr->filename, (t_ptr->moved!=0));
				pos_korr(window, t_ptr);
				break;

			case LINE_CHANGE:
				y_screen = (short)(c->y - window->doc.y);
				if (y_screen < window->w_height)
					line_out(window, t_ptr, y_screen);
				break;

			case TOTAL_CHANGE:
				y_screen = (short) (c->y - window->doc.y);
				bild_out(window,t_ptr);
				break;

			case SCROLL_UP:
				y_screen = (short) (c->y - window->doc.y);
				if (y_screen < window->w_height-1)
				{
					help = window->yfac*(y_screen+1);
					a = window->work;
					a.g_h -= help;
					a.g_y += help;
					scroll_vertical (&a, window->yfac);
				}
				if (y_screen < window->w_height)
					line_out(window, t_ptr, window->w_height-1);
				break;

			case SCROLL_DOWN:
				y_screen = (short) (c->y - window->doc.y);
				if (y_screen < window->w_height-1)
				{
					a = window->work;
					a.g_h -= window->yfac * (y_screen + 1);
					a.g_y += window->yfac * y_screen;
					scroll_vertical (&a, -window->yfac);
				}
				/* Die neue Zeile wird mit LINE_CHANGE gezeichnet */
				break;

			case MOVE_UP:
				arrow_window(window, WA_DNLINE, c->y);
				break;

			case MOVE_DOWN:
				arrow_window(window, WA_UPLINE, c->y);
				break;

			case BLK_CHANGE:
				z1 = c->y;
				c++; c_anz--;
				z2 = c->y;
				bild_blkout(window,t_ptr,z1,z2);
				break;
		}
	}
}

void restore_edit(void)
{
	WINDOWP	window;
	TEXTP 	t_ptr;
	short	i, c_anz, link, max;
	TCHANGE	c[MAX_CHG], *c_ptr;
	
	if (!chg_anz) 
		return;

	hide_mouse();
	max = setmax(chg_links);
	for (link = setmin(chg_links); link <= max; link++) 
	{
		if(setin(chg_links,link))
		{
			window = get_window(link);
			if (window == NULL) 
				continue;								/* Kommt vor (Prj_Text mit 10001) */
			c_anz = 0;
			c_ptr = chg;
			for (i=chg_anz; (--i)>=0; c_ptr++)
				if (c_ptr->link==link && c_ptr->c!=NOP_CHANGE)
					c[c_anz++] = *c_ptr;
	
			t_ptr = get_text(window->handle);

			/* Slider anpassen */
			if (window->doc.h != t_ptr->text.lines)
			{
				window->doc.h = t_ptr->text.lines;
				set_sliders(window, VERTICAL, SLPOS+SLSIZE);
			}

			get_longestline(t_ptr);
			if (window->doc.w != t_ptr->max_line->exp_len)
			{
				window->doc.w = t_ptr->max_line->exp_len;
				set_sliders(window, HORIZONTAL, SLPOS+SLSIZE);
			}	

			if ((window->flags & WI_OPEN) || (window->flags & WI_ICONIFIED))
			{
				if (free_for_draw(window))
					restore_indesk(window, c, c_anz, t_ptr);
				else
					restore_offdesk(window, c, c_anz, t_ptr);
			}
		}
	}
	show_mouse();
	chg_anz = 0;			/* Keine Žnderungen mehr */
	setclr(chg_links);
}

/***************************************************************************/
static void destruct(short icon)
{
	TEXTP t_ptr = get_text(icon);
	WINDOWP window = get_window(icon);

	insert_poslist(&lastpos_list, t_ptr->filename, t_ptr->xpos, t_ptr->ypos);

	/* Text war Error-Datei */
	if (t_ptr == last_errtext)
		last_errtext = NULL;

	close_window(window);
	destruct_text(t_ptr);
	del_icon(icon);
	setexcl(used_info,icon);
	clr_undo();
	do_all_icon(prj_type, DO_UPDATE);		/* Projekte updaten */
}

/***************************************************************************/

static short crt_new_text(char *filename, bool bin)
{
	TEXTP 		t_ptr;
	WINDOWP		win;
	PATH			name;
	bool		namenlos;

	if (filename[0] == EOS)
	{
		strcpy(name, rsc_string(NAMENLOS));
		namenlos = TRUE;
	}
	else
	{
		strcpy(name, filename);
		namenlos = FALSE;
	}
	
	/* Fenster anlegen */
	win = create_window(KIND, CLASS_EDIT, crt_edit);
	if (win == NULL)
		return -1;
	if (!add_icon(edit_type, win->handle))
		return -1;

	/* Text kreiern */
	t_ptr = new_text(win->handle);
	if (t_ptr == NULL)
	{
		del_icon(win->handle);
		return -1;
	}

	if (bin)
	{
		t_ptr->text.ending = lns_binmode;
		t_ptr->text.max_line_len = bin_line_len;
	}
	else
		t_ptr->text.max_line_len = MAX_LINE_LEN;

	set_text_name(t_ptr, name, namenlos);

	setincl(used_info, win->handle);

	set_wtitle(win, name);
	set_winfo(win, "");
	
	if (!namenlos)
		do_all_icon(prj_type, DO_UPDATE);				/* Projekte updaten */

	t_ptr->asave = time(NULL);

	return win->handle;
	
}

/***************************************************************************/
/* Kreieren eines Fensters 																*/
/***************************************************************************/
static void crt_edit(WINDOWP window)
{
	short initw = 0, inith = 0;

	if (window->work.g_w == 0 || window->work.g_h == 0)
	{
		/* Keine Gr”že bekannt. */
		initw  = min ((gl_desk.g_w / font_wcell) * font_wcell - 7 * font_wcell, 80 * font_wcell);
		inith  = (gl_desk.g_h / font_hcell) * font_hcell - 7 * font_hcell;
	
		window->work.g_x	= gl_wchar + 2 * 8;
		window->work.g_y	= 60;
		window->work.g_w	= initw;
		window->work.g_h	= inith;
	}
	
	window->flags		= FLAGS;
	window->doc.w		= 0;
	window->doc.h		= 0;
	window->xfac		= font_wcell;
	window->yfac		= font_hcell;
	window->w_width		= initw/font_wcell;
	window->w_height	= inith/font_hcell;
	window->draw		= wi_draw;
	window->click		= wi_click;
	window->key 		= wi_key;
	window->top		= wi_top;
	window->ontop		= wi_top;
	window->iconify		= wi_iconify;
	window->uniconify	= wi_uniconify;
	window->close		= NULL;
}

/***************************************************************************/
/* ™ffnen des Objekts																		*/
/***************************************************************************/
static bool open_edit(short icon)
{
	bool	ok;
	WINDOWP	window = get_window(icon);

	ok = TRUE;

	if (window->flags & WI_ICONIFIED)
		uniconify_window(window, NULL);
	else if (window->flags & WI_SHADED)
		shade_window(window, -1);
	else if (window->flags & WI_OPEN)
		top_window(window);
	else
	{
		TEXTP t_ptr = get_text(window->handle);

		window->doc.x = 0;
		window->doc.y = 0;
		window->doc.h = t_ptr->text.lines;
		pos_korr(window, t_ptr);
		ok = open_window (window);
		ch_kurzel(t_ptr->loc_opt->kurzel, FALSE);
	}
	return ok;
}

/***************************************************************************/
/* Info des Objekts																			*/
/***************************************************************************/
bool info_edit (short icon)
{
	char		str[32], date[11];
	TEXTP 		t_ptr = get_text(icon);
	short		erg;
	LINEENDING	ending;
	bool		b;
	
	set_long(textinfo, INFBYTES, textring_bytes(&t_ptr->text));
	if (t_ptr->text.ending != lns_binmode)
		set_long(textinfo, INFZEILE, t_ptr->text.lines);
	else
		set_string(textinfo, INFZEILE, "--");		/* Bin„r: keine Zeilen */

	make_shortpath(t_ptr->filename, str, 30);
	set_string (textinfo, INFNAME, str);		/* Name mit Pfad */
	if (t_ptr->namenlos)
	{
		strcpy(str, "");
		strcpy(date, "--");
	}
	else
		file_time (t_ptr->filename, date, str);
	set_string(textinfo, INFDATUM, date); 	/* Datum */
	set_string(textinfo, INFZEIT, str);		/* Uhrzeit */

	set_state(textinfo, INFTOS, OS_SELECTED, (t_ptr->text.ending == lns_tos));
	set_state(textinfo, INFUNIX, OS_SELECTED, (t_ptr->text.ending == lns_unix));
	set_state(textinfo, INFMAC, OS_SELECTED, (t_ptr->text.ending == lns_apple));
	ending = t_ptr->text.ending;

	/* Dateien aus einem Projekt oder Bin„r -> kein Zeilenende */
	b = (!setin(used_info, icon) || (t_ptr->text.ending == lns_binmode));
	set_state(textinfo, INFTOS, OS_DISABLED, b);
	set_state(textinfo, INFUNIX, OS_DISABLED, b);
	set_state(textinfo, INFMAC, OS_DISABLED, b);

	erg = simple_mdial(textinfo, 0) & 0x7fff;
	if (erg == INFOK)
	{
		if (get_state(textinfo, INFTOS, OS_SELECTED))
			t_ptr->text.ending = lns_tos;
		if (get_state(textinfo, INFUNIX, OS_SELECTED))
			t_ptr->text.ending = lns_unix;
		if (get_state(textinfo, INFMAC, OS_SELECTED))
			t_ptr->text.ending = lns_apple;

		if (t_ptr->text.ending != ending)
		{
			t_ptr->moved++;
			make_chg(t_ptr->link, POS_CHANGE, 0); 	/* Damit Infozeile einen '*' bekommt */
			restore_edit();
		}
	}
	return TRUE;
}

void init_edit(void)
{
	setclr(chg_links);
	setclr(used_info);
	drag_filename[0] = EOS;
	edit_type = decl_icon_type(e_icon_test, e_icon_edit, e_icon_exist, e_icon_drag);
}

void	cursor_off(short wHandle)
{
	WINDOWP	window;
	TEXTP 	t_ptr;

	window = get_window(wHandle);
	if (window!=NULL && window->class==CLASS_EDIT)
	{
		t_ptr = get_text(window->handle);
		if (t_ptr->cursor)
			t_ptr->cursor = FALSE;
	}
}

void	cursor_on(short wHandle)
{
	WINDOWP	window;
	TEXTP 	t_ptr;

	window = get_window(wHandle);
	if (window!=NULL && window->class==CLASS_EDIT)
	{
		t_ptr = get_text(window->handle);
		if (!t_ptr->cursor)
			t_ptr->cursor = TRUE;
	}
}

void set_info(TEXTP t_ptr, char *str)
{
	strcpy(t_ptr->info_str, str);
}

void clear_info(TEXTP t_ptr)
{
	if (t_ptr->info_str[0] != EOS)
		strcpy(t_ptr->info_str, "");
}

/***************************************************************************/
static void do_color_change(WINDOWP w)
{
	redraw_window(w, &w->work);
}

void color_change(void)
{
	vsf_color(vdi_handle, bg_color);
	vst_color(vdi_handle, fg_color);
	fill_color = bg_color;
	set_drawmode();
	
	/* Alle Fenster updaten */
	do_all_window(CLASS_ALL, do_color_change);
}
