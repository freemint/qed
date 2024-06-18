#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "global.h"
#include "aktion.h"
#include "ausgabe.h"
#include "comm.h"
#include "edit.h"
#include "find.h"
#include "icon.h"
#include "memory.h"
#include "olga.h"
#include "options.h"
#include "projekt.h"
#include "rsc.h"
#include "text.h"
#include "window.h"
#include "file.h"
#include "hl.h"
#include "av.h"


static PATH	last_path; 						/* letzter Pfad der Dateiauswahl */

/* Puffer-L�nge zum Lesen/Schreiben */
#define	BUFFERSIZE	4*1024L

void open_error(char *filename, short error)
{
	FILENAME	datei;
	PATH		path;

	split_filename(filename, path, datei);
	if (error == -ENOENT && path_exists(path))	/* file not found und Pfad existiert */
		snote(1, 0, NOTEXIST, datei);
	else
		snote(1, 0, READERR, datei);
}



short load(TEXTP t_ptr, bool verbose)
{
	short	antw;
	bool	null_byte;
	
	antw = load_datei(t_ptr->filename, &t_ptr->text, verbose, &null_byte);
	
	
	t_ptr->cursor_line = t_ptr->text.head.next;
	t_ptr->readonly = file_readonly(t_ptr->filename);
	if (null_byte)
		t_ptr->moved++;
	if (antw == 0)
	{
		t_ptr->file_date_time = file_time(t_ptr->filename,NULL,NULL);
		hl_init_text( t_ptr );
	}
	else
		t_ptr->file_date_time = -1L;
	return(antw);
}

short load_from_fd(short fd, char *name, RINGP t, bool verbose, bool *null_byte, long size)
{
	short		antw;
	bool		nb = FALSE, 
				ol = FALSE;
	char		*buffer, *zeile;
	LINEP 	start, next;
	long		l, p, bytes = 0L;
	short		n;
	bool		new_line, cr = FALSE, mem = TRUE;

	/* Puffer anfordern */
	buffer = (char *)malloc(BUFFERSIZE);
	zeile = malloc(MAX_LINE_LEN + 2);				/* + 2 f�r Zeilenende */

	if (buffer == NULL || zeile == NULL)
	{
		note(1, 0, NOMEMORY);
		return -ENOMEM;
	}				

	nb = FALSE;
	graf_mouse(HOURGLASS, NULL);

	/* Progress-Bar */
	verbose = verbose && ((size >> 10) >= transfer_size);
	if (verbose)
	{
		char		str[40];
		FILENAME	file;

		strcpy(str, rsc_string(LOADSTR));
		file_name(name, file, FALSE);
		strcat(str, file);
		start_aktion(str, FALSE, size);
	}


	/* Liste vorbereiten */
	start = t->tail.prev;

	/* Einlesen */
	l = Fread(fd, BUFFERSIZE, buffer);

	n = 0;
	new_line = FALSE;
	while (mem && (l > 0))
	{
		p = 0;
		while (mem && (p < l))
		{
			if (t->ending != lns_binmode)
			{
				if (buffer[p] == 0x0D)				/* CR -> Mac */
				{
					new_line = TRUE;
					p++;
					cr = TRUE;
					t->ending = lns_apple;
				}
				else if (buffer[p] == 0x0A)		/* LF */
				{
					p++;
					if (cr)								/* CRLF -> TOS*/
					{
						cr = FALSE;
						t->ending = lns_tos;
					}
					else
					{
						new_line = TRUE;				/* -> Unix */
						t->ending = lns_unix;
					}
				}
				else if (n >= t->max_line_len)	/* �berl�nge */
				{
					ol = TRUE;
					new_line = TRUE;
				}
				else 
				{
					/* Nullbyte? */
					if (buffer[p] == '\0')
					{
						nb = TRUE;
						buffer[p] = ' ';				/* Durch Leerzeichen ersetzen */
					}
					zeile[n] = buffer[p];
					n++;
					p++;
				}
			}
			else	/* lns_binmode */
			{
				if (n >= (t->max_line_len - 1))
					new_line = TRUE;
				zeile[n] = buffer[p];
				n++;
				p++;
			}
			
			if (new_line)
			{
				zeile[n] = EOS;
				start->next = new_col(zeile, n);
				if (start->next == NULL)
					mem = FALSE;
				else
				{
					start = start->next;
					if (ol)
					{
						start->info |= OVERLEN;
						ol = FALSE;
					}
				}
				n = 0;
				new_line = FALSE;
			}
		} /* while */

		if (verbose)
		{
			bytes += BUFFERSIZE;
			do_aktion("", bytes);
		}
		l = Fread(fd, BUFFERSIZE, buffer);

		/* EOF */
		if (l == 0)
		{
			/* letzte Zeile ohne Zeilenende! */
			if (n > 0)
			{
				zeile[n] = EOS;
				start->next = new_col(zeile, n);
				if (start->next == NULL)
					mem = FALSE;
				else
					start = start->next;
			}

			/* letzte Zeile hatte ZE -> ein Dummyzeile anh�ngen */
			else if (n == 0)
			{
				start->next = new_col(zeile, 0);
				if (start->next == NULL)
					mem = FALSE;
				else
					start = start->next;
			}
		}
	} /* while */

	if (mem)
	{
		/* Ring schlie�en */
		t->tail.prev = start;
		start->next = &t->tail;

		/* Anzahl der Zeilen ermitteln */
		start = t->head.next;
		next = start->next;
		for (l = 0; !IS_TAIL(start); l++)
		{
			next->prev = start;
			start = next;
			NEXT(next);
		}
		t->lines = l;
		if (l > 1L)
			col_delete(t, t->head.next);

		antw = 0;
	}
	else
	{
		/* Speichermangel! Bisher gelesene Zeilen freigeben. */
		next = FIRST(t);
		while (next)
		{
			start = next;
			NEXT(next);
			free_col(t,start);
		}
		graf_mouse(ARROW, NULL);
		note(1, 0, NOMEMORY);
		antw = -ENOMEM;
	}

	free(buffer);
	free(zeile);
	
	if (verbose)
		end_aktion();

	graf_mouse(ARROW, NULL);

	if (null_byte != NULL)
		*null_byte = nb;
	return(antw);
}

short load_datei(char *name, RINGP t, bool verbose, bool *null_byte)
{
	short	antw, fd;
	long	size;
	
	/* gr��e der Datei ermitteln */
	size = file_size(name);
	if (size >= 0)
	{
		fd = (short) Fopen(name, 0);
		if (fd > 0)
		{
			antw = load_from_fd(fd, name, t, verbose, null_byte, size);
			Fclose(fd);
		} else
		{
			antw = fd;
		}
	} else
	{
		antw = -1;
	}

	return antw;
}


/*
 * Ermittelt die anzahl der Bytes und Zeilen der �bergebenen Datei.
 * Wird bei der Projektverwaltung (Info) benutzt.
 */
short infoload(char *name, long *bytes, long *lines)
{
	RING	t;
	short	antw = 1;

	if (file_exists(name))
	{
		init_textring(&t);
		if (load_datei(name, &t, FALSE, NULL) == 0)
		{
			*bytes = textring_bytes(&t);
			*lines = t.lines;
			antw = 0;
		}
		kill_textring(&t);
	}
	else
	{
		char	s[28];
		
		*bytes = 0;
		*lines = 0;
		make_shortpath(name, s, 28);
		snote(1, 0, READERR, s);
	}
	return antw;
}


/****************************************************************************/

static void backup_name(char *name, char *ext)
{
	PATH		new;
	FILENAME	new_name;
	
	strcpy(new, name);
	strcat(new, ".");
	strcat(new, ext);
	file_name(new, new_name, FALSE);
	if (fs_long_name(new) >= strlen(new_name))
		strcpy(name, new);
	else
		set_extension(name, ext);
}

static void restore_back_up(TEXTP t_ptr)
{
	if (t_ptr->loc_opt->backup)
	{
		PATH old;

		if (file_exists(t_ptr->filename))
			Fdelete(t_ptr->filename);
		strcpy(old, t_ptr->filename);
		backup_name(old, t_ptr->loc_opt->backup_ext);
		(void)Frename(0, old, t_ptr->filename);
	}
}


static void back_up(TEXTP t_ptr)
{
	PATH new;

	if (t_ptr->loc_opt->backup)
	{
		graf_mouse(HOURGLASS, NULL);
		strcpy(new, t_ptr->filename);
		backup_name(new, t_ptr->loc_opt->backup_ext);
		if (file_exists(new))			/* Alte DUP-Datei l�schen */
			Fdelete(new);
		(void)Frename(0, t_ptr->filename, new);
		graf_mouse(ARROW, NULL);
	}
}

short save_to_fd(short fd, char *name, RINGP t, bool verbose)
{
	char	end_str[3], *zeile, *buffer, *ptr;
	short	e_len, z_len, antw;
	long	bytes = 0L;
	long	ret, b, rest, text_size;
	LINEP	line;

	/* Puffer anfordern */
	buffer = (char *)malloc(BUFFERSIZE);
	zeile = (char *)malloc(MAX_LINE_LEN + 2);

	if (buffer == NULL || zeile == NULL)
	{
		note(1, 0, NOMEMORY);
		return -ENOMEM;
	}				

	graf_mouse(HOURGLASS, NULL);

	/* Progress-Bar */
	text_size = textring_bytes(t);
	verbose = verbose && ((text_size >> 10) >= transfer_size);
	if (verbose)
	{
		char	str[40];
		FILENAME	file;

		strcpy(str, rsc_string(SAVESTR));
		file_name(name, file, FALSE);
		strcat(str,file);
		start_aktion(str, FALSE, text_size);
	}

	/* String mit Zeilenende erzeugen */
	switch (t->ending)
	{
		case lns_tos :
			strcpy(end_str, "\r\n");
			e_len = 2;
			break;
		case lns_unix :
			strcpy(end_str, "\n");
			e_len = 1;
			break;
		case lns_apple :
			strcpy(end_str, "\r");
			e_len = 1;
			break;
		default:
			strcpy(end_str, "");
			e_len = 0;
			break;
	}
	
	line = FIRST(t);
	if (line != NULL)
	{
		b = 0L;
		ret = 1;
		ptr = buffer;
		rest = BUFFERSIZE;
		while ((!IS_TAIL(line)) && (ret > 0))
		{
			/* Zeile aus Text und Zeilenende zusammen setzen */
			memcpy(zeile, TEXT(line), line->len);
			z_len = line->len;

			/*
			 * Das Zeilenende wird nur dann angeh�ngt, wenn lauf nicht letzte
			 * Zeile ist. Gab es beim Laden der Datei in der letzten Zeile
			 * ein ZE, gibt es die Dummyzeile. Gab es das ZE nicht, wird auch
			 * kein ZE angeh�ngt!
			 */
			if (!IS_LAST(line) && (e_len > 0) && !IS_OVERLEN(line))
			{
				memcpy(zeile + line->len, end_str, e_len);
				z_len += e_len;
			}
			
			/* Passt die Zeile noch in den Puffer? */
			if (z_len < rest)
			{
				/* komplett in den Puffer */
				memcpy(ptr, zeile, z_len);
				ptr += z_len;
				b += z_len;
				rest -= z_len;
			}
			else
			{
				/* nur soviel kopieren, wie noch passt */
				memcpy(ptr, zeile, rest);
				ptr += rest;
				
				/* wegschreiben */
				ret = Fwrite(fd, BUFFERSIZE, buffer);
				if (verbose)
				{
					bytes += BUFFERSIZE;
					do_aktion("", bytes);
				}

				if (ret != BUFFERSIZE)
					ret = -ENOSPC;

				/* und den Rest in den Puffer */
				b = z_len - rest;
				ptr = buffer;
				memcpy(ptr, zeile + rest, b);
				ptr += b;
				
				rest = BUFFERSIZE - b;
			}
			NEXT(line);
		}

		/* Befindet sich noch etwas im Puffer und ist kein Fehler aufgetreten? */
		if ((b > 0L) && (ret > 0))
		{
			ret = Fwrite(fd, b, buffer);
			if (verbose)
			{
				bytes += b;
				do_aktion("", bytes);
			}
			if (ret != b)
				ret = -ENOSPC;
		}
		if (ret < 0)
			antw = (short)ret;
		else
			antw = 0;
	}
	else
		antw = 1;

	free(buffer);
	free(zeile);

	if (verbose)
		end_aktion();

	graf_mouse(ARROW, NULL);

	return antw;
}

short save_datei(char *name, RINGP t, bool verbose)
{
	short	fd, antw;

	fd = (short) Fcreate(name, 0);
	if (fd > 0)
	{
		antw = save_to_fd(fd, name, t, verbose);
		Fclose(fd);
		if (antw == -ENOSPC)
			Fdelete(name);
	}
	else
		antw = fd;

	{
		char path[ MAX_PATH_LEN+1 ];
		split_filename( name, path, NULL );
		send_avpathupdate( path );
	}

	return antw;
}


short save(TEXTP t_ptr)
{
	short		antw;
	struct stat	st;
	bool		not_exists;

	if (file_exists(t_ptr->filename))
	{
		if (file_readonly(t_ptr->filename))
		{
			FILENAME	file;

			t_ptr->readonly = TRUE;
			file_name(t_ptr->filename, file, FALSE);
			snote(1, 0, READONLY, file);
			return -EACCES;
		}
		else
			t_ptr->readonly = FALSE;
		if (t_ptr->file_date_time != -1L)
		{
			long date_time = file_time(t_ptr->filename, NULL, NULL);

			if (date_time != t_ptr->file_date_time)
			{
				FILENAME	name;

				file_name(t_ptr->filename, name, FALSE);
				if (snote(1, 2, MOVED2, name) == 2)
					return -1;
			}
		}
	}
	
	not_exists = (stat(t_ptr->filename, &st) != 0);
	
	back_up(t_ptr);
	antw = save_datei(t_ptr->filename, &t_ptr->text, TRUE);
	if (antw != 0)
	{
		if (antw == -ENOSPC)
		{
			char	tmp[20];

			make_shortpath(t_ptr->filename, tmp, 19);
			snote(1, 0, NOSPACE, tmp);
		}
		else
			note(1, 0, WRITEERR);
		restore_back_up(t_ptr);
		t_ptr->file_date_time = -1L;
		return antw;
	}

	t_ptr->moved = 0;
	t_ptr->file_date_time = file_time(t_ptr->filename, NULL, NULL);
	t_ptr->asave = time(NULL);

	/* OLGA informieren */
	do_olga(OLGA_UPDATE, t_ptr->filename, NULL);

	/* Attribute wieder herstellen */
	if (!not_exists)
	{
		Fchmod(t_ptr->filename, (st.st_mode & 0x0000FFFF));
		if (Pgetuid() == 0)						/* nur root darf Owner �ndern */
			Fchown(t_ptr->filename, st.st_uid, st.st_gid);
	}

	return antw;
}


short save_as(TEXTP t_ptr, char *name)
{
	short	antw;

	if (file_exists(name))
	{
		if (file_readonly(name))
		{
			FILENAME	file;

			file_name(t_ptr->filename, file, FALSE);
			snote(1, 0, READONLY, file);
			return -EACCES;
		}
	}
	antw = save_datei(name, &t_ptr->text, TRUE);
	if (antw != 0)
	{
		if (antw == -ENOSPC)
		{
			char	tmp[20];

			make_shortpath(t_ptr->filename, tmp, 19);
			snote(1, 0, NOSPACE, tmp);
		}
		else
			note(1, 0, WRITEERR);
		restore_back_up(t_ptr);
	}
	return(antw);
}


bool	save_new(char *name, char *mask, char *title)
{
	PATH	new;
	bool	ok = FALSE;
	
	if ((name[0] == EOS) || (strcmp(name, rsc_string(NAMENLOS)) == 0))
		strcpy(new, last_path);
	else
		strcpy(new, name);
	if (select_single(new, mask, title))
	{
		if (strcmp(name, new) != 0)		/* wirklich neuer Name? */
		{
			if (file_exists(new) && (note(1, 2, EXIST) == 2))
				ok = FALSE;
			else
			{
				strcpy(name, new);
				ok = TRUE;
			}
		}
		else
			ok = FALSE;
	}
	return ok;
}


/*****************************************************************************/
/* Dateiauswahl																				  */
/*****************************************************************************/
static bool	multi_bin = FALSE;

void store_path(char *path)
{
	PATH	p;

	split_filename(path, p, NULL);
	if (strcmp(last_path, p) != 0)		/* neuer Pfad? */
	{
		if (path_exists(p))
			strcpy(last_path, p);
	}
}

bool select_single(char *filename, char *mask, char *title)
{
	PATH		path = "";
	FILENAME	name = "";
	bool		ok;
	
	if (filename[0] != EOS)
		split_filename(filename, path, name);
	else
		strcpy(path, last_path);

	ok = select_file(path, name, mask, title, FSCB_NULL);
	if (ok && name[0] != EOS)			/* f�r den Fall, da� nur ein Verzeichnis kommt */
	{
		strcpy(filename, path);
		strcat(filename, name);
		store_path(path);
	}
	return ok;
}


bool select_path(char *pathname, char *title)
{
	PATH		path = "";
	FILENAME	name = "";
	bool		ok;
	
	if (pathname[0] != EOS)
		split_filename(pathname, path, NULL);
	else
		strcpy(path, last_path);

	ok = select_file(path, name, "", title, FSCB_NULL);
	if (ok)							/* f�r den Fall, da� nur ein Verzeichnis kommt */
	{
		strcpy(pathname, path);
		store_path(path);
	}
	return ok;
}

static int open_multi(char *path, char *name)
{
	if (name[0] != EOS)			/* f�r den Fall, da� nur ein Verzeichnis kommt */
	{
		PATH	filename;
		short	r;
		
		strcpy(filename, path);
		strcat(filename, name);
		if (!shift_pressed() && filematch(filename, "*.QPJ", -1))
			r = load_projekt(filename);
		else
			r = load_edit(filename, multi_bin);
		if (r > 0)
			send_dhst(filename);
	}
	return TRUE;
}

void select_multi(bool binary)
{
	PATH		path;
	FILENAME	name = "";
	char		*title;

	multi_bin = binary;
	strcpy(path, last_path);

	if (binary)
		title = rsc_string(LOADBINSTR);
	else
		title = rsc_string(LOADFILESTR);
	select_file(path, name, "", title, open_multi);
}

/******************************************************************************/
void init_file(void)
{
	get_path(last_path, 0);							/* Aktuellen Pfad holen */
}

void term_file(void)
{
}
