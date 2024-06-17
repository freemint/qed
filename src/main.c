#include <ctype.h>
#include <signal.h>
#include <support.h>

#if defined(__PUREC__)
#undef SIGHUP
#undef SIGINT
#undef SIGSYS
#undef SIGQUIT
#undef SIGPIPE
#undef SIGTERM
#define SIGHUP  1
#define SIGINT  2
#define SIGQUIT 3
#define SIGSYS  12
#define SIGPIPE 13
#define SIGTERM 15
#undef SIG_DFL
#undef SIG_IGN
#undef SIG_ERR
#undef SIG_SYS
#define SIG_DFL	((__mint_sighandler_t) 0L)
#define SIG_IGN	((__mint_sighandler_t) 1L)
#define SIG_ERR	((__mint_sighandler_t)-1L)
#define SIG_SYS	((__mint_sighandler_t)-2L)
#endif

#include "global.h"
#include "clipbrd.h"
#include "comm.h"
#include "edit.h"
#include "error.h"
#include "event.h"
#include "file.h"
#include "find.h"
#include "icon.h"
#include "kurzel.h"
#include "makro.h"
#include "memory.h"
#include "menu.h"
#include "options.h"
#include "poslist.h"
#include "printer.h"
#include "projekt.h"
#include "rsc.h"
#include "set.h"
#include "text.h"
#include "umbruch.h"
#include "version.h"
#include "window.h"
#include "hl.h"


static void __CDECL handle_term(long sig)
{
	(void) sig;
	abort_prog = TRUE;
}

static bool init_all(int argc, char *argv[])
{
	POSENTRY	*arglist = NULL;
	short gi_wind, gi_msgs, d;

	init_global();
	if (debug_level & DBG_INIT)
		debug("init_global done.\n");

	appl_xgetinfo(11, &gi_wind, &d, &d, &d);
	appl_xgetinfo(12, &gi_msgs, &d, &d, &d);

	wcmode = 0;	
	if (gi_wind & 0x0800)	/* WF_OPTS */
	{
		short wopts, new_wopts = 0;

		appl_xgetinfo(97, &wopts, &d, &d, &d);
		
		if (gi_msgs & 0x0400) /* WM_REPOSED */
			new_wopts |= WO0_SENDREPOS; /* wind_set(-1, WF_OPTS, 1, WO0_SENDREPOS, 0, 0); */
		if (wopts & 0x0020) /* WCOWORK */
		{
			new_wopts |= 0x0020;  /* WO0_WCOWORK */
			wcmode = 1;
		}
	
		wind_set(-1, WF_OPTS, 1, new_wopts, 0, 0);
#if 0	
		{
			char db[256];
			
			sprintf(db, "new_wopts %x\r\n", new_wopts);
			Cconws(db);
		}
#endif
	}
#ifdef ONLY_XAAES
	if (!wcmode)
	{
		do_alert(1, 0, "[3][This version of Qed needs WCOWORK,|only provided by XaAES atm!][Exit]");
		return FALSE;
	}
#endif	
	if (gi_msgs & 0x0008) /* AP_TERM */
		shel_write(9, 1, 1, 0L, 0L);

	if (!init_resource())
		return FALSE;

	if (debug_level & DBG_INIT)
		debug("init_resource done.\n");
		
	init_file();
	if (debug_level & DBG_INIT)
		debug("init_file done.\n");

	init_window();
	if (debug_level & DBG_INIT)
		debug("init_window done.\n");

	init_icon();
	if (debug_level & DBG_INIT)
		debug("init_icon done.\n");

	init_memory();
	if (debug_level & DBG_INIT)
		debug("init_memory done.\n");

	init_edit();
	if (debug_level & DBG_INIT)
		debug("init_edit done.\n");

	init_makro ();
	if (debug_level & DBG_INIT)
		debug("init_makro done.\n");

	init_marken();
	if (debug_level & DBG_INIT)
		debug("init_makren done.\n");

	init_printer ();
	if (debug_level & DBG_INIT)
		debug("init_printer done.\n");

	init_projekt ();
	if (debug_level & DBG_INIT)
		debug("init_projekt done.\n");

	init_default_var();
	if (debug_level & DBG_INIT)
		debug("init_default_var done.\n");

	option_load(&arglist);
	if (debug_level & DBG_INIT)
		debug("option_load done.\n");

	hl_init();
	hl_read_syn();
	if (debug_level & DBG_INIT)
		debug("hl_read_syn done.\n");

	font_change();
	color_change();

	init_clipbrd();
	if (debug_level & DBG_INIT)
		debug("init_clipbrd done.\n");

	init_comm();
	if (debug_level & DBG_INIT)
		debug("init_com done.\n");
	
	if (argc > 1)
	{
		short		i, x, l;
		long		y;
		PATH		str;
		bool		in_quote = FALSE;

#ifndef PREDEF_DEBUG_LEVEL
		if (debug_level)
			i = 2;			/* --debug Åberspringen */
		else
#endif
			i = 1;
		while (i < argc)
		{
			/*
			 * MagXDesk liefert Namen mit Blanks nicht Åber ARGV sondern
			 * die Kommandozeile.
			*/
			if (in_quote)
			{
				strcat(str, " ");
				strcat(str, argv[i]);
				l = (short)strlen(str);
				if (str[l-1] == '\'')			/* letztens Namesteil gefunden */
				{
					str[l-1] = EOS;				/* ' am Ende lîschen */
					in_quote = FALSE;
				}
			}
			else if (argv[i][0] == '\'')
			{
				strcpy(str, argv[i]+1);
				in_quote = TRUE;
			}
			else
				strcpy(str, argv[i]);
			i++;
			if (!in_quote)
			{
				x = 0;
				y = 0;
				if ((i < argc) && isdigit(argv[i][0]))		/* next: Ziffer? -> Zeile */
				{
					y = max(0, atol(argv[i]) - 1);
					i++;
					if ((i < argc) && isdigit(argv[i][0]))	/* next: Ziffer? -> Spalte */
					{
						x = max(0, atoi(argv[i]) - 1);
						i++;
					}
				}

				if (debug_level & DBG_ARG)
					debug("arglist= %s, %d, %ld\n", str, x, y);
				insert_poslist(&arglist, str, x, y);
			}
		}
	}

	menu_icheck(menu, MOEINF, !overwrite);
	menu_icheck(menu, MOOVERW, overwrite);

	open_poslist(arglist);
	delete_poslist(&arglist);
	if (debug_level & DBG_ARG)
		debug("arglist done.\n");
	
	return TRUE;
}

int main(int argc, char *argv[])
{
	short	i, d;
	char	menu_str[20];

#ifdef PREDEF_DEBUG_LEVEL
	debug_level = PREDEF_DEBUG_LEVEL;
#endif

	(void) Pdomain(1);
	if ((argc > 1) && (strncmp(argv[1], "--debug", 7) == 0))
	{
		debug_level = DBG_GEN;
		if (isdigit(argv[1][7]))						/* folgt Level ? */
			debug_level |= atoi(argv[1]+7);
	}
#ifndef PREDEF_DEBUG_LEVEL
	else
		debug_level = 0;
#endif

	if (debug_level)
	{
/* added by Heiko */
#ifdef DEBUG_LOGFILE
		debug_init("qed",Datei,DEBUG_LOGFILE );
#else
		if (gl_mint)		/* gl_mint wird erst vin init_app() gesetzt! */
			debug_init("qed", Con, NULL);
		else
			debug_init("qed", TCon, NULL);
#endif

		debug( "*************************************************\n" );
	}

	nkc_init();
	init_app(NULL);
	init_colorpop( 8 );

	debug("Debug-Level: %d\n", debug_level);

	if (debug_level & DBG_ARG)
	{
		debug("argc=  %d\n", argc);
		for (i = 0; i < argc; i++)
			debug("argv[%d]= >%s<\n", i, argv[i]);
	}

	graf_mouse(HOURGLASS, NULL);
	
	return_code = 0;

	if (gl_gem >= 0x400)
	{
		strcpy(menu_str, "  qed ");
		strcat(menu_str, QED_VERSION);
		if (debug_level)
			strcat(menu_str, "(debug)");
		menu_register(gl_apid, menu_str);
	}

	Psignal(SIGINT, SIG_IGN);
	Psignal(SIGSYS, SIG_IGN);
	Psignal(SIGTERM, handle_term);
	Psignal(SIGQUIT, handle_term);
	Psignal(SIGHUP, handle_term);

	if (debug_level & DBG_OS)
	{
		debug("gem=      0x%X\n", gl_gem);
		debug("mint=     0x%X\n", gl_mint);
		debug("magx=     0x%X\n", gl_magx);
		debug("naes=     0x%X\n", gl_naes);
		debug("gl_apid=  %d\n", gl_apid);
		debug("app_path= %s\n", gl_appdir);
	}

	if (init_all(argc, argv))
	{
		graf_mouse(ARROW, NULL);

		if (debug_level & DBG_INIT)
			debug("enter event loop...\n");

		main_loop();
		if (debug_level & DBG_INIT)
			debug("event loop leaved.\n");

  	exit_colorpop();

		term_printer();
		if (debug_level & DBG_INIT)
			debug("term_printer done.\n");
		
		term_comm();
		if (debug_level & DBG_INIT)
			debug("term_comm done.\n");
		term_file();
		if (debug_level & DBG_INIT)
			debug("term_file done.\n");
		term_window();
		if (debug_level & DBG_INIT)
			debug("term_window done.\n");
	}
	term_resource();
	if (debug_level & DBG_INIT)
		debug("term_resource done.\n");
	term_global();
	if (debug_level & DBG_INIT)
		debug("term_global done.\n");

	exit_app(return_code);
	return return_code;
}
