#include <stdarg.h>
#include <sys/stat.h>
#include <support.h>
#include <time.h>
#include <unistd.h>
#include <gemx.h>

#include "global.h"
#include "hl.h"
#include "makro.h"
#include "options.h"
#include "rsc.h"
#include "set.h"
#include "window.h"


/*
 * exportierte Variablen
 */
short		wcmode;				/* Window Coordinate Mode */

short		fill_color;			/* aktuell eingestellte Fllfarbe */

bool		quick_close;		/* Sichern der Texte ohne Nachfrage */
short		vdi_handle;			/* Virtuelles Workstation Handle */

bool		done;					/* Ende gew„hlt ? */

short		desire_x, return_code;
long		desire_y, undo_y;

_WORD		font_id, font_pts,
			font_wcell, font_hcell,
			min_ascii, max_ascii;

bool		font_prop, font_vector;
short font_left_italicoffset, font_right_italicoffset;

short		debug_level;

/****** lokale Variablen *****************************************************/

static bool 	msleep = FALSE;

static GRECT 	clip; 			/* Letztes Clipping Rechteck */
static bool 	clip_flag;

static long		_idt;				/* Cookie fr Datum und Zeit */

/*****************************************************************************/
/* Maus-Routinen																														 */
/*****************************************************************************/
bool mouse_sleeps(void)
{
	return msleep;
}

void sleep_mouse(void)
{
	if (!msleep)
	{
		msleep = TRUE;
		hide_mouse();
	}
}

void wake_mouse(void)
{
	if (msleep)
	{
		msleep = FALSE;
		show_mouse();
	}
}

/*****************************************************************************/
/* Ausgabe recht oben im Men */
void print_headline(char *str)
{
	GRECT	c;
		
	if (gl_desk.g_y != 1)		/* gl_desk.g_y = 1: Men unsichtbar unter N.AES */
	{
		if (str[0] == EOS)		/* l”schen */
			menu_bar(menu, 1);
		else
		{
			_WORD 	d;
			_WORD	pxy[8];
			
			get_clip(&c);
			set_clip(FALSE, NULL);
			vst_font(vdi_handle, sys_big_id);
			vst_height(vdi_handle, sys_big_height, &d, &d, &d, &d);
			vqt_extent(vdi_handle, str, pxy);
			vswr_mode(vdi_handle, MD_TRANS);
			v_gtext(vdi_handle, gl_desk.g_x + gl_desk.g_w - (pxy[2] - pxy[0]) - 20, 0, str);
			vswr_mode(vdi_handle, MD_REPLACE);
			vst_font(vdi_handle, font_id);
			vst_point(vdi_handle, font_pts, &d, &d, &d, &d);
			set_clip(TRUE, &c);
		}
	}
}

/*****************************************************************************/
bool shift_pressed(void)
{
	_WORD	d, kstate;

	if (makro_play)
		kstate = makro_shift;
	else
		graf_mkstate(&d, &d, &d, &kstate);
	return ((kstate & (K_LSHIFT|K_RSHIFT)) != 0);
}

/*****************************************************************************/
bool inside (short x, short y, GRECT *r)
{
	return (x >= r->g_x && y >= r->g_y && x < r->g_x + r->g_w && y < r->g_y + r->g_h);
}

/*****************************************************************************/
bool get_clip (GRECT *size)
{
	*size = clip;
	return clip_flag;
}

void set_clip (bool clipflag, GRECT *size)
{
	_WORD	xy[4];

	if (!clip_flag && !clipflag) 
		return;										/* Es ist aus und bleibt aus */
	clip_flag = clipflag;
	if (clipflag)
	{
		if (size == NULL)
			clip = gl_desk;						/* Nichts definiert, nimm Desktop */
		else
			clip = *size;							/* Benutze definierte Gr”že */
		xy[0] = clip.g_x;
		xy[1] = clip.g_y;
		xy[2] = xy[0] + clip.g_w - 1;
		xy[3] = xy[1] + clip.g_h - 1;
	}
	else
		clip = gl_desk;
	vs_clip (vdi_handle, clipflag, xy);
}


/*****************************************************************************/
/* some day this nonsense with x different alert routines will
   have to be replaced by a variable argument list */
static short do_note(short def, short undo, char *s)
{
	wake_mouse();
	return do_walert(def, undo, s, " qed ");
}

short note(short def, short undo, short idx)
{
	return do_note(def, undo, (char *)alertmsg[idx]);
}

short inote(short def, short undo, short idx, short val)
{
	char	buf[128];
	
	sprintf(buf, (char *)alertmsg[idx], val);
	return do_note(def, undo, buf);
}

short snote(short def, short undo, short idx, char *val)
{
	char	buf[128];
	
	sprintf(buf, (char *)alertmsg[idx], val);
	return do_note(def, undo, buf);
}

short sinote(short def, short undo, short idx, char *sval, short val )
	{
	char	buf[128];

	sprintf(buf, (char *)alertmsg[idx], sval, val);
	return do_note(def, undo, buf);
}

/***************************************************************************/
/* Verschiedenes																				*/
/***************************************************************************/

/* Konfigurationsdatei; returns TRUE wenn gefunden,
 * filename enth„lt in diesem Fall den kompletten Pfad
 */
bool get_config_file( PATH filename, bool isdir )
{
	bool	found = FALSE;
	PATH	env, p_for_save = "";
	PATH	cfg_path;
/*	if (!gl_debug) */
	if (path_from_env("HOME", env))	/* 1. $HOME */
	{
	
		strcpy(cfg_path, env); /* 1a. $HOME/.qed */
		strcat(cfg_path, ".qed\\");
		if (path_exists(cfg_path))
	{
		strcat(cfg_path, filename);
			if (p_for_save[0] == EOS)
		strcpy(p_for_save, cfg_path);
			if( isdir )
				found = path_exists(cfg_path);
			else
		found = file_exists(cfg_path);
	}

		if (!found)										/* 1b. $HOME */
	{
		strcpy(cfg_path, env);
			if (path_exists(cfg_path))
			{
		strcat(cfg_path, filename);
		if (p_for_save[0] == EOS)
			strcpy(p_for_save, cfg_path);
				if( isdir )
					found = path_exists(cfg_path);
				else
		found = file_exists(cfg_path);
			}
		}

		if (!found)										/* 1c. $HOME/defaults */
		{
			strcpy(cfg_path, env);
			strcat(cfg_path, "defaults\\");
			if (path_exists(cfg_path))
			{
				strcat(cfg_path, filename);
				if (p_for_save[0] == EOS)
					strcpy(p_for_save, cfg_path);
				if( isdir )
					found = path_exists(cfg_path);
				else
				found = file_exists(cfg_path);
			}
		}		
		
	}

	if (!found && path_from_env("QED", cfg_path))			/* 2. $QED */
	{
		strcat(cfg_path, filename);
		if (p_for_save[0] == EOS)
			strcpy(p_for_save, cfg_path);
		if( isdir )
			found = path_exists(cfg_path);
		else
			found = file_exists(cfg_path);
	}

	if (!found && gl_appdir[0] != EOS)			/* 3. Startverzeichnis */
	{
		strcpy(cfg_path, gl_appdir);
		strcat(cfg_path, filename);
		if (p_for_save[0] == EOS)
			strcpy(p_for_save, cfg_path);
		if( isdir )
			found = path_exists(cfg_path);
		else
		found = file_exists(cfg_path);
	}

	if (!found && (isdir ? path_exists(filename) : file_exists(filename)))			/* 4. aktuelles Verzeichnis */
	{
		get_path(cfg_path, 0);
		strcat(cfg_path, filename);
		if (p_for_save[0] == EOS)
			strcpy(p_for_save, cfg_path);
		found = TRUE;
	}

	if (!found)
		strcpy(cfg_path, p_for_save);

	strcpy(filename, cfg_path);
	
	debug("cfg_file: %s (%d)\n", cfg_path, found);
	
	return found;
}

bool get_config_dir ( PATH p )
{
	bool ret;
	PATH tmp;
	strcpy( tmp, CFGNAME );
	ret = get_config_file( tmp, FALSE );
	split_filename( tmp, p, NULL);
	return ret;
}


void file_name(char *fullname, char *filename, bool withoutExt)
{
	split_filename(fullname, NULL, filename);
	if (withoutExt)
		split_extension(filename, filename, NULL);
}

/*****************************************************************************/

static void make_date(struct tm *timev, char *date)
{
	if (date != NULL)
	{
		switch ((unsigned short)_idt & 0xF00)			/* Reihenfolge im Datum */
		{
			case 0x000:  /* MM/DD/YYYY */
				strftime(date, 11, "%m/%d/%Y", timev);
				break;
			case 0x100:  /* DD.MM.YYYY */
				strftime(date, 11, "%d.%m.%Y", timev);
				break;
			default:  /* YYYY-MM-DD */
				strftime(date, 11, "%Y-%m-%d", timev);
				break;
		}
	}
}
/*****************************************************************************/

void get_datum(char *date)
{
	time_t		ttime;
	struct tm	*timev;

	time(&ttime);
	timev = localtime(&ttime);
	make_date(timev, date);
}

/*****************************************************************************/

long file_time(char *filename, char *fdate, char *ftime)
{
	struct stat	s;

	if (stat(filename, &s) == 0)
	{
		struct tm	*timev;

		timev = localtime(&s.st_mtime);
		if (ftime != NULL)
			strftime(ftime, 9, "%H:%M:%S", timev);
		if (fdate != NULL)
			make_date(timev, fdate);
		return s.st_mtime;
	}
	else
	{
		if (ftime != NULL)
			strcpy(ftime, "??");
		if (fdate != NULL)
			strcpy(fdate, "??");
		return 0;
	}
}

/*****************************************************************************/
long file_size(char *filename)
{
	struct stat	s;
	
	if (stat(filename, &s) == 0)
	{
		if (!S_ISREG(s.st_mode))
		{
			return -1;
		}
		return s.st_size;
	}
	return 0;
}

/*****************************************************************************/
bool file_readonly (char *filename)
{
	struct stat	s;
	bool		ret = FALSE;
	
	if (stat(filename, &s) == 0)
	{
		short	uid, gid;

		uid = Pgetuid();
		if (uid == -32)
			uid = 0;
		gid = Pgetgid();
		if (gid == -32)
			gid = 0;
		if (((uid == s.st_uid) && ((s.st_mode & S_IWUSR) != 0)) ||	
				/* Besitzer hat Schreibrecht */
		   
		  	 ((gid == s.st_gid) && ((s.st_mode & S_IWGRP) != 0)) ||	
		  	 	/* Gruppe hat Schreibrecht */
		    
		    (((s.st_mode & S_IWOTH) != 0))	||								
		    	/* Welt hat Schreibrecht */
		    
		    ((uid == 0) && ((s.st_mode & S_IWUSR) != 0)))				
		    	/* root darf, wenn Owner darf */
			ret = FALSE;
		else
			ret = TRUE;
	}
	return ret;
}


/***************************************************************************/
bool path_from_env(char *env, char *path)
{
	char	*p;
	bool	ret = FALSE;
		
	p = getenv(env);
	if (p != NULL)
	{
		strcpy(path, p);
		ret = make_normalpath(path);
	}
	return ret;
}

/***************************************************************************/
bool is_bin_name(char *filename)
{
	char	*p;
	short	i;
	
	p = strrchr(filename, '.');
	if (p != NULL)
	{
		for (i = 0; i < BIN_ANZ; i++)
		{
			if (stricmp(p+1, bin_extension[i]) == 0)
				return TRUE;
		}
	}
	return FALSE;
}

/***************************************************************************/

/* Ein ziemlicher Hack... Abfragen, ob ein Font ein Vektorfont ist */
static bool font_is_vector(short idx)
{
	short workout[57];
	short fs_handle = open_vwork(workout);
	short f_anz = workout[10];
	char fontname[34];
	bool ret = FALSE;
	_WORD di, fonttype;
	
	if (gl_gdos)
	{
		f_anz += vst_load_fonts(fs_handle, 0);
		while (f_anz)
		{
			if (vqt_ext_name(fs_handle, f_anz, fontname, &fonttype, &di ) == idx)
			{
				ret = (fonttype & 1) ? FALSE : TRUE;
				break;
			} else if (vqt_name(fs_handle, f_anz, fontname) == idx)
			{
				ret = fontname[ 32 ] ? TRUE : FALSE;
				break;
			}
			f_anz--;
		}
		vst_unload_fonts(fs_handle, 0);
	}
	v_clsvwk(fs_handle);
	return ret;
}

void font_change(void)
{
	_WORD ret, w1, w2, d, d1[5], effects[3];

	vst_effects(vdi_handle, 0);
	/* *_cell werden NUR hier ver„ndert */
	vst_font(vdi_handle, font_id);
	font_pts = vst_point(vdi_handle, font_pts, &d, &d, 
										&font_wcell, &font_hcell);

	vqt_width(vdi_handle, 'W', &w1, &ret, &ret);
	vqt_width(vdi_handle, 'i', &w2, &ret, &ret);
	font_prop = (w1 != w2);

	vst_effects(vdi_handle, HL_ITALIC);
	vqt_fontinfo(vdi_handle, &min_ascii, &max_ascii, d1, &d, effects);
	vst_effects(vdi_handle, TXT_NORMAL);
	if (min_ascii <= 0)
		min_ascii = 1;

	font_vector = font_is_vector(font_id);

	if ( font_vector )
	{	
		_WORD extent[8];
		vqt_extent(vdi_handle, "a", extent); /* no matter if prop font - font_wcell isn't used at all in that case */
		font_wcell = extent[2] - extent[0];
		font_hcell = extent[5] - extent[1];
	}

	font_right_italicoffset = effects[ 2 ];
	font_left_italicoffset = effects[ 1 ];

	/* Alle Fenster updaten */
	do_all_window(CLASS_ALL, do_font_change);
}

void select_font(void)
{
	short	n_id, n_pts;
	bool	ok = FALSE;

	n_id = font_id;
	n_pts = font_pts;
	ok = do_fontsel(FS_M_ALL, rsc_string(SELWFONTSTR), &n_id, &n_pts);
	if (ok)
	{
		font_id = n_id;
		font_pts = n_pts;
		font_change();
	}
	else if (n_id == -1 && n_pts == -1)
		note(1, 0, NOFSL);
}

/***************************************************************************/
/* Initialisieren des Moduls																*/
/***************************************************************************/
void init_global (void)
{
	short	work_out[57];
	_WORD	ret, f_anz;

	done = FALSE;
	clip_flag = TRUE;

	vdi_handle = open_vwork(work_out);
	f_anz = work_out[10];

	if (gl_gdos)
		f_anz += vst_load_fonts(vdi_handle, 0);

	vst_alignment(vdi_handle, TA_LEFT, TA_TOP, &ret, &ret);

	if (!getcookie("_IDT", &_idt))				/* Format fr Datum und Zeit */
		_idt = 0x0000112E;				/* DD.MM.YYYY HH:MM:SS */
}

/************************************************************************/
/* Terminieren des Moduls																*/
/************************************************************************/
void term_global(void)
{
	if (gl_gdos)
		vst_unload_fonts(vdi_handle, 0);
	v_clsvwk(vdi_handle);
}
