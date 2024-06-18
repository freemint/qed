#include "global.h"
#include "edit.h"
#include "file.h"
#include "icon.h"
#include "memory.h"
#include "menu.h"
#include "options.h"
#include "rsc.h"
#include "tasten.h"
#include "text.h"
#include "window.h"
#include "kurzel.h"
#include "hl.h"

/*
 * Exportierte Variablen
 */
bool	krz_loaded;

#if __GNUC_PREREQ(7, 0)
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

/**************************************************************************/


#define MNAME_LEN		12
#define KRZ_MAX_LEN	8

typedef struct
{
	char	name[MNAME_LEN+1];
	PATH	file;
	long	y;
	short	x;
}MARKE, *MARKEP;

#define KRZ_LEN(col)	(TEXT(col)[0])
#define KRZ_TXT(col)	(TEXT(col)+1)
#define ERSATZ(col)	(TEXT(col)+1+KRZ_MAX_LEN+1)

static RING		kurz;
/* Sortierte Zeilen 																*/
/* Am Anfang in umgedrehter Reihenfolge die Zeichen 					*/
/* des K�rzel als Null-Terminierender String							 	*/
/* davor noch die l�nge des K�rzel (Pascal�hnlich)						*/
/* 0 K�rzell�nge, 1-9 K�rzel, ab der 10 Position der Ersatzstring	*/
static RING	auto_kurz;

static MARKE	Marken[MARKEN_ANZ];

static PATH		krz_name;	/* Name der aktuellen K�rzeldatei, oder leer */

/***************************************************************************/

static short  load_kurzel	(void);
static void clr_kurzel	(void);
static short  add_kurzel	(RINGP rp, ZEILEP col);

/***************************************************************************/

bool goto_line_dial (void)
{
	short	antw;
	char	s[12];

	set_string(pos, GZEILE, "");
	set_string(pos, GSPALTE, "");
	antw = simple_mdial(pos, GZEILE) & 0x7fff;
	if (antw == GOK)
	{
		get_string(pos, GZEILE, s);
		if (s[0] != EOS)
			desire_y = atol(s) - 1;
		else
			desire_y = 0L;
		get_string(pos, GSPALTE, s);
		if (s[0] != EOS)
			desire_x = atoi(s) - 1;
		else
			desire_x = 0;
		return TRUE;
	}
	return FALSE;
}

/***************************************************************************/
/* Verwaltung der Marken																	*/
/***************************************************************************/

void del_marke(short nr)
{
	MARKEP	m;
	char		*str;
	short		len;

	if (nr >= 0 && nr < MARKEN_ANZ)
	{
		m = Marken + nr;
		m->file[0] = EOS;
		m->name[0] = EOS;
		str = menu[MMARKE1+nr].ob_spec.free_string + 5;
		len = MNAME_LEN;
		while ((--len)>=0)
			*str++ = ' ';
	}
}

void set_marke(short nr, char *name, PATH file, long y, short x)
{
	MARKEP	m;

	if (nr >= 0 && nr < MARKEN_ANZ)
	{
		m = Marken + nr;
		m->x = x;
		m->y = y;
		strncpy(m->name, name, MNAME_LEN);
		m->name[MNAME_LEN] = EOS;
		strcpy(m->file, file);
		fillup_menu(MMARKE1 + nr, name, 5);
	}
}

bool get_marke(short nr, char *name, PATH file, long *y, short *x)
{
	file[0] = EOS;
	if (nr >= 0 && nr < MARKEN_ANZ)
	{
		MARKEP	m;

		m = Marken+nr;
		*x = m->x;
		*y = m->y;
		strncpy(name, m->name, MNAME_LEN);
		name[MNAME_LEN] = EOS;
		strcpy(file, m->file);
	}
	return (file[0] != EOS);
}

void init_marken(void)
{
	short		i;
	MARKEP	m;

	for (i = MARKEN_ANZ, m = Marken; (--i)>=0; m++)
	{
		m->file[0] = EOS;
		m->name[0] = EOS;
	}
	init_textring(&kurz);
	init_textring(&auto_kurz);
}

void goto_marke(short nr)
{
	PATH	file;
	char	name[MNAME_LEN + 1];
	long	y;
	short	x, icon;

	if (shift_pressed())
		del_marke(nr);
	else
	{
		get_marke(nr,name,file,&y,&x);
		if (file[0]!=EOS)
		{
			icon = load_edit(file, FALSE);
			if (icon >= 0)
			{
				desire_y = y;
				desire_x = x;
				icon_edit(icon,DO_GOTO);
			}
		}
	}
}

void config_marken(TEXTP t_ptr)
{
	short	antw;
	PATH	file;
	char	name[14];
	long	y;
	short	x, i;

	set_state(marken, MRK1, OS_SELECTED, TRUE);
	for (i = 1; i < MARKEN_ANZ; i++)
		set_state(marken, MRK1+i, OS_SELECTED, FALSE);
	for (i=0; i < MARKEN_ANZ; i++)
	{
		get_marke(i, name, file, &y, &x);
		set_string(marken, MRKTXT1+i, name);
	}
	antw = simple_mdial(marken, MRKTXT1);
	if (antw == MRKOK)
	{
		for (i = 0; i < MARKEN_ANZ; i++)
			if (get_state(marken, MRK1 + i, OS_SELECTED))
				break;
		get_string(marken, MRKTXT1+i, name);
		if (name[0] == EOS)
		{
			file_name(t_ptr->filename, name, FALSE);
			name[MNAME_LEN] = EOS;
		}
		set_marke(i, name, t_ptr->filename, t_ptr->ypos, t_ptr->xpos);
	}
}

/***************************************************************************/
/* Verwaltung der K�rzel																	*/
/***************************************************************************/

void clr_kurzel(void)
{
	free_textring(&kurz);
	free_textring(&auto_kurz);
	krz_loaded = FALSE;
}

void do_kurzel(TEXTP t_ptr, bool online)
{
	bool		set_pos, save_insert;
	short		xw, i, len;
	char		*str, buffer[KRZ_MAX_LEN+1];
	ZEILEP	col;
	RINGP		k;

	if (online)
		k = &auto_kurz;
	else
		k = &kurz;

	if (ist_leer(k))
		return;

	xw = t_ptr->xpos;
	if (xw == 0)
		return;
	str = TEXT(t_ptr->cursor_line)+xw;
	i = 0;
	len = min(xw,KRZ_MAX_LEN);
	while (len>0)														/* umdrehen */
	{
		buffer[i++] = *(--str);
		len--;
	}
	buffer[i] = EOS;													/* abschliessen */

	/* longest Match */
	for (col=FIRST(k); TRUE; NEXT(col))
	{
		if (IS_TAIL(col))
		{
			if (!online)			/* wenn man Auto-K�rzel hat, pings sonst bei */
				Bconout(2,7);		/* jedem Zeichen, das kein K�rzel ist! */
			return;
		}
		len = strncmp(buffer,KRZ_TXT(col),KRZ_LEN(col));
		if (len == 0)
			break;
		if (len > 0)
		{
			if (!online)			/* dito. */
				Bconout(2,7);
			return;
		}
	}

	/*
	 * Damit w�hrend der K�rzelexpandierung NICHT automatisch
	 * einger�ckt wird, merken wir uns die Einstellung und schalten es ab!
	 * K�rzel, die mehrzeilig sind und mit Blanks/TABs beginnen, erscheinen
	 * sonst nicht korrekt.
	*/
	save_insert = t_ptr->loc_opt->einruecken;
	t_ptr->loc_opt->einruecken = FALSE;

	len = KRZ_LEN(col);
	if (overwrite)
		while ((--len)>=0) char_left(t_ptr);
	else
		while ((--len)>=0) char_bs(t_ptr);
	str = ERSATZ(col);
	set_pos = FALSE;
	for (i=col->len-(KRZ_MAX_LEN+2); (--i)>=0; )
	{
		if (*str == '^' && i > 0 && str[1] == '^')
		{
			hl_update( t_ptr );
			char_cr(t_ptr);
			restore_edit();						/* sonst entstehen komische Effekte */
			xw++;
			str++;
			i--;
		}
		else if (*str!='~' || set_pos)		/* das erste '~'-Zeichen z�hlt */
		{
			char_insert(t_ptr, *str);
			xw++;
		}
		else
		{
			set_pos = TRUE;
			xw = 0;
		}
		str++;
	}
	if (set_pos)
		while ((--xw)>=0) char_left(t_ptr);
	hl_update( t_ptr );
	/*
	 * Und wieder herstellen
	*/
	t_ptr->loc_opt->einruecken = save_insert;
}

/* return 1 : kein Speicher mehr => abbruch */
/*        0 : alles ok                      */
short add_kurzel(RINGP rp, ZEILEP col)
{
	short		len, i;
	char		*str, buffer[MAX_LINE_LEN+1], *start;
	ZEILEP	c;
	bool		online;

	krz_loaded = TRUE;
	if (col->len<2)
		return(0);
	if (col->len > MAX_LINE_LEN - KRZ_MAX_LEN)
		return(0);
	if (!ist_mem_frei())
		return (1);
	start = strchr(TEXT(col),'=');					/* start zeigt auf Ersatztext */
	if (start==NULL)
		return(0);
	start++;
	/* WS am Anfang �berspringen */
	for (str=TEXT(col); *str==' ' || *str=='\t'; str++) ;
	if (str[0]=='#' || str[0]=='=' ||
	    (str[0]=='*' && str[1]=='='))
		return(0);									/* Kommentarzeile oder kein Kurzel */
	len = 0;
	while (len<KRZ_MAX_LEN && *str!=' ' && *str!='\t' && *str!='=')	/* Ende suchen */
	{
		len++; str++;
	}
	if (start[-2]=='*')									/* auto. K�rzel */
	{
		online = TRUE;
		if (*str=='=')
		{
			str--;
			len--;
		}
	}
	else
		online = FALSE;
	buffer[0] = len;										/* vorne L�nge */
	i = 1;
	while (len>0)											/* umdrehen */
	{
		buffer[i++] = *(--str);
		len--;
	}
	buffer[i] = EOS;										/* abschliessen */
	len = (short) strlen(start);
	memcpy(buffer + KRZ_MAX_LEN + 2, start, len);
	len += (KRZ_MAX_LEN+2);

	c = FIRST(&kurz);
	if (ist_leer(&kurz))
		INSERT(&c,0,len,buffer);
	else
	{
		while (!IS_TAIL(c) && strcmp(KRZ_TXT(c),buffer+1)>0)
			NEXT(c);
		col_insert(rp, c->vorg,new_col(buffer,len));
		kurz.lines++;
	}
	if (online)
	{
		c = FIRST(&auto_kurz);
		if (ist_leer(&auto_kurz))
			INSERT(&c,0,len,buffer);
		else
		{
			while (!IS_TAIL(c) && strcmp(KRZ_TXT(c),buffer+1)>0)
				NEXT(c);
			col_insert(rp, c->vorg,new_col(buffer,len));
			auto_kurz.lines++;
		}
	}
	return(0);
}

short load_kurzel(void)
{
	long		anz;
	RING		t;
	ZEILEP	lauf;
	short		erg;

	if (krz_name[0] == EOS)
		return 0;

	erg = 1;
	init_textring(&t);
	if (load_datei(krz_name, &t, FALSE, NULL) == 0)
	{
		anz = t.lines;
		lauf = FIRST(&t);
		if (anz)
		{
			clr_kurzel();							/* alte K�rzel l�schen */
			while ((--anz)>=0)
			{
				if (add_kurzel(&t, lauf)) 
					break;
				NEXT(lauf);
			}
		}
		erg = 0;
	}
	kill_textring(&t);
	return erg;
}

void	ch_kurzel(char *name, bool force_load)
{
	if (name[0] == EOS)
	{
		clr_kurzel();
		krz_name[0] = EOS;
		return;
	}
	if ((strcmp(name, krz_name) != 0) || (force_load))
	{
		strcpy(krz_name, name);
		load_kurzel();
	}
}
