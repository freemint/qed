#include "global.h"
#include "ausgabe.h"
#include "block.h"
#include "clipbrd.h"
#include "edit.h"
#include "find.h"
#include "icon.h"
#include "kurzel.h"
#include "makro.h"
#include "memory.h"
#include "options.h"
#include "rsc.h"
#include "set.h"
#include "text.h"
#include "umbruch.h"
#include "window.h"
#include "tasten.h"
/* Heiko */
#include "hl.h"

/* Modi fr deselect_block() */
#define UP		1
#define DOWN	2
#define LEFT	3
#define RIGHT	4

/* lokale Prototypen */
static void text_end(TEXTP t_ptr);
static void text_start(TEXTP t_ptr);

static void line_up(TEXTP t_ptr);
static void page_up(TEXTP t_ptr);

static void line_down(TEXTP t_ptr);
static void page_down(TEXTP t_ptr);

static void char_right(TEXTP t_ptr);

static void word_left(TEXTP t_ptr);
static void word_right(TEXTP t_ptr);

static void line_end(TEXTP t_ptr);
static void line_start(TEXTP t_ptr);

static void tabulator(TEXTP t_ptr);

static void ctrl_y(TEXTP t_ptr);
static void char_delete(TEXTP t_ptr);

static void word_delete(TEXTP t_ptr);
static void ctrl_word_delete(TEXTP t_ptr);
static void word_bs(TEXTP t_ptr);
static void ctrl_word_bs(TEXTP t_ptr);

static char	alt_str[3];
static short	alt_cnt = -1;

static bool pos_move(TEXTP t_ptr, long delta)
{
	LINEP l_ptr;

	l_ptr = t_ptr->cursor_line;
	if (!t_ptr->up_down)
	{
		t_ptr->desire_x = bild_pos(t_ptr->xpos,l_ptr,t_ptr->loc_opt->tab,t_ptr->loc_opt->tabsize);
		t_ptr->up_down = TRUE;
	}
	if (delta < 0) /* rauf */
	{
		if (t_ptr->ypos==0) return FALSE;
		delta = -delta;
		if (delta > t_ptr->ypos)
			delta = t_ptr->ypos;
		t_ptr->ypos -= delta;
		while (--delta>=0) PREV(l_ptr);
		t_ptr->cursor_line = l_ptr;
		t_ptr->xpos = inter_pos(t_ptr->desire_x,l_ptr,t_ptr->loc_opt->tab,t_ptr->loc_opt->tabsize);
		return TRUE;
	}
	else if (delta>0) /* runter */
	{
		long rest = t_ptr->text.lines-t_ptr->ypos-1;

		if (rest==0) return FALSE;
		if (delta>rest) delta = (short)rest;
		t_ptr->ypos += delta;
		while (--delta>=0)	NEXT(l_ptr);
		t_ptr->cursor_line = l_ptr;
		t_ptr->xpos = inter_pos(t_ptr->desire_x,l_ptr,t_ptr->loc_opt->tab,t_ptr->loc_opt->tabsize);
		return TRUE;
	}
	return FALSE;
}


/*
 * Bewegen im Text
*/
static void text_start(TEXTP t_ptr)
{
	t_ptr->up_down = FALSE;
	t_ptr->cursor_line = FIRST(&t_ptr->text);
	t_ptr->xpos = 0;
	t_ptr->ypos = 0;
	make_chg(t_ptr->link,POS_CHANGE,0);
}

static void text_end(TEXTP t_ptr)
{
	t_ptr->up_down = FALSE;
	t_ptr->cursor_line = LAST(&t_ptr->text);
	t_ptr->xpos = 0;
	t_ptr->ypos = t_ptr->text.lines-1;
	make_chg(t_ptr->link,POS_CHANGE,0);
}

static void page_up(TEXTP t_ptr)
{
	WINDOWP window = get_window(t_ptr->link);

	if (pos_move(t_ptr, - window->w_height))
	{
		make_chg(t_ptr->link,MOVE_DOWN,window->w_height);
		make_chg(t_ptr->link,POS_CHANGE,0);
	}
	else
		end_play();
}

static void line_up(TEXTP t_ptr)
{
	LINEP l_ptr = t_ptr->cursor_line;
	if (!t_ptr->up_down)
	{
		t_ptr->desire_x = bild_pos(t_ptr->xpos,l_ptr,t_ptr->loc_opt->tab,t_ptr->loc_opt->tabsize);
		t_ptr->up_down = TRUE;
	}
	if (t_ptr->ypos)
	{
		PREV(l_ptr);
		t_ptr->cursor_line = l_ptr;
		t_ptr->xpos = inter_pos(t_ptr->desire_x,l_ptr,t_ptr->loc_opt->tab,t_ptr->loc_opt->tabsize);
		t_ptr->ypos--;
		make_chg(t_ptr->link,POS_CHANGE,0);
	}
	else
		end_play();
}

static void page_down(TEXTP t_ptr)
{
	WINDOWP window = get_window(t_ptr->link);

	if (pos_move(t_ptr, window->w_height))
	{
		make_chg(t_ptr->link,MOVE_UP,window->w_height);
		make_chg(t_ptr->link,POS_CHANGE,0);
	}
	else
		end_play();
}

static void line_down(TEXTP t_ptr)
{
	LINEP l_ptr = t_ptr->cursor_line;

	if (!t_ptr->up_down)
	{
		t_ptr->desire_x = bild_pos(t_ptr->xpos,l_ptr,t_ptr->loc_opt->tab,t_ptr->loc_opt->tabsize);
		t_ptr->up_down = TRUE;
	}
	if (!IS_LAST(t_ptr->cursor_line))
	{
		NEXT(l_ptr);
		t_ptr->cursor_line = l_ptr;
		t_ptr->xpos = inter_pos(t_ptr->desire_x,l_ptr,t_ptr->loc_opt->tab,t_ptr->loc_opt->tabsize);
		t_ptr->ypos++;
		make_chg(t_ptr->link,POS_CHANGE,0);
	}
	else
		end_play();
}

static void line_start(TEXTP t_ptr)
{
	t_ptr->up_down = FALSE;
	t_ptr->xpos = 0;
	make_chg(t_ptr->link,POS_CHANGE,0);
}

static void line_end(TEXTP t_ptr)
{
	t_ptr->up_down = FALSE;
	t_ptr->xpos = t_ptr->cursor_line->len;
	make_chg(t_ptr->link,POS_CHANGE,0);
}

static void word_left(TEXTP t_ptr)
{
	short	xw;
	long	yw;
	char *str;
	LINEP l_ptr;

	t_ptr->up_down = FALSE;
	l_ptr = t_ptr->cursor_line;
	xw = t_ptr->xpos;
	yw = t_ptr->ypos;
	str = TEXT(l_ptr) + xw;
	do 									/* Ersten Buchstaben finden */
	{
		while (xw == 0)
		{
			if (yw == 0) return;
			PREV(l_ptr);
			yw--;
			xw = l_ptr->len;
			str = TEXT(l_ptr) + xw;
		}
		xw--;
	}while (!setin(t_ptr->loc_opt->wort_set, *(--str)));

	while (xw > 0) 						/* Ersten NICHT-Buchstaben suchen */
	{
		xw--;
		if (!setin(t_ptr->loc_opt->wort_set, *(--str)))
		{
			xw++;
			break;
		}
	}
	t_ptr->cursor_line = l_ptr;
	t_ptr->xpos = xw;
	t_ptr->ypos = yw;
	make_chg(t_ptr->link,POS_CHANGE,0);
}

static void word_right(TEXTP t_ptr)
{
	short	xw;
	long	yw;
	char *str;
	LINEP l_ptr;

	t_ptr->up_down = FALSE;
	l_ptr = t_ptr->cursor_line;
	xw = t_ptr->xpos;
	yw = t_ptr->ypos;
	str = TEXT(l_ptr)+xw;
	while (TRUE)						/* erster nicht-wort-Buchstabe */
	{
		if (xw==l_ptr->len || !setin(t_ptr->loc_opt->wort_set,*str))
			break;
		str++; xw++;
	}
	while (TRUE)						/* erster wort-Buchstabe */
	{
		while (xw==l_ptr->len)
		{
			if (IS_LAST(l_ptr))
				goto ende;
			NEXT(l_ptr);
			yw++;
			xw = 0;
			str = TEXT(l_ptr);
		}
		if (setin(t_ptr->loc_opt->wort_set,*str))
			break;
		str++; xw++;
	}
ende:
	t_ptr->cursor_line = l_ptr;
	t_ptr->xpos = xw;
	t_ptr->ypos = yw;
	make_chg(t_ptr->link,POS_CHANGE,0);
}

void char_left(TEXTP t_ptr)				/* -> kurzel.c */
{
	if (t_ptr->xpos)
	{
		t_ptr->xpos--;
		make_chg(t_ptr->link,POS_CHANGE,0);
	}
	else if (t_ptr->ypos)
	{
		LINEP l_ptr = t_ptr->cursor_line;

		PREV(l_ptr);
		t_ptr->cursor_line = l_ptr;
		t_ptr->xpos = l_ptr->len;
		t_ptr->ypos--;
		make_chg(t_ptr->link,POS_CHANGE,0);
	}
	else
		end_play();
}

static void char_right(TEXTP t_ptr)
{
	LINEP l_ptr = t_ptr->cursor_line;

	if (t_ptr->xpos<l_ptr->len)
	{
		t_ptr->xpos++;
		make_chg(t_ptr->link,POS_CHANGE,0);
	}
	else if (!IS_LAST(l_ptr))
	{
		NEXT(l_ptr);
		t_ptr->cursor_line = l_ptr;
		t_ptr->xpos = 0;
		t_ptr->ypos++;
		make_chg(t_ptr->link,POS_CHANGE,0);
	}
	else
		end_play();
}


/*
 * Text hinzufgen
*/
void char_cr(TEXTP t_ptr) 				/* -> kurzel.c */
{
	t_ptr->blk_mark_mode = FALSE;
	t_ptr->up_down = FALSE;
	clr_undo();
	if (t_ptr->block)
		blk_delete(t_ptr);
	t_ptr->moved++;
	col_split(&t_ptr->text, &t_ptr->cursor_line,t_ptr->xpos);
	t_ptr->cursor_line->info |= ABSATZ;
	t_ptr->text.lines++;
	NEXT(t_ptr->cursor_line);
	t_ptr->ypos++;
	if (t_ptr->loc_opt->einruecken)
	{
		t_ptr->xpos = col_einrucken(&t_ptr->text,&t_ptr->cursor_line);
		hl_update( t_ptr );
	}
	else
		t_ptr->xpos = 0;

/*
Problem: Wenn ein Block markiert war, mu eventuell gescrollt werden
         und es kommt zu Redraw-Fehlern, da doch mehr Zeilen betroffen sind!

	make_chg(t_ptr->link,SCROLL_DOWN,t_ptr->ypos);
	make_chg(t_ptr->link,LINE_CHANGE,t_ptr->ypos-1);
	make_chg(t_ptr->link,LINE_CHANGE,t_ptr->ypos);
*/
	make_chg(t_ptr->link,TOTAL_CHANGE,0);
	make_chg(t_ptr->link,POS_CHANGE,0);
	if (t_ptr->loc_opt->umbrechen)
		umbruch(t_ptr);
	t_ptr->max_line = NULL;
}

void char_insert(TEXTP t_ptr, char c)
{
	t_ptr->up_down = FALSE;

	/* Null nur in Binr erlaubt! */
	if (c == 0 && t_ptr->text.ending != lns_binmode)
	{
		Bconout(2, 7);
		return;
	}
	
	if (t_ptr->block)
		blk_delete(t_ptr);
	get_undo_col(t_ptr);
	if (overwrite && t_ptr->xpos < t_ptr->cursor_line->len)
	{
		TEXT(t_ptr->cursor_line)[t_ptr->xpos] = c;
	}
	else
	{
		if (t_ptr->cursor_line->len == MAX_LINE_LEN)
		{
			inote(1, 0, TOOLONG, MAX_LINE_LEN);
			return;
		}
		*(REALLOC(&t_ptr->cursor_line,t_ptr->xpos,1)) = c;
	}
	t_ptr->xpos++;
	t_ptr->moved++;
	make_chg(t_ptr->link,LINE_CHANGE,t_ptr->ypos);
	make_chg(t_ptr->link,POS_CHANGE,0);
	if (t_ptr->loc_opt->umbrechen)
		umbruch(t_ptr);
}

static void tabulator(TEXTP t_ptr)
{
	t_ptr->blk_mark_mode = FALSE;
	t_ptr->up_down = FALSE;
	if (t_ptr->block)
		blk_delete(t_ptr);
	if (t_ptr->loc_opt->tab)
		char_insert(t_ptr, '\t');
	else
	{
		short xw, tabsize = t_ptr->loc_opt->tabsize;

		xw = bild_pos(t_ptr->xpos,t_ptr->cursor_line,t_ptr->loc_opt->tab,tabsize);
		xw = tabsize-(xw%tabsize);
		while((--xw) >= 0)
			char_insert(t_ptr, ' ');
	}
}

void char_swap(TEXTP t_ptr)		/* -> menu.c */
{
	char 	c;
	short	x;

	x = t_ptr->xpos;
	if (x > 0 && x < t_ptr->cursor_line->len)
	{
		get_undo_col(t_ptr);
		c = TEXT(t_ptr->cursor_line)[x];
		TEXT(t_ptr->cursor_line)[x] = TEXT(t_ptr->cursor_line)[x-1];
		TEXT(t_ptr->cursor_line)[x-1] = c;
		t_ptr->xpos++;
		t_ptr->moved++;
		make_chg(t_ptr->link,LINE_CHANGE,t_ptr->ypos);
		make_chg(t_ptr->link,POS_CHANGE,0);
	}
}

static void expand_kurzel(TEXTP t_ptr)
{
	if (krz_loaded)
	{
		blk_demark(t_ptr);
		if (ctrl_mark_mode)
			t_ptr->blk_mark_mode = FALSE;
		t_ptr->up_down = FALSE;
		do_kurzel(t_ptr, FALSE);
	}
	else
		Bconout(2, 7);
}


/*
 * Text enfernen
*/
void char_bs(TEXTP t_ptr) 			/* -> kurzel.c */
{
	if (IS_FIRST(t_ptr->cursor_line) && t_ptr->xpos == 0)
	{
		end_play(); 			/* Makro beenden */
	}
	else
	{
		char_left(t_ptr);
		char_delete(t_ptr);
	}
}

static void char_delete(TEXTP t_ptr)
{
	LINEP col = t_ptr->cursor_line;

	if (t_ptr->xpos < col->len)
	{
		get_undo_col(t_ptr);
		t_ptr->moved++;
		REALLOC(&t_ptr->cursor_line,t_ptr->xpos,-1);
		make_chg(t_ptr->link,LINE_CHANGE,t_ptr->ypos);
		make_chg(t_ptr->link,POS_CHANGE,0); 					/* wg. moved */
	}
	else if (!IS_LAST(col))
	{
		if (col->len + col->next->len > MAX_LINE_LEN)
		{
			inote(1, 0, TOOLONG, MAX_LINE_LEN);
			return;
		}
		clr_undo();
		t_ptr->moved++;
		col_concate(&t_ptr->text, &t_ptr->cursor_line);
		t_ptr->text.lines--;
		make_chg(t_ptr->link, SCROLL_UP, t_ptr->ypos + 1);
		make_chg(t_ptr->link, LINE_CHANGE, t_ptr->ypos);
		make_chg(t_ptr->link, POS_CHANGE, 0);
	}
	else
		end_play(); 			/* Makro beenden */
	
	if (t_ptr->loc_opt->umbrechen)
		umbruch(t_ptr);
}

static void word_bs(TEXTP t_ptr)
/* Lschen wortweise nach links */
{
	short	xpos = t_ptr->xpos - 1;
	bool	in_word = FALSE;

	t_ptr->blk_mark_mode = FALSE;
	t_ptr->up_down = FALSE;
	if (!t_ptr->block)
	{
		blk_mark(t_ptr,0);
		while (setin(t_ptr->loc_opt->wort_set, TEXT(t_ptr->cursor_line)[xpos]) && xpos > 0)
		{
			xpos--;
			in_word = TRUE;
		}
		if (!in_word)
			while (!setin(t_ptr->loc_opt->wort_set, TEXT(t_ptr->cursor_line)[xpos]) && xpos > 0)
				xpos--;
		if (xpos == 0 && t_ptr->xpos == 1)		/* erstes Zeichen */
			t_ptr->xpos = 0;
		else
			t_ptr->xpos = xpos + 1;
		blk_mark(t_ptr,1);
	}
	blk_delete(t_ptr);
}
// how (does)  this behave
// how (does)  this behave

// this is a longonly
static void ctrl_word_bs(TEXTP t_ptr)
/* Lschen wortweise nach links */
{
	short	xpos = t_ptr->xpos;
	bool in_word = FALSE;

	if (!xpos)
	{
		if (t_ptr->ypos)
		{
			char_left(t_ptr);
			char_delete(t_ptr);
		}
	}
	else
	{
// 		xpos--;
		t_ptr->blk_mark_mode = FALSE;
		t_ptr->up_down = FALSE;
		if (xpos >= 0 && !t_ptr->block)
		{
			if (xpos) xpos--;

			blk_mark(t_ptr, 0);

			while (!setin(t_ptr->loc_opt->wort_set, TEXT(t_ptr->cursor_line)[xpos]) && xpos > 0)
				xpos--;

			while (setin(t_ptr->loc_opt->wort_set, TEXT(t_ptr->cursor_line)[xpos]) && xpos > 0)
			{
				xpos--;
				in_word = TRUE;
			}
			
			if (in_word && !setin(t_ptr->loc_opt->wort_set, TEXT(t_ptr->cursor_line)[xpos]))
 				xpos++;
			t_ptr->xpos = xpos;
			blk_mark(t_ptr,1);
		}
		blk_delete(t_ptr);
	}
}

// this is a longonly
static void ctrl_word_delete(TEXTP t_ptr)
/* Lschen wortweise nach links */
{
	short	xpos = t_ptr->xpos;
	bool	in_whites = FALSE;

	if (xpos == t_ptr->cursor_line->len)
	{
		if (!IS_LAST(t_ptr->cursor_line))
		{
			char_delete(t_ptr);
		}
	}
	else
	{
		t_ptr->blk_mark_mode = FALSE;
		t_ptr->up_down = FALSE;
		if (xpos >= 0 && !t_ptr->block)
		{
			blk_mark(t_ptr,0);

			while (!setin(t_ptr->loc_opt->wort_set, TEXT(t_ptr->cursor_line)[xpos]) && t_ptr->cursor_line->len > xpos)
			{
				xpos++;
				in_whites = TRUE;
			}
			if (!in_whites)
			{
				while (setin(t_ptr->loc_opt->wort_set, TEXT(t_ptr->cursor_line)[xpos]) && t_ptr->cursor_line->len > xpos)
					xpos++;
				while (!setin(t_ptr->loc_opt->wort_set, TEXT(t_ptr->cursor_line)[xpos]) && t_ptr->cursor_line->len > xpos)
					xpos++;
			}
			t_ptr->xpos = xpos;
			blk_mark(t_ptr,1);
		}
		blk_delete(t_ptr);
	}
}

static void word_delete(TEXTP t_ptr)
/* Lschen bis zum nchsten Wortanfang */
{
	short xpos = t_ptr->xpos;
	bool	in_word = FALSE;

	t_ptr->blk_mark_mode = FALSE;
	t_ptr->up_down = FALSE;
	if (!t_ptr->block)
	{
		blk_mark(t_ptr,0);
		while (setin(t_ptr->loc_opt->wort_set,TEXT(t_ptr->cursor_line)[xpos])
				&& t_ptr->cursor_line->len > xpos)
		{
			xpos++;
			in_word = TRUE;
		}
		if (!in_word)
			while (!setin(t_ptr->loc_opt->wort_set,TEXT(t_ptr->cursor_line)[xpos])
					&& t_ptr->cursor_line->len > xpos)
			xpos++;
		t_ptr->xpos = xpos;
		blk_mark(t_ptr,1);
	}
	blk_delete(t_ptr);
}

static void ctrl_y(TEXTP t_ptr)
{
	LINEP col = t_ptr->cursor_line;

	if (!t_ptr->block)
	{
		t_ptr->xpos = 0;
		blk_mark(t_ptr, 0);
		if (IS_LAST(col))					/* letzte Zeile im Text nur krzen */
		{
			t_ptr->xpos = col->len;
			blk_mark(t_ptr,1);
		}
		else
		{
			NEXT(col);
			t_ptr->cursor_line = col;
			t_ptr->ypos++;
			blk_mark(t_ptr,1);
		}
	}
	blk_cut(t_ptr);
}

static void set_block(TEXTP t_ptr)
{
	if (!t_ptr->blk_mark_mode)
	{
		blk_mark(t_ptr, 0);
		t_ptr->blk_mark_mode = TRUE;
	}
}

static void unset_block(TEXTP t_ptr)
{
	blk_demark(t_ptr);
	t_ptr->blk_mark_mode = FALSE;
}

/* Deselektiert Block und stellt Cursor entsprechend <dir> */
static void deselect_block(TEXTP t_ptr, short dir)
{
	switch (dir)
	{
		case UP		:
			desire_x = bild_pos(t_ptr->x1, t_ptr->cursor_line, t_ptr->loc_opt->tab, t_ptr->loc_opt->tabsize);
			desire_y = t_ptr->z1;
			break;
		case DOWN	:
			desire_x = bild_pos(t_ptr->x2, t_ptr->cursor_line, t_ptr->loc_opt->tab, t_ptr->loc_opt->tabsize);
			desire_y = t_ptr->z2;
			break;
		case LEFT	:
			desire_x = bild_pos(t_ptr->x1, t_ptr->cursor_line, t_ptr->loc_opt->tab, t_ptr->loc_opt->tabsize);
			desire_y = t_ptr->z1;
			t_ptr->up_down = FALSE;
			break;
		case RIGHT	:
			desire_x = bild_pos(t_ptr->x2, t_ptr->cursor_line, t_ptr->loc_opt->tab, t_ptr->loc_opt->tabsize);
			desire_y = t_ptr->z2;
			t_ptr->up_down = FALSE;
			break;
	}
	blk_demark(t_ptr);
	if (ctrl_mark_mode)
		t_ptr->blk_mark_mode = FALSE;
	icon_edit(t_ptr->link, DO_GOTO);
}


bool edit_key(TEXTP t_ptr, WINDOWP window, short kstate, short kreturn)
{
	unsigned short	nkey;
	unsigned char	ascii_code;
	bool		shift, ctrl, alt;
	
	/* Key konvertieren */	
	nkey = gem_to_norm(kstate, kreturn);
	nkey &= ~(NKF_RESVD|NKF_SHIFT|NKF_CTRL|NKF_CAPS);
	ascii_code = nkey & 0x00FF;
	shift = (kstate & (K_RSHIFT|K_LSHIFT)) != 0;
	ctrl = (kstate & K_CTRL) != 0;
	alt = (kstate & K_ALT) != 0;

	/* Sonderbehandlung fr ^Y */
	if (ascii_code == 'Y' && ctrl)
	{
		cursor_visible(window, t_ptr);
		t_ptr->blk_mark_mode = FALSE;
		ctrl_y(t_ptr);
		t_ptr->up_down = FALSE;
		return TRUE;
	}

	if (alt_cnt != -1 && !(nkey & NKF_FUNC))
		alt_cnt = -1;

	if (nkey & NKF_FUNC)
	{
		nkey &= ~NKF_FUNC;
		if (shift && ctrl)										/* Shift-Ctrl */
		{
			switch (nkey)
			{
				case NK_UP :
					if (ctrl_mark_mode)
					{
						if (!t_ptr->block)
							cursor_visible(window, t_ptr);
						set_block(t_ptr);
						page_up(t_ptr);
						blk_mark(t_ptr,1);
					}
					break;
				case NK_DOWN :
					if (ctrl_mark_mode)
					{
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
						set_block(t_ptr);
						page_down(t_ptr);
						blk_mark(t_ptr,1);
					}
					break;
				case NK_LEFT :
					if (ctrl_mark_mode)
					{
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
						set_block(t_ptr);
						word_left(t_ptr);
						blk_mark(t_ptr,1);
					}
					break;
				case NK_RIGHT :
					if (ctrl_mark_mode)
					{
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
						set_block(t_ptr);
						word_right(t_ptr);
						blk_mark(t_ptr,1);
					}
					break;
				default:
					return FALSE;
			}
		}
		else if (shift)												/* alle Shift-Codes */
		{
			switch (nkey)
			{
				case NK_CLRHOME:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if (ctrl_mark_mode)
						unset_block(t_ptr);
					else if (!t_ptr->blk_mark_mode)
						blk_demark(t_ptr);
					text_end(t_ptr);
					if (!ctrl_mark_mode && t_ptr->blk_mark_mode)
						blk_mark(t_ptr,1);
					break;
				case NK_UP:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if (ctrl_mark_mode)
						unset_block(t_ptr);
					else if (!t_ptr->blk_mark_mode)
						blk_demark(t_ptr);
					page_up(t_ptr);
					if (!ctrl_mark_mode && t_ptr->blk_mark_mode)
						blk_mark(t_ptr,1);
					break;
				case NK_DOWN:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if (ctrl_mark_mode)
						unset_block(t_ptr);
					else if (!t_ptr->blk_mark_mode)
						blk_demark(t_ptr);
					page_down(t_ptr);
					if (!ctrl_mark_mode && t_ptr->blk_mark_mode)
						blk_mark(t_ptr,1);
					break;
				case NK_LEFT:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if (ctrl_mark_mode)
						unset_block(t_ptr);
					else if (!t_ptr->blk_mark_mode && t_ptr->block)
						blk_demark(t_ptr);
					line_start(t_ptr);
					if (!ctrl_mark_mode && t_ptr->blk_mark_mode)
						blk_mark(t_ptr,1);
					break;
				case NK_RIGHT:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if (ctrl_mark_mode)
						unset_block(t_ptr);
					else if (!t_ptr->blk_mark_mode && t_ptr->block)
						blk_demark(t_ptr);
					line_end(t_ptr);
					if (!ctrl_mark_mode && t_ptr->blk_mark_mode)
						blk_mark(t_ptr,1);
					break;
				case NK_BS:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					word_bs(t_ptr);
					break;
				case NK_DEL:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					word_delete(t_ptr);
					break;
				case NK_RET :
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					word_left(t_ptr);
					char_cr(t_ptr);
					break;
				default:
					return FALSE;
			} /* switch shift */
		} /* if shift */
		else if (ctrl) 											/* alle Control-Codes */
		{
			switch (nkey)
			{
				case NK_UP:
					if (ctrl_mark_mode)
					{
						if (!t_ptr->block)
							cursor_visible(window, t_ptr);
						set_block(t_ptr);
						line_up(t_ptr);
						blk_mark(t_ptr,1);
					}
					else 	/* seitenweise Blttern, fr PC-Tastaturen PgUp/Down */
					{
						arrow_window(window, WA_UPLINE, 1);
					#if 0
						if (!t_ptr->block)
							cursor_visible(window, t_ptr);
						if (!t_ptr->blk_mark_mode && t_ptr->block)
							deselect_block(t_ptr, UP);
						else
						{
							page_up(t_ptr);
							if (t_ptr->blk_mark_mode)
								blk_mark(t_ptr,1);
						}
					#endif
					}
					break;
				case NK_DOWN:
					if (ctrl_mark_mode)
					{
						if (!t_ptr->block)
							cursor_visible(window, t_ptr);
						set_block(t_ptr);
						line_down(t_ptr);
						blk_mark(t_ptr,1);
					}
					else
					{
						arrow_window(window, WA_DNLINE, 1);
					#if 0
						if (!t_ptr->block)
							cursor_visible(window, t_ptr);
						if (!t_ptr->blk_mark_mode && t_ptr->block)
							deselect_block(t_ptr, DOWN);
						else
						{
							page_down(t_ptr);
							if (t_ptr->blk_mark_mode)
								blk_mark(t_ptr,1);
						}
					#endif
					}
					break;
				case NK_LEFT:
					if (ctrl_mark_mode)
					{
						if (!t_ptr->block)
							cursor_visible(window, t_ptr);
						set_block(t_ptr);
						t_ptr->up_down = FALSE;
						char_left(t_ptr);
						blk_mark(t_ptr,1);
					}
					else
					{
					#if 0
						arrow_window(window, WA_LFLINE, 1);
					#else
						if (!t_ptr->block)
							cursor_visible(window, t_ptr);
						if (!t_ptr->blk_mark_mode && t_ptr->block)
							deselect_block(t_ptr, LEFT);
						else
						{
							word_left(t_ptr);
							if (t_ptr->blk_mark_mode)
								blk_mark(t_ptr,1);
						}
					#endif
					}
					break;
				case NK_RIGHT:
					if (ctrl_mark_mode)
					{
						if (!t_ptr->block)
							cursor_visible(window, t_ptr);
						set_block(t_ptr);
						t_ptr->up_down = FALSE;
						char_right(t_ptr);
						blk_mark(t_ptr,1);
					}
					else
					{
					#if 0
						arrow_window(window, WA_RTLINE, 1);
					#else
						if (!t_ptr->block)
							cursor_visible(window, t_ptr);
						if (!t_ptr->blk_mark_mode && t_ptr->block)
							deselect_block(t_ptr, RIGHT);
						else
						{
							word_right(t_ptr);
							if (t_ptr->blk_mark_mode)
								blk_mark(t_ptr,1);
						}
					#endif
					}
					break;
				case NK_TAB:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if (!ctrl_mark_mode)
						t_ptr->blk_mark_mode = FALSE;
					t_ptr->up_down = FALSE;
					if (!blk_mark_brace(t_ptr))
						blk_mark_word(t_ptr);
					hl_update( t_ptr );
					break;
				case NK_DEL :
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if (ctrl_mark_mode)
					{
						t_ptr->up_down = FALSE;
						do_icon(t_ptr->link, DO_CUT);
					}
					else
						ctrl_word_delete(t_ptr);
					hl_update( t_ptr );
					break;
				case NK_INS:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if (ctrl_mark_mode)
					{
						t_ptr->up_down = FALSE;
						do_icon(t_ptr->link, DO_PASTE);
					}
					hl_update( t_ptr );
					break;
				case NK_CLRHOME:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if (ctrl_mark_mode)
					{
						t_ptr->up_down = FALSE;
						if (t_ptr->block)
							do_icon(t_ptr->link, DO_COPY);
						else
							do_icon(t_ptr->link, DO_LINECOPY);
					}
					break;
				case NK_BS:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					ctrl_word_bs(t_ptr);
					hl_update(t_ptr);
					break;

				default:
					return FALSE;
			}
		} /* if ctrl */
		else if (alt)												/* alle Alt-Codes */
		{
			char	c;
			short	i;
			
			c = (char)nkey;
			if ((nkey & NKF_NUM) && c >= '0' && c <= '9')
			{
				if (!t_ptr->block)
					cursor_visible(window, t_ptr);
				if (alt_cnt < 2)
				{
					alt_cnt++;
					alt_str[alt_cnt] = c;
				}
				if (alt_cnt == 2)
				{
					alt_cnt = -1;
					alt_str[3] = EOS;
					i = atoi(alt_str);
					if (i < 256)
						char_insert(t_ptr, i);
				}
				return TRUE;
			}
			else			
				return FALSE;
		} /* if alt */
		else										/* alles restliche Funktionscodes */
		{
			switch (nkey)
			{
				case NK_ESC :
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					expand_kurzel(t_ptr);
					break;
				case NK_TAB:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					tabulator(t_ptr);
					hl_update( t_ptr );
					break;
				case NK_RET:
				case (NK_ENTER|NKF_NUM) :
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					char_cr(t_ptr);
					break;
				case NK_CLRHOME:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if (ctrl_mark_mode)
						unset_block(t_ptr);
					else if (!t_ptr->blk_mark_mode)
						blk_demark(t_ptr);
					text_start(t_ptr);
					if (!ctrl_mark_mode && t_ptr->blk_mark_mode)
						blk_mark(t_ptr,1);
					break;
				case NK_UNDO:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					icon_edit(t_ptr->link, DO_UNDO);
					break;
				case NK_UP:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if ((ctrl_mark_mode || !t_ptr->blk_mark_mode) && t_ptr->block)
					{
						deselect_block(t_ptr, UP);
						break;
					}
					line_up(t_ptr);
					if (!ctrl_mark_mode && t_ptr->blk_mark_mode)
						blk_mark(t_ptr,1);
					break;
				case NK_DOWN:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if ((ctrl_mark_mode || !t_ptr->blk_mark_mode) && t_ptr->block)
					{
						deselect_block(t_ptr, DOWN);
						break;
					}
					line_down(t_ptr);
					if (!ctrl_mark_mode && t_ptr->blk_mark_mode)
						blk_mark(t_ptr,1);
					break;
				case NK_LEFT:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					t_ptr->up_down = FALSE;
					if ((ctrl_mark_mode || !t_ptr->blk_mark_mode) && t_ptr->block)
					{
						deselect_block(t_ptr, LEFT);
						break;
					}
					char_left(t_ptr);
					if (!ctrl_mark_mode && t_ptr->blk_mark_mode)
						blk_mark(t_ptr,1);
					break;
				case NK_RIGHT:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					t_ptr->up_down = FALSE;
					if ((ctrl_mark_mode || !t_ptr->blk_mark_mode) && t_ptr->block)
					{
						deselect_block(t_ptr, RIGHT);
						break;
					}
					char_right(t_ptr);
					if (!ctrl_mark_mode && t_ptr->blk_mark_mode)
						blk_mark(t_ptr,1);
					break;
				case NK_BS:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					t_ptr->blk_mark_mode = FALSE;
					t_ptr->up_down = FALSE;
					if (t_ptr->block)
						blk_delete(t_ptr);
					else
						char_bs(t_ptr);
					hl_update( t_ptr );
					break;
				case NK_DEL:
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					t_ptr->blk_mark_mode = FALSE;
					t_ptr->up_down = FALSE;
					if (t_ptr->block)
						blk_delete(t_ptr);
					else
						char_delete(t_ptr);
					hl_update( t_ptr );
					break;
				case NK_M_PGUP:				/* Mac: page up -> shift-up */
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if (ctrl_mark_mode)
						unset_block(t_ptr);
					else if (!t_ptr->blk_mark_mode)
						blk_demark(t_ptr);
					page_up(t_ptr);
					if (!ctrl_mark_mode && t_ptr->blk_mark_mode)
						blk_mark(t_ptr,1);
					break;
				case NK_M_PGDOWN: 			/* Mac: page down -> shift-down */
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if (ctrl_mark_mode)
						unset_block(t_ptr);
					else if (!t_ptr->blk_mark_mode)
						blk_demark(t_ptr);
					page_down(t_ptr);
					if (!ctrl_mark_mode && t_ptr->blk_mark_mode)
						blk_mark(t_ptr,1);
					break;
				case NK_M_END: 				/* Mac: end -> shift-home */
					if (!t_ptr->block)
						cursor_visible(window, t_ptr);
					if (ctrl_mark_mode)
						unset_block(t_ptr);
					else if (!t_ptr->blk_mark_mode)
						blk_demark(t_ptr);
					text_end(t_ptr);
					if (!ctrl_mark_mode && t_ptr->blk_mark_mode)
						blk_mark(t_ptr,1);
					break;
				default:
					return FALSE;
			}
		}
	} /* if NKF_FUNC */
	else													/* keine Funktionstaste */
	{
		if (!t_ptr->block)
			cursor_visible(window, t_ptr);
		if (ascii_code)
		{
			t_ptr->blk_mark_mode = FALSE;
			char_insert(t_ptr, ascii_code);
			if (krz_loaded)
				do_kurzel(t_ptr, TRUE);
			hl_update( t_ptr );
		}
		else
			return FALSE;
	}
	return TRUE;
}
