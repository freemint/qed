#include <ctype.h>
#include <support.h>

#include "global.h"
#include "edit.h"
#include "icon.h"
#include "memory.h"
#include "rsc.h"
#include "set.h"
#include "text.h"
#include "window.h"
#include "error.h"

extern void	menu_help(short title, short item);

/*
 * Exportierte Variablen:
*/
char	error[FEHLERANZ][40];
TEXTP	last_errtext = NULL;

/*
 * lokales
*/
typedef enum {nil, read_text, read_name, read_zeile, read_spalte, read_fehler} TOKEN;

#define MAX_TOKEN		10				/* Anzahl der Token */
#define MAX_ERRLEN	120			/* Lnge der Fehlerzeile */

typedef struct
{
	TOKEN	token;
	char	text[30];
} TOKENELEM, *TEP;

static TOKENELEM	token_list[MAX_TOKEN];
static short			token_anzahl;
static PATH			error_name;					/* Dateiname des Errorfiles */

/* das Ergebnis */
static PATH	dateiname;
static char	fehlertext[MAX_ERRLEN];
static long	fehlerzeile;
static short	fehlerspalte;
static short	err_anz = 0;



static void init_parser(char *mustertxt)
{
	char	tmp[2] = " ";
	short	i;
	TOKEN last_token;

	strcpy(dateiname, "");
	fehlerzeile = -1;
	fehlerspalte = -1;
	strcpy(fehlertext, "");

	token_anzahl = 0;
	last_token = nil;
	for (i = 0; i < MAX_TOKEN; i++)
	{
		token_list[i].token = nil;
		strcpy(token_list[i].text, "");
	}

	for (i = 0; i < (short)strlen(mustertxt); i++)
	{
		switch (mustertxt[i])
		{
			case '%' :
				i++;
				if (last_token == read_text)
					token_anzahl++;
				switch (mustertxt[i])
				{
					case 'f' :
						token_list[token_anzahl].token = read_name;
						last_token = read_name;
						break;
					case 'z' :
						token_list[token_anzahl].token = read_zeile;
						last_token = read_zeile;
						break;
					case 's' :
						token_list[token_anzahl].token = read_spalte;
						last_token = read_spalte;
						break;
					case 't' :
						token_list[token_anzahl].token = read_fehler;
						last_token = read_fehler;
						break;
				}
				break;
			default:
				if (last_token > read_text)
					token_anzahl++;
				token_list[token_anzahl].token = read_text;
				tmp[0] = mustertxt[i];
				strcat(token_list[token_anzahl].text, tmp);
				last_token = read_text;
				break;
		}
	}
}


static bool readin_text(char *zeile, short *position, char *text)
{
	short	len = (short)strlen(text),
			i;
	char	tmp[10];

	i = *position;
	while ( (i < (short)strlen(zeile)) && (i < (len + *position)))
	{
		tmp[i - *position] = zeile[i];
		i++;
	}
	if (i > *position)
	{
		tmp[i - *position] = EOS;
		*position = i;
		return (strcmp(tmp, text) == 0);
	}
	else
		return FALSE;
}

static bool	readin_name(char *zeile, short *position)
{
	SET	valid_char;
	PATH	tmp;
	short	i;

	strcpy(tmp,"-+._~\\/A-Za-z0-9");				/* Zulssige Zeichen fr Dateinamen */
	str2set(tmp, valid_char);
	i = *position;
	while ( 	(i < (short)strlen(zeile)) && 	/* Sonderbehandlung fr ':', nur im Pfad erlaubt! */
				((setin(valid_char, zeile[i])) ||
				 (zeile[i] == ':' && ((zeile[i+1] == '\\') || zeile[i+1] == '/'))))
	{
		tmp[i - *position] = zeile[i];
		i++;
	}
	if (i > *position)
	{
		tmp[i - *position] = EOS;
		*position = i;

#ifdef __MINT__
		if (strchr(tmp, '/') != NULL)				/* UNIX-Pfad -> nach TOS wandeln */
		{
			unx2dos(tmp, dateiname);
		}
		else
#endif
		if (tmp[1] != ':')						/* Kein Laufwerk -> Name ohne Pfad! */
		{

			split_filename(error_name, dateiname, NULL);
			strcat(dateiname, tmp);
		}
		else
			strcpy(dateiname, tmp);
		return (file_exists(dateiname));
	}
	else
		return FALSE;
}

static bool	readin_zeile(char *zeile, short *position)
{
	short	i;
	char	tmp[10];

	i= *position;
	while ( (i < (short)strlen(zeile)) && (isdigit(zeile[i])) )
	{
		tmp[i - *position] = zeile[i];
		i++;
	}
	if (i > *position)
	{
		tmp[i - *position] = EOS;
		fehlerzeile = atol(tmp);
		*position = i;
		return TRUE;
	}
	else
		return FALSE;
}

static bool	readin_spalte(char *zeile, short *position)
{
	short	i;
	char	tmp[10];

	i= *position;
	while ( (i < (short)strlen(zeile)) && (isdigit(zeile[i])) )
	{
		tmp[i - *position] = zeile[i];
		i++;
	}
	if (i > *position)
	{
		tmp[i - *position] = EOS;
		fehlerspalte = atoi(tmp);
		*position = i;
		return TRUE;
	}
	else
		return FALSE;
}

static bool	readin_fehler(char *zeile, short *position)
{
	short	i, j;
	char	tmp[MAX_ERRLEN];

	i = *position;
	j = 0;
	while ( (i < (short)strlen(zeile)) && (j < sizeof(tmp)) )
	{
		tmp[i - *position] = zeile[i];
		i++;
		j++;
	}
	if (i > *position)
	{
		tmp[i - *position] = EOS;
		*position = i;
		strcpy(fehlertext, tmp);
		return TRUE;
	}
	else
		return FALSE;
}

static bool	parse_line(char *zeile)
{
	short	i, z_pos = 0;
	bool	ok = FALSE;

	for (i = 0; i <= token_anzahl; i++)
	{
		switch (token_list[i].token)
		{
			case read_text :
				ok = readin_text(zeile, &z_pos, token_list[i].text);
				break;
			case read_name :
				ok = readin_name(zeile, &z_pos);
				break;
			case read_zeile :
				ok = readin_zeile(zeile, &z_pos);
				break;
			case read_spalte :
				ok = readin_spalte(zeile, &z_pos);
				break;
			case read_fehler :
				ok = readin_fehler(zeile, &z_pos);
				break;
			default:;
		}
		if (!ok)
			break;
	}
	return ok;
}


void	handle_error(TEXTP t_ptr)
{
	short	icon, i;
	char	str[WINSTRLEN+1];

	if (last_errtext != NULL && t_ptr != last_errtext)
	{
		LINEP line = t_ptr->cursor_line;
	
		t_ptr = last_errtext;
		line = t_ptr->cursor_line;
		/* nchste Zeile setzen */		
		if (!IS_LAST(t_ptr->cursor_line))
		{
			NEXT(line);
			t_ptr->cursor_line = line;
			t_ptr->xpos = 0;
			t_ptr->ypos++;
			make_chg(t_ptr->link, POS_CHANGE, 0);
		}
	}
	for (i = 0; i < err_anz; i++)
	{
		init_parser(error[i]);
		strcpy(error_name, t_ptr->filename);
		if (parse_line(TEXT(t_ptr->cursor_line)))
		{
			last_errtext = t_ptr;

			icon = load_edit(dateiname, FALSE);			/* Laden als Text und ffnen */
			if (icon > 0)
			{
				if (fehlerspalte > 0)
					desire_x = fehlerspalte - 1;				/* wir fange bei 0 an! */
				else
					desire_x = 0;
				if (fehlerzeile > 0)
					desire_y = fehlerzeile - 1;
				else
					desire_y = 0;

				if (strlen(fehlertext) > 0)
				{
					strcpy(str, rsc_string(ERRORSTR));
					strcat(str, fehlertext);
					set_info(get_text(icon), str);
				}
				icon_edit(icon, DO_GOTO);
				return;
			}
		}
	}
	if (last_errtext != NULL)
		last_errtext = NULL;
	Bconout(2, 7);
}

/*
 * Die Dialogbox fr die Fehlerzeile
 */
void	fehler_box(void)
{
	short	i, antw = 0;
	char	str[40];
	MDIAL	*dial;
	bool	close = FALSE;
	
	for (i = 0; i < FEHLERANZ; i++)
		set_string(fehler, FEHLTEXT1 + i, error[i]);
	
	dial = open_mdial(fehler, 0);
	if (dial != NULL)
	{
		while (!close)
		{
			antw = do_mdial(dial) & 0x7fff;
			switch (antw)
			{
				case FEHLHELP :
					menu_help(TSPEZIAL, MFEHLER);
					set_state(fehler, antw, OS_SELECTED, FALSE);
					redraw_mdobj(dial, antw);
					break;
				default:
					close = TRUE;
					break;
			}
		}
		close_mdial(dial);
		set_state(fehler, antw, OS_SELECTED, FALSE);
		if (antw == FEHLOK)
		{
			err_anz = 0;
			for (i = 0; i < FEHLERANZ; i++)
			{
				strcpy(error[i], "");								/* leeren */
				get_string(fehler, FEHLTEXT1 + i, str);
				set_errorline(str);
			}
		}
	}
}

/*
 * Zeilen aus Parameterdatei eintragen.
*/
void set_errorline(char *zeile)
{
	if ((err_anz < FEHLERANZ) && (zeile[0] != EOS))
	{
		strcpy(error[err_anz], zeile);
		err_anz++;
	}
}
