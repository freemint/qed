#include "global.h"
#include "ausgabe.h"
#include "block.h"
#include "clipbrd.h"
#include "edit.h"
#include "memory.h"
#include "set.h"
#include "tasten.h"
#include "text.h"
#include "window.h"
#include "umbruch.h"

#include "hl.h"

/* Muessen am Anfang jeden Umbruchs gesetzt werden */
static bool	tab;
static short		tab_size, lineal_len;
static SET		umbruch_set;

static bool Absatz		(LINEP col);
static bool too_int	(TEXTP t_ptr, LINEP col, long y);
static bool too_long 	(TEXTP t_ptr, LINEP col, long y);

void save_absatz(TEXTP t_ptr)
/* Es werden LZ und TABs am Absatzende gelscht */
/* und im Absatz hinzugefgt							*/
{
	bool		action = FALSE;
	short		i;
	char		c;
	LINEP	line;

	setcpy(umbruch_set,t_ptr->loc_opt->umbruch_set);/* !! Mu gesetzt werden !! */
	setincl(umbruch_set,' ');								/* wenigstens das */
	line = FIRST(&t_ptr->text);
	while (!IS_TAIL(line))
	{
		if (IS_ABSATZ(line))					/* Absatz => LZ und TAB lschen */
		{
			for (i=line->len; (--i)>=0 ; )
			{
				c = TEXT(line)[i];
				if (c!=' ' && c!='\t') break;
			}
			i++;
			if (i<line->len)		/* Zeile verkrzen */
			{
				REALLOC(&line,i,i-line->len);
				action = TRUE;
			}
		}
		else								/* Nicht Absatz => LZ anhngen */
		{
			c = TEXT(line)[line->len-1];
			if (!setin(umbruch_set,c) && line->len < MAX_LINE_LEN)
			{
				*REALLOC(&line,line->len,1) = ' ';
				action = TRUE;
			}
		}
		NEXT(line);
	}
	if (action)		/* Es wurde etwas verndert */
	{
		make_chg(t_ptr->link,TOTAL_CHANGE,0);
		t_ptr->moved++;
		line = t_ptr->cursor_line = get_line(&t_ptr->text,t_ptr->ypos);
		if (t_ptr->xpos>line->len)
		{
			t_ptr->xpos = line->len;
			make_chg(t_ptr->link,POS_CHANGE,0);
		}
	}
}

static bool Absatz(LINEP col)
/* TRUE : Die Zeile ist die letzte Zeile eines Absatz */
/* FALSE : sonst													*/
{
	short	col_len = col->len;
	char	c = TEXT(col)[col_len-1];

	return (col_len==0 || IS_LAST(col) || !setin(umbruch_set,c));
}

void make_absatz(TEXTP t_ptr)
/* Absatzmarkierungen anbringen oder lschen */
{
	LINEP line;

	setcpy(umbruch_set,t_ptr->loc_opt->umbruch_set);/* !! Mu gesetzt werden !! */
	setincl(umbruch_set,' ');								/* wenigstens das */
	line = FIRST(&t_ptr->text);
	if (t_ptr->loc_opt->umbrechen)
		while (!IS_TAIL(line))
		{
			if (Absatz(line))
				line->info |= ABSATZ;
			else
				line->info &= (~ABSATZ);		/* z.B. CR setzt immer Bit */
			NEXT(line);
		}
	else
		while (!IS_TAIL(line))
		{
			line->info &= (~ABSATZ);
			NEXT(line);
		}
}

static short long_brk(RINGP rp, LINEP col)
/* Zeile ist zu lang. Wo soll sie abgebrochen werden (mind. ein Wort) */
{
	short	off, pos;
	char	c, *str;

	off = col_offset(rp, col);
	pos = inter_pos(lineal_len,col,tab,tab_size);
	str = TEXT(col)+pos;
	pos--;
	c = *(--str);
   if (!setin(umbruch_set,c))
		while (pos>off && setin(umbruch_set,c))					/* Wortende suchen */
		{
			pos--;
			c = *(--str);
		}
	while (pos>off && !setin(umbruch_set,c))						/* Wortanfang suchen */
	{
		pos--;
		c = *(--str);
	}
	if (pos<=off)															/* nach rechts */
	{
		pos = inter_pos(lineal_len,col,tab,tab_size);
		str = TEXT(col)+pos;
		c = *str++;
		while (pos<=col->len && setin(umbruch_set,c))			/* Wortanf suchen */
		{
			pos++;
			c = *str++;
		}
		while (pos<=col->len && !setin(umbruch_set,c))			/* Wortende suchen */
		{
			pos++;
			c = *str++;
		}
		while (pos<=col->len && setin(umbruch_set,c))			/* Wortanf suchen */
		{
			pos++;
			c = *str++;
		}
		if (pos>=col->len)
			return 0;														/* nichts machen */
	}
	else
		pos++;
	return pos;
}

static short short_brk(RINGP rp, LINEP col, short len)
/* Einer Zeile fehlen Zeichen, sie ist jetzt im Bild len lang */
/* Wo soll der Nachfolger (col) hochgezogen werden (mind. ein Wort) */
{
	char	*str, c;
	short	pos, merk_pos;

	pos = col_offset(rp,col);
	str = TEXT(col)+pos;
	merk_pos = -1;
	if (!tab)
	{
		while (TRUE)
		{
			if (pos>=col->len) return col->len;					/* ganze Zeile */
			c = *str++;
			pos++;
			len++;
			if (len>lineal_len) break;								/* jetzt reichts */
			if (setin(umbruch_set,c))
				merk_pos = pos;
		}
	}
	else
	{
		short tabH;

		tabH = tab_size-(len%tab_size);
		while (TRUE)
		{
			if (pos>=col->len) return col->len;					/* ganze Zeile */
			c = *str++;
			pos++;
			if (c=='\t')
			{
				len += tabH;
				tabH = tab_size;
			}
			else
			{
				len++;
				if ((--tabH)==0) tabH = tab_size;
			}
			if (len>lineal_len) break;								/* jetzt reichts */
			if (setin(umbruch_set,c))
				merk_pos = pos;
		}
	}
	if (merk_pos>0) pos = merk_pos;
	else pos = 0;														/* nichts zu machen */
	return(pos);
}

/* !!! cursor_line mu hinterher entsprechend ypos gesetzt werden !!! */
static bool too_long(TEXTP t_ptr, LINEP col, long y)
{
	short	i, len, off;
	bool	absatz, weiter, changed;

	changed = FALSE;
	weiter = FALSE;
	while (bild_len(col,t_ptr->loc_opt->tab,t_ptr->loc_opt->tabsize)>lineal_len)							/* Zeile zu lang */
	{
		i = long_brk(&t_ptr->text,col);										/* wo abbrechen */
		if (i==0)
		{
			weiter = FALSE;
			break;
		}
		absatz = IS_ABSATZ(col);
		len = col->len-i;											/* soviel abschneiden */
		if (absatz || col->next->len + len >= MAX_LINE_LEN)	/* col_split */
		{
			LINEP help;

			col_split(&t_ptr->text,&col,i);
			t_ptr->text.lines++;
			if (t_ptr->ypos>y) t_ptr->ypos++;
			help = col->next;
			off = col_einrucken(&t_ptr->text, &help);
			hl_update_zeile( &t_ptr->text, help );
			if (t_ptr->ypos==y && t_ptr->xpos>i)			/* Cursor umbrechen */
			{
				t_ptr->ypos++;
				t_ptr->xpos -= (i-off);
			}
			weiter = TRUE;
		}
		else															/* Text rumschieben */
		{
			LINEP help = col->next;

			off = col_offset(&t_ptr->text,help);
			INSERT(&help,off,len,TEXT(col)+i);				/* Next verlngern */
			REALLOC(&col,i,-len);								/* Zeile krzen */
			hl_update_zeile( &t_ptr->text, col );
			hl_update_zeile( &t_ptr->text, help );
			if (t_ptr->ypos==y+1 && t_ptr->xpos>off)		/* Cursor verschieben */
			{
				t_ptr->xpos += len;
			}
			if (t_ptr->ypos==y && t_ptr->xpos>i)			/* Cursor umbrechen */
			{
				t_ptr->ypos++;
				t_ptr->xpos -= (i-off);
			}
			weiter = FALSE;
		}
		NEXT(col);													/* nchste Zeile zu lang? */
		y++;
		changed = TRUE;
	}
	if (weiter) too_int(t_ptr,col,y);						/* nchste Zeile zu kurz? */
	return (changed);
}

/* !!! cursor_line mu hinterher entsprechend ypos gesetzt werden !!! */
static bool too_int(TEXTP t_ptr, LINEP col, long y)
{
	short	len, off;
	LINEP	next_col;
	bool	changed;

	changed = FALSE;
	tab = t_ptr->loc_opt->tab;
	tab_size = t_ptr->loc_opt->tabsize;
	while (bild_len(col,t_ptr->loc_opt->tab,t_ptr->loc_opt->tabsize)<lineal_len && !(IS_ABSATZ(col)) && col->next->len > 0)
	{
		next_col = col->next;
		len = short_brk(&t_ptr->text,next_col,bild_len(col,t_ptr->loc_opt->tab,t_ptr->loc_opt->tabsize));
		if (len==0) break;										/* nichts zu machen */
		off = col_offset(&t_ptr->text,next_col);
		if (len==next_col->len)									/* ganze Zeile hochziehen */
		{
			if (t_ptr->ypos==y+1)								/* Cursor hochziehen */
			{
				t_ptr->ypos--;
				t_ptr->xpos += (col->len-off);
			}
			if (t_ptr->ypos>y+1) t_ptr->ypos--;
			REALLOC(&next_col,0,-off);
			col_concate(&t_ptr->text,&col);
			t_ptr->text.lines--;									/* gleiche Zeile nochmal */
		}
		else															/* teilweise hochziehen */
		{
			len -= off;
			if (t_ptr->ypos==y+1 && t_ptr->xpos>=off)
			{
				if (t_ptr->xpos<off+len)
				{
					t_ptr->ypos--;
					t_ptr->xpos += (col->len-off);
				}
				else
					t_ptr->xpos -= len;
			}
			INSERT(&col,col->len,len,TEXT(next_col)+off);/* Zeile verlngern */
			REALLOC(&next_col,off,-len);						/* Zeile verkrzen */
			hl_update_zeile( &t_ptr->text, col );
			hl_update_zeile( &t_ptr->text, next_col );
			col = next_col;										/* nchste Zeile weiter */
			y++;
		}
		changed = TRUE;
	}
	return(changed);
}

void umbruch(TEXTP t_ptr)
{
	long y = t_ptr->ypos;

	setcpy(umbruch_set,t_ptr->loc_opt->umbruch_set);/* !! Mu gesetzt werden !! */
	setincl(umbruch_set,' ');								/* wenigstens das */
	tab = t_ptr->loc_opt->tab;
	tab_size = t_ptr->loc_opt->tabsize;
	lineal_len = t_ptr->loc_opt->lineal_len;
	if (too_long(t_ptr, t_ptr->cursor_line, t_ptr->ypos))
	{
		t_ptr->cursor_line = get_line(&t_ptr->text,t_ptr->ypos);
		t_ptr->moved++;
		make_chg(t_ptr->link,POS_CHANGE,0);
		make_chg(t_ptr->link,TOTAL_CHANGE,y);
		clr_undo();
	}
	else if (too_int(t_ptr, t_ptr->cursor_line, t_ptr->ypos))
	{
		t_ptr->cursor_line = get_line(&t_ptr->text,t_ptr->ypos);
		t_ptr->moved++;
		make_chg(t_ptr->link,POS_CHANGE,0);
		make_chg(t_ptr->link,TOTAL_CHANGE,y);
		clr_undo();
	}
}

void format(TEXTP t_ptr)
{
	LINEP	line;
	long	y, start_y;
	bool	change;

	setcpy(umbruch_set,t_ptr->loc_opt->umbruch_set);/* !! Mu gesetzt werden !! */
	setincl(umbruch_set,' ');								/* wenigstens das */
	tab = t_ptr->loc_opt->tab;
	tab_size = t_ptr->loc_opt->tabsize;
	lineal_len = t_ptr->loc_opt->lineal_len;
	change = FALSE;
	line = t_ptr->cursor_line;
	y = t_ptr->ypos;
	if (y)
	{
		y--;
		PREV(line);
		while ((y > 0) && !(IS_ABSATZ(line)))
		{
			y--;
			PREV(line);
		}
		if (y)
		{
			y++;
			NEXT(line);
		}
	}
	/* line zeigt jetzt auf die erste Zeile des Absatz */
	start_y = y;
	line = line->prev;	/* Einen davor, weil akt. Zeile gendert wird */
	while(TRUE)
	{
		if (!too_long(t_ptr, line->next,y))
		{
			if (too_int(t_ptr, line->next,y))
				change = TRUE;
		}
		else
			change = TRUE;

		y++;
		NEXT(line);
		if (IS_ABSATZ(line))
			break;
	}
	if (change)
	{
		t_ptr->cursor_line = get_line(&t_ptr->text,t_ptr->ypos);
		t_ptr->moved++;
		make_chg(t_ptr->link,POS_CHANGE,0);
		make_chg(t_ptr->link,TOTAL_CHANGE,start_y);
		clr_undo();
	}
}

void total_format(TEXTP t_ptr)
{
	LINEP	line;
	long	y;
	bool	change;

	setcpy(umbruch_set,t_ptr->loc_opt->umbruch_set);/* !! Mu gesetzt werden !! */
	setincl(umbruch_set,' ');								/* wenigstens das */
	tab = t_ptr->loc_opt->tab;
	tab_size = t_ptr->loc_opt->tabsize;
	lineal_len = t_ptr->loc_opt->lineal_len;
	change = FALSE;
	graf_mouse(HOURGLASS, NULL);
	line = FIRST(&t_ptr->text);
	y = 0;
	while (!IS_TAIL(line))
	{
		/* line zeigt jetzt auf die erste Zeile des Absatz */
		line = line->prev;	/* Einen davor, weil akt. Zeile gendert wird */
		while(TRUE)
		{
			if (!too_long(t_ptr, line->next,y))
			{
				if (too_int(t_ptr, line->next,y))
					change = TRUE;
			}
			else
				change = TRUE;
			y++;
			NEXT(line);
			if (IS_ABSATZ(line))
				break;
		}
		NEXT(line);
	}
	if (change)
	{
		t_ptr->cursor_line = get_line(&t_ptr->text,t_ptr->ypos);
		t_ptr->moved++;
		make_chg(t_ptr->link,POS_CHANGE,0);
		make_chg(t_ptr->link,TOTAL_CHANGE,0);
		clr_undo();
	}
	graf_mouse(ARROW, NULL);
}
