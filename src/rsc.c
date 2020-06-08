#include "global.h"
#include "rsc.h"
#include "version.h"
#include "se.h"

extern char	const __Ident_gnulib[];
extern char	const __Ident_gem[];
extern char	const __Ident_cflib[];

/*
 * exportierte Variablen
 */
OBJECT 	*menu;
OBJECT 	*about;
OBJECT 	*about2;
OBJECT 	*print;
OBJECT 	*textinfo;
OBJECT 	*prjinfo;
OBJECT	*blockinfo;
OBJECT	*sort;
OBJECT 	*replace;
OBJECT	*repask;
OBJECT 	*find_obj;
OBJECT 	*pos;
OBJECT 	*marken;
OBJECT 	*makrorep;
OBJECT 	*funktionstasten;
OBJECT	*umlautkonv;
OBJECT	*fehler;
OBJECT 	*globalop;
OBJECT 	*localop;
OBJECT  *syntaxop;
OBJECT 	*muster;
OBJECT	*autosave;
OBJECT	*klammer;
OBJECT 	*printer;
OBJECT 	*printer_sub;
OBJECT	*seoptions;

OBJECT 	*winicon;

OBJECT 	*popups;
OBJECT 	*strings;

char		**alertmsg;


static bool	rsc_init = FALSE;

bool init_resource(void)
{
	PATH	rsc_path;
	char t[20];

	/* 1st, look in our own app dir */
	strcpy(rsc_path, gl_appdir);
	strcat(rsc_path, "qed.rsc");
	rsc_init = rsrc_load(rsc_path);

	/* then look in $QED */
	if( !rsc_init && path_from_env("QED", rsc_path))
	{
		strcat(rsc_path, "qed.rsc");
		rsc_init = rsrc_load(rsc_path);
	}
	
	/* then look via shel_find */
	if (!rsc_init)
	{
		strcpy(rsc_path, "qed.rsc");
		shel_find(rsc_path);
		rsc_init = rsrc_load(rsc_path);
	}

	if (rsc_init)
	{
		char	str[25];
		OBJECT	*tmp;
		
		/* Stimmt die RSC-Version? */
		rsrc_gaddr(R_TREE, VERSION, &tmp);
		get_string(tmp, RSC_VERSION, str);
		if (strcmp(str, QED_VERSION) != 0)
		{
			do_alert(1, 0, "[3][Falsche RSC-Version!|Wrong RSC version!][Exit]");
			rsrc_free();
			return FALSE;
		}

		rsrc_gaddr(R_TREE, MENUTREE,	&menu);
		menu[ROOT].ob_width = gl_desk.g_w;
		/* nur tempor�r, da sonst die XXX kurz zusehen sind. */
		init_se_title(FALSE);
		create_menu(menu);

		rsrc_gaddr(R_TREE, ABOUT,		&about);
		fix_dial(about);
		rsrc_gaddr(R_TREE, ABOUT2,		&about2);
		fix_dial(about2);

		rsrc_gaddr(R_TREE, PRINT,		&print);
		fix_dial(print);
		rsrc_gaddr(R_TREE, TEXTINFO,	&textinfo);
		fix_dial(textinfo);
		rsrc_gaddr(R_TREE, PRJINFO,	&prjinfo);
		fix_dial(prjinfo);
		rsrc_gaddr(R_TREE, BLOCKINFO, &blockinfo);
		fix_dial(blockinfo);
		rsrc_gaddr(R_TREE, SORT,      &sort);
		fix_dial(sort);

		rsrc_gaddr(R_TREE, REPLACE,	&replace);
		fix_dial(replace);
		rsrc_gaddr(R_TREE, REPASK,		&repask);
		fix_dial(repask);
		rsrc_gaddr(R_TREE, FINDFILE,	&find_obj);
		fix_dial(find_obj);
		rsrc_gaddr(R_TREE, POS,			&pos);
		fix_dial(pos);
		rsrc_gaddr(R_TREE, MARKEN,		&marken);
		fix_dial(marken);

		rsrc_gaddr(R_TREE, MAKROREP, 	&makrorep);
		fix_dial(makrorep);
		rsrc_gaddr(R_TREE, FUNKTION,	&funktionstasten);
		fix_dial(funktionstasten);
		rsrc_gaddr(R_TREE, UMLAUTE, 	&umlautkonv);
		fix_dial(umlautkonv);
		rsrc_gaddr(R_TREE, FEHLER, 	&fehler);
		fix_dial(fehler);

		rsrc_gaddr(R_TREE, GLOBALOP,	&globalop);
		fix_dial(globalop);
		fix_colorpopobj( globalop, GOFCOL, 0 ); /* Farbe wird in options.c jedesmal neu gesetzt */
		fix_colorpopobj( globalop, GOBCOL, 0 );
		fix_colorpopobj( globalop, GOBLOCKFCOL, 0 );
		fix_colorpopobj( globalop, GOBLOCKBCOL, 0 );
		
		rsrc_gaddr(R_TREE, LOCALOP,	&localop);
		fix_dial(localop);
		rsrc_gaddr(R_TREE, SYNTAXOP,	&syntaxop);
		fix_dial(syntaxop);
		fix_colorpopobj( syntaxop, SYOCOLOR, 0 ); /* Farbe wird in options.c jedesmal neu gesetzt */
		fix_colorpopobj( syntaxop, SYOSELCOLOR, 0 );

		rsrc_gaddr(R_TREE, MUSTER,		&muster);
		fix_dial(muster);
		rsrc_gaddr(R_TREE, AUTOSAVE,  &autosave);
		fix_dial(autosave);
		rsrc_gaddr(R_TREE, KLAMMER,	&klammer);
		fix_dial(klammer);
		rsrc_gaddr(R_TREE, PRINTER,	&printer);
		fix_dial(printer);
		rsrc_gaddr(R_TREE, PRN_SUB,	&printer_sub);
		fix_dial(printer_sub);

		rsrc_gaddr(R_TREE, SEOPTION, 	&seoptions);
		fix_dial(seoptions);

		rsrc_gaddr(R_TREE, WINICON,	&winicon);
		rsrc_gaddr(R_TREE, POPUPS,		&popups);
		fix_popup(popups, TRUE);

		rsrc_gaddr(R_TREE, STRINGS,   &strings);
		rsrc_gaddr(R_FRSTR, NOWINDOW,	&alertmsg);		/* _erster_ Alert */

		/* Versionen eintragen */		
		set_string(about, ADATUM, __DATE__);
		strcpy( t, QED_VERSION );
		strcat( t, QED_REVISION );
		set_string(about, AVERSION, t);
		get_patchlev(__Ident_gnulib, str);
		set_string(about2, AMINT, str);
		get_patchlev(__Ident_gem, str);
		set_string(about2, AGEM, str);
		get_patchlev(__Ident_cflib, str);
		set_string(about2, ACF, str);

#if 0
		/* Hintergrundfarbe Iconify */
		if (gl_planes > 1)
			winicon[WIBOX].ob_spec.obspec.interiorcol = LWHITE;
#endif

		/* Strings in die Zeichenauswahl eintragen */
		set_asciitable_strings(rsc_string(ZEICHSTR1), rsc_string(ZEICHSTR2));
	}
	else
		do_alert(1, 0, "[3][Resource nicht gefunden!|Resource file not found!][Exit]");
	return rsc_init;
}


void term_resource (void)
{
	if (rsc_init)
	{
		delete_menu();
		rsrc_free();
	}
}
