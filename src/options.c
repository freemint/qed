#include <support.h>

#include "global.h"
#include "ausgabe.h"
#include "av.h"
#include "clipbrd.h"
#include "edit.h"
#include "error.h"
#include "file.h"
#include "find.h"
#include "highlite.h"
#include "hl.h"
#include "icon.h"
#include "kurzel.h"
#include "makro.h"
#include "memory.h"
#include "olga.h"
#include "options.h"
#include "poslist.h"
#include "printer.h"
#include "projekt.h"
#include "rsc.h"
#include "se.h"
#include "set.h"
#include "text.h"
#include "version.h"
#include "window.h"
#include "winlist.h"

extern void	menu_help(short title, short item);

static char	buffer[MAX_LINE_LEN];


/*
 * Autosave
*/
bool	as_text, as_prj;
bool	as_text_ask, as_prj_ask;
short	as_text_min, as_prj_min;

void set_autosave_options(void)
{
	short	antw;

	set_state(autosave, ASTEXT, OS_SELECTED, as_text);
	set_short(autosave, ASTMIN, as_text_min);
	set_state(autosave, ASTASK, OS_SELECTED, as_text_ask);

	set_state(autosave, ASPROJ, OS_SELECTED, as_prj);
	set_short(autosave, ASPMIN, as_prj_min);
	set_state(autosave, ASPASK, OS_SELECTED, as_prj_ask);

	antw = simple_mdial(autosave, ASTMIN) & 0x7fff;
	if (antw == ASOK)
	{
		as_text = get_state(autosave, ASTEXT, OS_SELECTED);
		as_text_min = get_short(autosave, ASTMIN);
		if (as_text_min == 0)
			as_text = FALSE;
		if (as_text_min > 59)
			as_text_min = 59;
		as_text_ask = get_state(autosave, ASTASK, OS_SELECTED);

		as_prj = get_state(autosave, ASPROJ, OS_SELECTED);
		as_prj_min = get_short(autosave, ASPMIN);
		if (as_prj_min == 0)
			as_prj = FALSE;
		if (as_prj_min > 59)
			as_prj_min = 59;
		as_prj_ask = get_state(autosave, ASPASK, OS_SELECTED);

		do_all_icon(ALL_TYPES, DO_AUTOSAVE);
	}
}


/*
 * Globale Optionen
*/
bool	clip_on_disk, wind_cycle, f_to_desk;
bool syntax_active;
bool syntax_setsamename;
short	transfer_size, bin_line_len;
short	fg_color, bg_color;
short fg_block_color, bg_block_color;
bool	save_opt, save_win, overwrite, blinking_cursor, ctrl_mark_mode, olga_autostart,
		emu_klammer;
PATH	helpprog;
char	bin_extension[BIN_ANZ][MASK_LEN+1];

static void do_avopen(WINDOWP window)
{
	send_avwinopen(window->handle);
}

static void do_avclose(WINDOWP window)
{
	send_avwinclose(window->handle);
}

#if 0
static void set_popcolor(short s_obj, short d_obj)
{
	OBSPEC	spec;
	short		color;

	spec.index = get_obspec(popups, s_obj);
	color = spec.obspec.interiorcol;			/* neue Farbe holen */
	spec.index = get_obspec(globalop, d_obj);
	spec.obspec.interiorcol = color;			/* neue Farbe setzen */
	set_obspec(globalop, d_obj, spec.index);
}
#endif


void set_global_options(void)
{
	short	i, antw = 0;
	bool	old_cycle, new_cycle, new_fg, new_bg, new_block_fg, new_block_bg;
	bool	close = FALSE;
	char	n[23] = "";
	MDIAL	*dial;
	PATH	new_helpprog;

	old_cycle = wind_cycle;
	set_state(globalop, GOASAVE, OS_SELECTED, save_opt);
	set_state(globalop, GOSAVEWIN, OS_SELECTED, save_win);
	set_state(globalop, GOCLIP, OS_DISABLED, (clip_dir[0] == EOS));
	set_state(globalop, GOCLIP, OS_SELECTED, clip_on_disk);
	set_state(globalop, GOBLINK, OS_SELECTED, blinking_cursor);
	set_state(globalop, GOCTRL, OS_SELECTED, ctrl_mark_mode);
	set_state(globalop, GOAVWIN, OS_SELECTED, wind_cycle);
	set_state(globalop, GOAVKEY, OS_SELECTED, f_to_desk);
	set_state(globalop, GOOLGA, OS_SELECTED, olga_autostart);
	set_state(globalop, GOKLAMMER, OS_SELECTED, emu_klammer);

	set_state(globalop, GOWDIAL, OS_DISABLED, !prn->pdlg_avail);
	set_state(globalop, GOWDIAL, OS_SELECTED, prn->use_pdlg);

	set_short(globalop, GOTRANS, transfer_size);

	set_state(globalop, GOSYNTAX, OS_SELECTED, syntax_active);

	if (helpprog[0] != EOS)
		make_shortpath(helpprog, n, 22);
	else
		strcpy(n, "");
	set_string(globalop, GOHELPNAME, n);
	strcpy(new_helpprog, helpprog);

	set_short(globalop, GOBLEN, bin_line_len);
	for (i = 4; i < BIN_ANZ; i++)
		set_string(globalop, i - 4 + GOBEXT1, bin_extension[i]);

	set_popobjcolor(globalop, GOFCOL, fg_color);
	set_popobjcolor(globalop, GOBCOL, bg_color);
	set_popobjcolor(globalop, GOBLOCKFCOL, fg_block_color);
	set_popobjcolor(globalop, GOBLOCKBCOL, bg_block_color);
	new_fg = fg_color;
	new_bg = bg_color;
	new_block_fg = fg_block_color;
	new_block_bg = bg_block_color;

	dial = open_mdial(globalop, GOTRANS);
	if (dial != NULL)
	{
		while (!close)
		{
			antw = do_mdial(dial) & 0x7fff;
			switch (antw)
			{
				case GOHELP :
					menu_help(TOPTIONS, MGLOBALO);
					break;

				case GOHELPSEL :
					if (select_single(new_helpprog, "", rsc_string(FINDHPSTR)))
					{
						make_shortpath(new_helpprog, n, 22);
						set_string(globalop, GOHELPNAME, n);
						redraw_mdobj(dial, GOHELPNAME);
					}
					break;

				case GOFCSTR :
				case GOFCOL :
					if (antw == GOFCOL)
						i = handle_colorpop(globalop, GOFCOL, POP_OPEN, 8, FALSE);
					else
						i = handle_colorpop(globalop, GOFCOL, POP_CYCLE, 8, FALSE);
					if (i > -1)
						new_fg = i;

					break;

				case GOBCSTR :
				case GOBCOL :
					if (antw == GOBCOL)
						i = handle_colorpop(globalop, GOBCOL, POP_OPEN, 8, FALSE);
					else
						i = handle_colorpop(globalop, GOBCOL, POP_CYCLE, 8, FALSE);
					if (i > -1)
						new_bg = i;
					break;

				case GOBLOCKFCOL :
					i = handle_colorpop(globalop, GOBLOCKFCOL, POP_OPEN, 8, FALSE);
					if (i > -1)
						new_block_fg = i;

					break;

				case GOBLOCKBCOL :
					i = handle_colorpop(globalop, GOBLOCKBCOL, POP_OPEN, 8, FALSE);
					if (i > -1)
						new_block_bg = i;
					break;

				default:
					close = TRUE;
					break;
			}
			if (!close)
			{
				set_state(globalop, antw, OS_SELECTED, FALSE);
				redraw_mdobj(dial, antw);
			}
		}
		set_state(globalop, antw, OS_SELECTED, FALSE);
		close_mdial(dial);
		if (antw == GOOK)
		{
			save_opt = get_state(globalop, GOASAVE, OS_SELECTED);
			save_win = get_state(globalop, GOSAVEWIN, OS_SELECTED);
			if (clip_dir[0] != EOS)
				clip_on_disk = get_state(globalop, GOCLIP, OS_SELECTED);
			blinking_cursor = get_state(globalop, GOBLINK, OS_SELECTED);
			ctrl_mark_mode = get_state(globalop, GOCTRL, OS_SELECTED);
			prn->use_pdlg = get_state(globalop, GOWDIAL, OS_SELECTED);
			olga_autostart = get_state(globalop, GOOLGA, OS_SELECTED);
			emu_klammer = get_state(globalop, GOKLAMMER, OS_SELECTED);
			transfer_size = get_short(globalop, GOTRANS);
			if (transfer_size == 0)
				transfer_size = 1;

			if( syntax_active && !get_state(globalop, GOSYNTAX, OS_SELECTED))
			{
				hl_disable();
				syntax_active = FALSE;
			}
			else if( !syntax_active && get_state(globalop, GOSYNTAX, OS_SELECTED))
			{
				syntax_active = TRUE;
				hl_enable();
			}

			new_cycle = get_state(globalop, GOAVWIN, OS_SELECTED);
			if (old_cycle && !new_cycle)						/* war an, nun aus */
			{
				do_all_window(CLASS_ALL, do_avclose);
				wind_cycle = new_cycle;
			}
			if (new_cycle && !old_cycle)						/* nun an, war aus */
			{
				wind_cycle = new_cycle;
				do_all_window(CLASS_ALL, do_avopen);
			}
			f_to_desk = get_state(globalop, GOAVKEY, OS_SELECTED);

			bin_line_len = get_short(globalop, GOBLEN);
			if (bin_line_len < 1)
				bin_line_len = 1;
			if (bin_line_len > MAX_LINE_LEN)
				bin_line_len = MAX_LINE_LEN;
			for (i = 4; i < BIN_ANZ; i++)
				get_string(globalop, i - 4 + GOBEXT1, bin_extension[i]);

			strcpy(helpprog, new_helpprog);

			if (new_fg != fg_color || new_bg != bg_color)
			{
				fg_color = new_fg;
				bg_color = new_bg;
				color_change();
			}
			if (new_block_fg != fg_block_color || new_block_bg != bg_block_color)
			{
				fg_block_color = new_block_fg;
				bg_block_color = new_block_bg;
				color_change();
			}

			if (olga_autostart)
				init_olga();
		}
	}
}

/*
 * Klammerpaare
*/
char	klammer_auf[11],
		klammer_zu[11];

void set_klammer_options(void)
{
	short	antw;
	char	s1[11], s2[11];

	set_string(klammer, KPAUF, klammer_auf);
	set_string(klammer, KPZU, klammer_zu);
	antw = simple_mdial(klammer, KPAUF);
	if (antw == KPOK)
	{
		get_string(klammer, KPAUF, s1);
		get_string(klammer, KPZU, s2);
		if (strlen(s1) == strlen(s2))
		{
			strcpy(klammer_auf, s1);
			strcpy(klammer_zu, s2);
		}
		else
			note(1, 0, KLAMMERERR);
	}
}



/*
 * Lokale Optionen
*/
LOCOPT	local_options[LOCAL_ANZ];

static short 	active_local_option;
static bool	krz_changed = FALSE;

static void null_locopt(LOCOPT *lo)
{
	strcpy(lo->muster, "");
	lo->tab = FALSE;
	lo->tabsize = 8;
	lo->einruecken = FALSE;
	strcpy(lo->wort_str,"A-Za-z0-9");
	str2set(lo->wort_str,lo->wort_set);
	lo->umbrechen  = FALSE;
	lo->format_by_load = FALSE;
	lo->format_by_paste = FALSE;
	strcpy(lo->umbruch_str,"");
	str2set(lo->umbruch_str,lo->umbruch_set);
	lo->lineal_len = 0;
	strcpy(lo->backup_ext, "");
	lo->backup = FALSE;
	lo->show_end = FALSE;
}

static void option_fill(void)
{
	char 		str[13] = "";
	LOCOPTP	lo;

	lo = &local_options[active_local_option];

	if (active_local_option < 2)
		strcpy(str, " ");
	else
		strcpy(str, " *.");
	strcat(str, lo->muster);
	set_string(localop, OTYPE, str);

	set_state(localop, OTAB, OS_SELECTED, lo->tab);
	set_short(localop, OTABSIZE, lo->tabsize);
	set_state(localop, OEINRUCK, OS_SELECTED, lo->einruecken);
	set_string(localop, OWORT, lo->wort_str);
	set_state(localop, OUMBRUCH, OS_SELECTED, lo->umbrechen);
	set_state(localop, OFORMLOAD, OS_SELECTED, lo->format_by_load);
	set_state(localop, OFORMPASTE, OS_SELECTED, lo->format_by_paste);
	set_string(localop, OUMBTEXT, lo->umbruch_str);
	set_state(localop, OBACKUP, OS_SELECTED, lo->backup);
	set_string(localop, OEXT, lo->backup_ext);
	set_short(localop, OLINEAL, lo->lineal_len);
	if (lo->kurzel[0] != EOS)
		file_name(lo->kurzel, str, FALSE);
	else
		strcpy(str, rsc_string(KURZELSTR));
	set_string(localop, OKURZELNAME, str);
	set_state(localop, OSHOWEND, OS_SELECTED, lo->show_end);
}

static void option_get(void)
{
	LOCOPTP	lo;

	lo = &local_options[active_local_option];
	lo->tab = get_state(localop, OTAB, OS_SELECTED);
	lo->tabsize = get_short(localop, OTABSIZE);
	if (lo->tabsize < 1 || lo->tabsize > 50)
		lo->tabsize = 3;
	lo->umbrechen = get_state(localop, OUMBRUCH, OS_SELECTED);
	lo->einruecken = get_state(localop, OEINRUCK, OS_SELECTED);
	get_string(localop, OUMBTEXT, lo->umbruch_str);
	str2set(lo->umbruch_str,lo->umbruch_set);
	lo->format_by_load = get_state(localop, OFORMLOAD, OS_SELECTED);
	lo->format_by_paste = get_state(localop, OFORMPASTE, OS_SELECTED);
	lo->backup = get_state(localop, OBACKUP, OS_SELECTED);
	get_string(localop, OEXT, lo->backup_ext);
	lo->lineal_len = get_short(localop, OLINEAL);
	if (lo->lineal_len < 3 || lo->lineal_len > MAX_LINE_LEN)
		lo->lineal_len = 65;
	get_string(localop, OWORT, lo->wort_str);
	str2set(lo->wort_str,lo->wort_set);
	lo->show_end = get_state(localop, OSHOWEND, OS_SELECTED);
}


static void config_muster(void)
{
	short	i, antw;
	char 	str[MASK_LEN+1];

	for (i = MFIRST; i <= MLAST; i++)
		set_string(muster, i, local_options[i + 2 - MFIRST].muster);

	antw = simple_mdial(muster, MFIRST);
	if (antw == MOK)
	{
		for (i = MFIRST; i <= MLAST; i++)
		{
			get_string(muster, i, str);
			strcpy(local_options[i + 2 - MFIRST].muster, str);
		}

		/* wurde der aktive gelîscht? */
		if (local_options[active_local_option].muster[0] == EOS)
		{
			active_local_option = 0;
			set_string(localop, OTYPE, " *");
		}
	}
}


static bool build_popup(POPUP *pop)
{
	char	str[MASK_LEN + 4];
	short	i;

	strcpy(str, " ");
	strcat(str, local_options[0].muster);			/* * */
	create_popup(pop, LOCAL_ANZ, MASK_LEN+2, str);

	strcpy(str, " ");										/* Binary */
	strcat(str, local_options[1].muster);
	append_popup(pop, str);

	for (i = 2; i < LOCAL_ANZ; i++)
	{
		if (local_options[i].muster[0] != EOS)
		{
			strcpy(str, " *.");
			strcat(str, local_options[i].muster);
			append_popup(pop, str);
		}
	}

	append_popup(pop, "--");
	append_popup(pop, rsc_string(CHAGESTR));

	fix_popup(pop->tree, TRUE);

	return (pop->tree != NULL);
}


void set_local_options(void)
{
	short		y, antw = 0;
	WINDOWP 	window;
	PATH		save_name;
	POPUP		pop;
	bool		close = FALSE;
	MDIAL		*dial;
	LOCOPT	*backup;

	active_local_option = 0;
	window = winlist_top();

	if ((window != NULL) && (window->class == CLASS_EDIT))
	{
		TEXTP t_ptr = get_text(window->handle);

		active_local_option = (short)(t_ptr->loc_opt - local_options);
	}

	option_fill();

	strcpy(save_name, local_options[active_local_option].kurzel);
	backup = (LOCOPT *)malloc(LOCAL_ANZ * sizeof(LOCOPT));
	memcpy(backup, local_options, (LOCAL_ANZ * sizeof(LOCOPT)));

	build_popup(&pop);

	dial = open_mdial(localop, OTABSIZE);
	if (dial != NULL)
	{
		while (!close)
		{
			antw = do_mdial(dial) & 0x7fff;
			switch (antw)
			{
				case OTYPESTR :
				case OTYPE :
					if (antw == OTYPE)
						y = handle_popup(localop, OTYPE, pop.tree, 0, POP_OPEN) - 1;
					else
						y = handle_popup(localop, OTYPE, pop.tree, 0, POP_CYCLE) - 1;
					if (y >= 0 && y != active_local_option)
					{
						if (y == pop.akt_item - 1)
						{
							config_muster();
							free_popup(&pop);		/* Popup neu aufbauen */
							build_popup(&pop);
							set_string(localop, OTYPE, " *");
						}
						else
						{
							option_get();
							active_local_option = y;
						}
						option_fill();
						redraw_mdobj(dial, OBOX);
					}
					break;
				case OKURZEL:
					if (shift_pressed())
					{
						strcpy(local_options[active_local_option].kurzel, "");
						krz_changed = TRUE;
						set_string(localop, OKURZELNAME, rsc_string(KURZELSTR));
						redraw_mdobj(dial, OKURZELNAME);
					}
					else
					{
						if (select_single(local_options[active_local_option].kurzel,
												"*.krz", rsc_string(FINDKURZELSTR)))
						{
							FILENAME str;

							file_name(local_options[active_local_option].kurzel, str, FALSE);
							krz_changed = TRUE;
							set_string(localop, OKURZELNAME, str);
						}
					}
					break;

				case LCHELP :
					menu_help(TOPTIONS, MLOCALOP);
					break;

				default:
					close = TRUE;
					break;
			}
			if (!close)
			{
				set_state(localop, antw, OS_SELECTED, FALSE);
				redraw_mdobj(dial, antw);
			}
		}
		set_state(localop, antw, OS_SELECTED, FALSE);
		close_mdial(dial);
		free_popup(&pop);

		if (antw == OOK)
		{
			option_get();
			update_loc_opt();
			absatz_edit();
			ch_kurzel(local_options[active_local_option].kurzel, TRUE);
		}
		else
		{
			strcpy(local_options[active_local_option].kurzel, save_name);
			memcpy(local_options, backup, (LOCAL_ANZ * sizeof(LOCOPT)));
		}
		free(backup);
	}
}

/* Syntax-Optionen */
static bool build_txtname_popup(int txtidx, POPUP *pop)
{
	char *name;
	int idx = 0;
	int count = 0;
	char str[256];
	bool ret = FALSE;

	while (Hl_EnumTxtNames(NULL, &count));

	if (Hl_EnumTxtNames(&name, &idx))
	{
		strcpy(str, " ");
		strncpy(str+1, name, syntaxop[SYOTXT].ob_spec.tedinfo->te_txtlen - 2);
		if (create_popup(pop, count, syntaxop[SYOTXT].ob_spec.tedinfo->te_txtlen , str))
		{
			ret = TRUE;
			while (Hl_EnumTxtNames(&name, &idx))
			{
				strcpy(str, " ");
				strncpy(str+1, name, syntaxop[SYOTXT].ob_spec.tedinfo->te_txtlen - 2);
				append_popup(pop, str);
			}
		}
	}
	if (Hl_EnumTxtNames(&name, &txtidx))
	{
		strcpy(str, " ");
		strncpy(str+1, name, syntaxop[SYOTXT].ob_spec.tedinfo->te_txtlen - 2);
		set_string(syntaxop, SYOTXT, str);
	}
	return ret;
}

static void disable_syntax_settings(bool disableit)
{
	set_string(syntaxop, SYORULE, "");
	set_state(syntaxop, SYORULE, OS_DISABLED, disableit);
	set_state(syntaxop, SYOBOLD, OS_DISABLED, disableit);
	set_state(syntaxop, SYOLIGHT, OS_DISABLED, disableit);
	set_state(syntaxop, SYOITALIC, OS_DISABLED, disableit);
	set_state(syntaxop, SYOCOLOR, OS_DISABLED, disableit);
	set_state(syntaxop, SYOSELCOLOR, OS_DISABLED, disableit);
	set_state(syntaxop, SYOCOLORTXT, OS_DISABLED, disableit);
	set_state(syntaxop, SYOSELCOLORTXT, OS_DISABLED, disableit);
	set_state(syntaxop, SYOSETSAMENAME, OS_DISABLED, disableit);
	if (disableit)
	{
		set_popobjcolor(syntaxop, SYOCOLOR, 1);
		set_popobjcolor(syntaxop, SYOSELCOLOR, 1);
	}
}

static void get_syntax_settings(int txtidx, int idx)
{
	char str[256];
	HL_RULEINFO ri;

	if (!Hl_EnumRules(txtidx, &ri, &idx)) /* idx wird zerstîrt */
	{
		disable_syntax_settings(TRUE);
		return;
	}
	disable_syntax_settings(FALSE);

	strcpy(str, " ");
	strncpy(str+1, ri.name, syntaxop[SYORULE].ob_spec.tedinfo->te_txtlen - 2);
	set_string(syntaxop, SYORULE, str);
	set_state(syntaxop, SYOBOLD, OS_SELECTED, ri.attribs & HL_BOLD);
	set_state(syntaxop, SYOLIGHT, OS_SELECTED, ri.attribs & HL_LIGHT);
	set_state(syntaxop, SYOITALIC, OS_SELECTED, ri.attribs & HL_ITALIC);
	set_popobjcolor(syntaxop, SYOCOLOR, (ri.attribs & HL_COLOR) ? (short) ri.color : -2);
	set_popobjcolor(syntaxop, SYOSELCOLOR, (ri.attribs & HL_SELCOLOR) ? (short) ri.selcolor : -2);

	set_state(syntaxop, SYOSETSAMENAME, OS_SELECTED, syntax_setsamename );

	set_state(syntaxop, SYOACTIVE, OS_SELECTED, Hl_IsActive(txtidx));

}

static void store_syntax_settings(int txtidx, int idx)
{
	HL_RULEINFO ri, tri;
	int tidx = idx;
	if (syntaxop[SYOBOLD].ob_state & OS_DISABLED)
		return;
	ri.attribs =   ((syntaxop[SYOBOLD].ob_state & OS_SELECTED) ? HL_BOLD : 0)
	             | ((syntaxop[SYOLIGHT].ob_state & OS_SELECTED) ? HL_LIGHT : 0)
	             | ((syntaxop[SYOITALIC].ob_state & OS_SELECTED) ? HL_ITALIC : 0)
	             | ((get_popobjcolor(syntaxop, SYOCOLOR) != -2) ? HL_COLOR : 0)
	             | ((get_popobjcolor(syntaxop, SYOSELCOLOR) != -2) ? HL_SELCOLOR : 0);
	ri.color = get_popobjcolor(syntaxop, SYOCOLOR);
	ri.selcolor = get_popobjcolor(syntaxop, SYOSELCOLOR);

	Hl_SetActive(txtidx, syntaxop[SYOACTIVE].ob_state & OS_SELECTED);

	Hl_EnumRules( txtidx, &tri, &tidx ); /* check if rule settings have really changed */
	if( tri.attribs != ri.attribs        /* otherwise all rules would be changed if only SYOACTIVE had changed */
	||  tri.color != ri.color
	||  tri.selcolor != ri.selcolor )
	{
		if( syntax_setsamename )
		{
			HL_RULEINFO ltri, cri;
			Hl_EnumRules( txtidx, &ltri, &idx ); /* get rule name in ltri.name */
			txtidx = 0;
			while( Hl_EnumTxtNames( NULL, &txtidx ) ) /* compare for all texts */
			{
				idx = 0;
				while( Hl_EnumRules( txtidx-1, &cri, &idx )) /* and all rules */
					if( stricmp( ltri.name, cri.name ) == 0 ) /* set it if same name */
						Hl_ChangeRule(txtidx-1, idx-1, &ri);
			}
		}
		else
			Hl_ChangeRule(txtidx, idx, &ri);
	}
}



static bool build_rule_popup(int txtidx, POPUP *pop)
{
	int idx = 0;
	int count = 0;
	char str[256];
	HL_RULEINFO ri;
	bool ret = FALSE;

	while (Hl_EnumRules(txtidx, NULL, &count));

	if (Hl_EnumRules(txtidx, &ri, &idx))
	{
		strcpy(str, " ");
		strncpy(str+1, ri.name, syntaxop[SYORULE].ob_spec.tedinfo->te_txtlen - 2);
		if (create_popup(pop, count, syntaxop[SYORULE].ob_spec.tedinfo->te_txtlen , str))
		{
			ret = TRUE;
			while (Hl_EnumRules(txtidx, &ri, &idx))
			{
				strcpy(str, " ");
				strncpy(str+1, ri.name, syntaxop[SYORULE].ob_spec.tedinfo->te_txtlen - 2);
				append_popup(pop, str);
			}
		}
	}
	return ret;
}

void set_syntax_options(void)
{
	short	y, antw = 0;
	bool	close = FALSE;
	WINDOWP window;
	MDIAL	*dial;
	POPUP   txtnames, rules;
	int     act_txttype = 0, act_rule = 0;
	bool    saved = Hl_SaveSettings();
	bool setsamename;

	window = winlist_top();
	if ((window != NULL) && (window->class == CLASS_EDIT))
	{
		TEXTP t_ptr = get_text(window->handle);
		PATH extension;
		split_extension(t_ptr->filename, NULL, extension);
		if ((act_txttype = Hl_TxtIndexByTxttype(extension)) == -1)
			act_txttype = 0;
	}

	if (!build_txtname_popup(act_txttype, &txtnames))
	{
		set_string(syntaxop, SYOTXT, "");
		set_state(syntaxop, SYOTXT, OS_DISABLED, TRUE);
		set_state(syntaxop, SYOACTIVE, OS_DISABLED, TRUE);
	}
	build_rule_popup(act_txttype, &rules);
	get_syntax_settings(act_txttype, 0);

	setsamename = syntax_setsamename;

	dial = open_mdial(syntaxop, 0);
	if (dial != NULL)
	{
		while (!close)
		{
			antw = do_mdial(dial) & 0x7fff;
			switch (antw)
			{
				case SYOHELP:
					menu_help(TOPTIONS, MSYNTAXOP);
					break;

				case SYOTXT:
					y = handle_popup(syntaxop, SYOTXT, txtnames.tree, 0, POP_OPEN) - 1;
					if (y == -1)
						break;
					store_syntax_settings(act_txttype, act_rule);
					act_txttype = y;
					act_rule = 0;
					free_popup(&rules);
					build_rule_popup(act_txttype, &rules);
					get_syntax_settings(act_txttype, 0);
					redraw_mdobj(dial, SYORULE);
					redraw_mdobj(dial, SYORULEFRAME);
					redraw_mdobj(dial, SYOACTIVE);
					break;

				case SYORULE:
					y = handle_popup(syntaxop, SYORULE, rules.tree, 0, POP_OPEN) - 1;
					if (y == -1)
						break;
					store_syntax_settings(act_txttype, act_rule);
					act_rule = y;
					get_syntax_settings(act_txttype, act_rule);
					redraw_mdobj(dial, SYORULEFRAME);
					break;

				case SYOSETSAMENAME:
					syntax_setsamename = syntaxop[SYOSETSAMENAME].ob_state & OS_SELECTED;
					break;

				case SYOCOLOR:
					handle_colorpop(syntaxop, SYOCOLOR, POP_OPEN, 8, TRUE);
					break;

				case SYOSELCOLOR:
					handle_colorpop(syntaxop, SYOSELCOLOR, POP_OPEN, 8, TRUE);
					break;

				case SYOOK:
					store_syntax_settings(act_txttype, act_rule);
					if (saved)
						Hl_DeleteSaveSettings();
					hl_update_all();
					close = TRUE;
					break;

				case SYOCANCEL:
					if (saved)
						Hl_RestoreSettings();
					syntax_setsamename = setsamename;
					close = TRUE;
					break;

				default:
					close = TRUE;
					break;

			}
			if (!close && antw != SYOSETSAMENAME)
			{
				set_state(syntaxop, antw, OS_SELECTED, FALSE);
				redraw_mdobj(dial, antw);
			}
		}
		set_state(syntaxop, antw, OS_SELECTED, FALSE);
		close_mdial(dial);
		free_popup(&txtnames);
	}
}

/*
 * Defaulteinstellungen
*/
void init_default_var(void)
{
	short 		i;
	char		*c;

	font_id = 1;
	font_pts = 10;

	s_grkl     = FALSE;
	s_vorw	  = TRUE;
	s_global	  = TRUE;
	s_quant    = FALSE;
	s_wort	  = FALSE;
	s_round	  = FALSE;
	r_modus	  = RP_FIRST;
	s_str[0] = EOS;
	r_str[0] = EOS;
	ff_rekursiv= FALSE;
	ff_mask[0] = EOS;
	for (i = 0; i < HIST_ANZ; i++)
	{
		s_history[i][0] = EOS;
		r_history[i][0] = EOS;
	}
	rp_box_x	  = 0;
	rp_box_y	  = 0;

	save_opt	  = FALSE;
	save_win  = FALSE;
	wind_cycle = FALSE;
	clip_on_disk = TRUE;
	overwrite  = FALSE;
	transfer_size = 100;
	syntax_active = TRUE;
	syntax_setsamename = FALSE;

	c = getenv("STGUIDE");
	if (c != NULL)
		strcpy(helpprog, c);
	else
		helpprog[0] = EOS;

	blinking_cursor = TRUE;
	ctrl_mark_mode = FALSE;
	f_to_desk = FALSE;

	for (i = 0; i < FEHLERANZ; i++)
		error[i][0] = EOS;

	for (i = 0; i < LOCAL_ANZ; i++)
		null_locopt(&local_options[i]);

	/* Default 1: * */
	strcpy(local_options[0].muster, "*");
	local_options[0].tab        = TRUE;
	local_options[0].tabsize    = 8;
	local_options[0].einruecken = TRUE;
	strcpy(local_options[0].umbruch_str,"- \t");
	str2set(local_options[0].umbruch_str,local_options[0].umbruch_set);
	local_options[0].lineal_len = 70;
	strcpy(local_options[0].backup_ext, "BAK");
	local_options[0].backup = FALSE;
	local_options[0].show_end = FALSE;

	/* Default 2: BinÑr */
	strcpy(local_options[1].muster, rsc_string(BINSTR));
	local_options[1].tab = FALSE;
	local_options[1].tabsize = 1;
	strcpy(local_options[1].backup_ext, "BAK");
	local_options[1].backup = TRUE;

	strcpy(bin_extension[0], "prg");
	strcpy(bin_extension[1], "app");
	strcpy(bin_extension[2], "tos");
	strcpy(bin_extension[3], "ttp");
	for (i = 4; i < BIN_ANZ; i++)
		bin_extension[i][0] = EOS;

	se_autosave = FALSE;
	se_autosearch = FALSE;
	se_ignoreclose = FALSE;

	for (i = 0; i < SHELLANZ; i++)
	{
		se_shells[i].name[0] = EOS;
		se_shells[i].makefile[0] = EOS;
	}

	umlaut_from = 0;
	umlaut_to = 0;

	as_text 		= FALSE;
	as_text_min = 0;
	as_text_ask = FALSE;
	as_prj 		= FALSE;
	as_prj_min = 0;
	as_prj_ask = FALSE;

	bin_line_len = 80;

	strcpy(klammer_auf, "({[<\"\'");
	strcpy(klammer_zu, ")}]>\"\'");

	fg_color = G_BLACK;
	bg_color = G_WHITE;
	fg_block_color = bg_color;
	bg_block_color = fg_color;

	olga_autostart = FALSE;
	emu_klammer = FALSE;
}


/*
 * Datei
*/
static PATH	cfg_path = "";
static FILENAME	dsp_name;	/* Name der Display-Datei */
static FILENAME col_name;	/* Name der Farb-Datei */
static FILE	*fd;
static LOCOPTP	lo = NULL;
static short	muster_nr = 1;

static bool get_cfg_path(void)
{
	bool found;
	short pl;
	strcpy(cfg_path, CFGNAME);
	found = get_config_file( cfg_path, FALSE );
	sprintf(dsp_name, "%04d%04d.qed", gl_desk.g_x + gl_desk.g_w, gl_desk.g_y + gl_desk.g_h);
	pl = min( gl_planes, 8 ); /* max. 8 planes supported; higher resolutions get the same color */
	sprintf(col_name, "col%02dbit.qed", pl );
	return found;
}

/******************************************************************************/
/* Dateioperation: Laden																		*/
/******************************************************************************/
void read_cfg_bool(char *str, bool *val)
{
	if (stricmp(str, "TRUE") == 0)
		*val = TRUE;
	if (stricmp(str, "FALSE") == 0)
		*val = FALSE;
}

void read_cfg_str(char *str, char *val)
{
	val[0] = EOS;
	if ((str[0] == '"') && (str[1] == '"'))	/* nur "" -> leer */
		return;
	else
	{
		short	len, i, j;

		if (str[0] == '"')
		{
			len = (short)strlen(str);
			j = 0;
			i = 1;
			while ((str[i] != '\"') && (i < len))
			{
				if ((str[i] == '\\') && (str[i+1] == '"'))
					i++;
				val[j++] = str[i++];
			}
			val[j] = EOS;
		}
	}
}


static void parse_line(POSENTRY **arglist, char *zeile)
{
	char	var[30], *p, tmp[80];
	short	x, i, d;
	long	y;
	PATH	filename;

	// STan: BUG? CRLF vs LF newlines in the .cfg file?
	// 	 The real solution is to ensure the .cfg file is either in CRLF or LF
	// 	 mode regardless to whether it runs under MiNT or SingleTOS. Other, quite nice
	// 	 solution is to read and write the documentation using QED text file routine
	// 	 itself. But this would need to rewrite the whole configuratin stuff.
	// Skip empty lines
	if ( strlen(zeile) == 0 )
		return;

	p = strchr(zeile, '=');
	if (p != NULL)
	{
		strncpy(var, zeile, p-zeile);
		var[p-zeile] = EOS;
		strcpy(buffer, p+1);

		/* Autosave */
		if (strcmp(var, "AutoSavePrj") == 0)
			read_cfg_bool(buffer, &as_prj);
		else if(strcmp(var, "AutoSavePrjAsk") == 0)
			read_cfg_bool(buffer, &as_prj_ask);
		else if(strcmp(var, "AutoSavePrjMin") == 0)
			as_prj_min = atoi(buffer);
		else if(strcmp(var, "AutoSaveText") == 0)
			read_cfg_bool(buffer, &as_text);
		else if(strcmp(var, "AutoSaveTextAsk") == 0)
			read_cfg_bool(buffer, &as_text_ask);
		else if(strcmp(var, "AutoSaveTextMin") == 0)
			as_text_min = atoi(buffer);

		/* BinÑr-Extensions */
		else if (strcmp(var, "BinExtension") == 0)
		{
			read_cfg_str(buffer, tmp);
			for (i = 0; i < BIN_ANZ; i++)
			{
				if (stricmp(bin_extension[i], tmp) == 0)	/* schon drin */
					break;
				if (bin_extension[i][0] == EOS)
				{
					strcpy(bin_extension[i], tmp);
					break;
				}
			}
		}

		/* DefaultPrj */
		else if(strcmp(var, "DefaultPrj") == 0)
		{
			read_cfg_str(buffer, def_prj_path);
			set_def_prj();
		}

		/* Fehlerzeilen */
		else if(strcmp(var, "Error") == 0)
		{
			read_cfg_str(buffer, tmp);
			set_errorline(tmp);
		}

		/* Globales */
		else if (strcmp(var, "GlobalAutosaveCfg") == 0)
			read_cfg_bool(buffer, &save_opt);
		else if (strcmp(var, "GlobalSaveWindows") == 0)
			read_cfg_bool(buffer, &save_win);
		else if (strcmp(var, "GlobalFgColor") == 0)
			fg_color = atoi(buffer);
		else if (strcmp(var, "GlobalBgColor") == 0)
			bg_color = atoi(buffer);
		else if (strcmp(var, "GlobalBlockFgColor") == 0)
			fg_block_color = atoi(buffer);
		else if (strcmp(var, "GlobalBlockBgColor") == 0)
			bg_block_color = atoi(buffer);
		else if (strcmp(var, "GlobalBinLineLen") == 0)
			bin_line_len = atoi(buffer);
		else if (strcmp(var, "GlobalBlinkCursor") == 0)
			read_cfg_bool(buffer, &blinking_cursor);
		else if (strcmp(var, "GlobalCtrlBlock") == 0)
			read_cfg_bool(buffer, &ctrl_mark_mode);
		else if (strcmp(var, "GlobalEmuKlammer") == 0)
			read_cfg_bool(buffer, &emu_klammer);
		else if (strcmp(var, "GlobalFtoDesk") == 0)
			read_cfg_bool(buffer, &f_to_desk);
		else if (strcmp(var, "GlobalGEMClip") == 0)
			read_cfg_bool(buffer, &clip_on_disk);
		else if (strcmp(var, "GlobalOlga") == 0)
			read_cfg_bool(buffer, &olga_autostart);
		else if (strcmp(var, "GlobalOverwrite") == 0)
			read_cfg_bool(buffer, &overwrite);
		else if (strcmp(var, "GlobalTransSize") == 0)
			transfer_size = atoi(buffer);
		else if (strcmp(var, "GlobalWindCycle") == 0)
			read_cfg_bool(buffer, &wind_cycle);
		else if (strcmp(var, "GlobalSyntaxActive") == 0)
			read_cfg_bool(buffer, &syntax_active);
		else if (strcmp(var, "SyntaxSetSameName") == 0)
			read_cfg_bool(buffer, &syntax_setsamename);

		/* Hilfe-Programm */
		else if (strcmp(var, "HelpProgram") == 0)
			read_cfg_str(buffer, helpprog);

		/* Klammerpaare */
		else if (strcmp(var, "KlammerAuf") == 0)
			read_cfg_str(buffer, klammer_auf);
		else if (strcmp(var, "KlammerZu") == 0)
			read_cfg_str(buffer, klammer_zu);

		/* Lokales */
		else if (strcmp(var, "LocalBegin") == 0)
		{
			read_cfg_str(buffer, tmp);

			/* SonderfÑlle */
			if (strcmp(tmp, "*") == 0)
				muster_nr = 0;
			else if (strcmp(tmp, rsc_string(BINSTR)) == 0)
				muster_nr = 1;
			else
				muster_nr++;

			lo = &(local_options[muster_nr]);
			strcpy(lo->muster, tmp);
		}
		else if (strcmp(var, "LocalEnd") == 0)
		{
			lo = NULL;
			if (muster_nr == 0)
				muster_nr = 1;
		}
		else if ((strcmp(var, "LocalBackup") == 0) && (lo != NULL))
			read_cfg_bool(buffer, &(lo->backup));
		else if ((strcmp(var, "LocalBackupExt") == 0) && (lo != NULL))
			read_cfg_str(buffer, lo->backup_ext);
		else if ((strcmp(var, "LocalInsert") == 0) && (lo != NULL))
			read_cfg_bool(buffer, &(lo->einruecken));
		else if ((strcmp(var, "LocalKurzel") == 0) && (lo != NULL))
			read_cfg_str(buffer, lo->kurzel);
		else if ((strcmp(var, "LocalTab") == 0) && (lo != NULL))
			read_cfg_bool(buffer, &(lo->tab));
		else if ((strcmp(var, "LocalTabSize") == 0) && (lo != NULL))
			lo->tabsize = atoi(buffer);
		else if ((strcmp(var, "LocalUmbruch") == 0) && (lo != NULL))
			read_cfg_bool(buffer, &(lo->umbrechen));
		else if ((strcmp(var, "LocalUmbruchLineLen") == 0) && (lo != NULL))
			lo->lineal_len = atoi(buffer);
		else if ((strcmp(var, "LocalUmbruchIns") == 0) && (lo != NULL))
			read_cfg_bool(buffer, &(lo->format_by_paste));
		else if ((strcmp(var, "LocalUmbruchLoad") == 0) && (lo != NULL))
			read_cfg_bool(buffer, &(lo->format_by_load));
		else if ((strcmp(var, "LocalUmbruchAt") == 0) && (lo != NULL))
		{
			read_cfg_str(buffer, lo->umbruch_str);
			str2set(lo->umbruch_str, lo->umbruch_set);
		}
		else if ((strcmp(var, "LocalUmbruchShow") == 0) && (lo != NULL))
			read_cfg_bool(buffer, &(lo->show_end));
		else if ((strcmp(var, "LocalWordSet") == 0) && (lo != NULL))
		{
			read_cfg_str(buffer, lo->wort_str);
			str2set(lo->wort_str,lo->wort_set);
		}

		/* Makro */
		else if (strcmp(var, "Makro") == 0)
			set_makro_str(buffer);

		/* Marken */
		else if (strcmp(var, "Marke") == 0)
		{
			read_cfg_str(buffer, filename);
			d = sscanf(buffer + strlen(filename) + 3, "%hd \"%[^\"]\" %ld %hd", &i, tmp, &y, &x);
			if (d != 4)		/* altes Format (<4.10) ohne " um tmp */
				sscanf(buffer + strlen(filename) + 3, "%hd %s %ld %hd", &i, tmp, &y, &x);
			set_marke(i, tmp, filename, y, x);
		}

		/* offenen Dateien */
		else if (strcmp(var, "OpenPrj") == 0)
		{
			read_cfg_str(buffer, filename);
			insert_poslist(arglist, filename, 0, 0);
		}
		else if (strcmp(var, "OpenText") == 0)
		{
			read_cfg_str(buffer, filename);
			sscanf(buffer + strlen(filename) + 3, "%ld %hd", &y, &x);
			insert_poslist(arglist, filename, x, y);
		}

		/* Drucker */
		else if (prn_get_cfg(var, buffer))
			/* do nothing */ ;

		/* Ersetzen */
		else if (strcmp(var, "ReplaceBox") == 0)
			sscanf(buffer, "%hd %hd", &rp_box_x, &rp_box_y);
		else if (strcmp(var, "ReplaceMode") == 0)
			r_modus = atoi(buffer);
		else if (strcmp(var, "ReplaceStr") == 0)
			read_cfg_str(buffer, r_str);
		else if (strcmp(var, "ReplaceHistory") == 0)
		{
			read_cfg_str(buffer, tmp);
			for (i = 0; i < HIST_ANZ; i++)
				if (r_history[i][0] == EOS)
					break;
			if (i < HIST_ANZ)
				strcpy(r_history[i], tmp);
		}
		else if (strcmp(var, "ReplaceUmlautFrom") == 0)
			umlaut_from = atoi(buffer);
		else if (strcmp(var, "ReplaceUmlautTo") == 0)
			umlaut_to = atoi(buffer);

		/* Suchen */
		else if (strcmp(var, "SearchFileMask") == 0)
			read_cfg_str(buffer, ff_mask);
		else if (strcmp(var, "SearchFileRek") == 0)
			read_cfg_bool(buffer, &ff_rekursiv);
		else if (strcmp(var, "SearchDown") == 0)
			read_cfg_bool(buffer, &s_vorw);
		else if (strcmp(var, "SearchGlobal") == 0)
			read_cfg_bool(buffer, &s_global);
		else if (strcmp(var, "SearchGrkl") == 0)
			read_cfg_bool(buffer, &s_grkl);
		else if (strcmp(var, "SearchHistory") == 0)
		{
			read_cfg_str(buffer, tmp);
			for (i = 0; i < HIST_ANZ; i++)
				if (s_history[i][0] == EOS)
					break;
			if (i < HIST_ANZ)
				strcpy(s_history[i], tmp);
		}
		else if (strcmp(var, "SearchQuant") == 0)
			read_cfg_bool(buffer, &s_quant);
		else if (strcmp(var, "SearchRound") == 0)
			read_cfg_bool(buffer, &s_round);
		else if (strcmp(var, "SearchWord") == 0)
			read_cfg_bool(buffer, &s_wort);
		else if (strcmp(var, "SearchStr") == 0)
			read_cfg_str(buffer, s_str);

		/* SE-Protokoll */
		else if (strcmp(var, "SESave") == 0)
			read_cfg_bool(buffer, &se_autosave);
		else if (strcmp(var, "SESearch") == 0)
			read_cfg_bool(buffer, &se_autosearch);
		else if (strcmp(var, "SEIgnoreClose") == 0)
			read_cfg_bool(buffer, &se_ignoreclose);
		else if (strcmp(var, "SEShellName") == 0)
		{
			read_cfg_str(buffer, tmp);
			for (i = 0; i < SHELLANZ; i++)
				if (se_shells[i].name[0] == EOS)
					break;
			if (i < SHELLANZ)
				strcpy(se_shells[i].name, tmp);
		}

		/* Fensterfont */
		else if (strcmp(var, "WinFontID") == 0)
			font_id = atoi(buffer);
		else if (strcmp(var, "WinFontSize") == 0)
			font_pts = atoi(buffer);

		/* Fensterposition */
		else if (strcmp(var, "Window") == 0)
		{
			short	class;
			GRECT	size;

			sscanf(buffer, "%hd %hd %hd %hd %hd",
				 &class, &size.g_x, &size.g_y, &size.g_w, &size.g_h);
			add_winlist(class, &size);
		}


		/* Unbekannte Zeile */
		else
		{
			if (strlen(var) > 28)
				var[28] = EOS;
			snote(1, 0, WRONGINF, var);
		}
	}
	else
	{
		if (strlen(zeile) > 28)
			zeile[28] = EOS;
		snote(1, 0, WRONGINF, zeile);
	}
}

void option_load(POSENTRY **list)
{
	PATH	tmp;

	if (!get_cfg_path())					/* keine qed.cfg gefunden */
		return;

	fd = fopen(cfg_path, "r");
	if (fd != NULL)
	{
		/* 1. Zeile auf ID checken */
		fgets(buffer, (short)sizeof(buffer), fd);
		if (strncmp(buffer, "ID=qed", 6) == 0)
		{
			while (fgets(buffer, (short)sizeof(buffer), fd) != NULL)
			{
				if (buffer[strlen(buffer) - 1] == '\n')
					buffer[strlen(buffer) - 1] = EOS;
				parse_line(list, buffer);
			}
		}
		else
		{
			/* Zeile kurzhacken */
			if (strlen(buffer) > 28)
				buffer[28] = EOS;
			snote(1, 0, WRONGINF, buffer);
		}
		fclose(fd);
		fd = NULL;

		/* Farb-abhÑngige Parameter laden */
		split_filename(cfg_path, tmp, NULL);
		strcat(tmp, col_name);
		fd = fopen(tmp, "r");
		if (fd != NULL)
		{
			/* 1. Zeile auf ID checken */
			fgets(buffer, (short)sizeof(buffer), fd);
			if (strncmp(buffer, "ID=qed color configuration", 26) == 0)
			{
				while (fgets(buffer, (short)sizeof(buffer), fd) != NULL)
				{
					if (buffer[strlen(buffer) - 1] == '\n')
						buffer[strlen(buffer) - 1] = EOS;
					parse_line(list, buffer);
				}
			}
			else
			{
				/* Zeile kurzhacken */
				if (strlen(buffer) > 28)
					buffer[28] = EOS;
				snote(1, 0, WRONGINF, buffer);
			}

			fclose(fd);
			fd = NULL;

			set_drawmode();

		}

		/* Auflîsungs-abhÑngige Parameter laden */
		split_filename(cfg_path, tmp, NULL);
		strcat(tmp, dsp_name);
		fd = fopen(tmp, "r");
		if (fd != NULL)
		{
			/* 1. Zeile auf ID checken */
			fgets(buffer, (short)sizeof(buffer), fd);
			if (strncmp(buffer, "ID=qed display configuration", 28) == 0)
			{
				while (fgets(buffer, (short)sizeof(buffer), fd) != NULL)
				{
					if (buffer[strlen(buffer) - 1] == '\n')
						buffer[strlen(buffer) - 1] = EOS;
					parse_line(list, buffer);
				}
			}
			else
			{
				/* Zeile kurzhacken */
				if (strlen(buffer) > 28)
					buffer[28] = EOS;
				snote(1, 0, WRONGINF, buffer);
			}

			fclose(fd);
			fd = NULL;

			set_drawmode();

		}

		/* Zum Schluû noch 'pdlg.qed' */
		prn_get_cfg("PdlgRead", cfg_path);
	}
}


/******************************************************************************/
/* Dateioperation: Speichern																	*/
/******************************************************************************/
void write_cfg_str(char *var, char *value)
{
	if (strchr(value, '\"') != NULL)	/* " in value fÅhrt zu \" in der Datei */
	{
		short	len, i;

		fprintf(fd, "%s=\"", var);
		len = (short)strlen(value);
		for (i = 0; i < len; i++)
		{
			if (value[i] == '\"')
				fputc('\\', fd);
			fputc(value[i], fd);
		}
		fprintf(fd, "\"\n");
	}
	else
		fprintf(fd, "%s=\"%s\"\n", var, value);
}

void write_cfg_int(char *var, short value)
{
	fprintf(fd, "%s=%d\n", var, value);
}

void write_cfg_long(char *var, long value)
{
	fprintf(fd, "%s=%ld\n", var, value);
}

void write_cfg_bool(char *var, bool value)
{
	fprintf(fd, "%s=%s\n", var, value ? "TRUE" : "FALSE");
}

static void save_open_text(TEXTP t_ptr)
{
	WINDOWP	w = get_window(t_ptr->link);

	if (!t_ptr->namenlos)
	{
		if (w->class == CLASS_EDIT)
		{
			fprintf(fd, "OpenText=\"%s\" %ld %d\n", t_ptr->filename,
				t_ptr->ypos,
				bild_pos(t_ptr->xpos, t_ptr->cursor_line, t_ptr->loc_opt->tab, t_ptr->loc_opt->tabsize));
		}
		if (w->class == CLASS_PROJEKT)
			write_cfg_str("OpenPrj", t_ptr->filename);
	}
}

void option_save(void)
{
	short	i, x;
	long	y;
	PATH	path;
	LOCOPTP	lo_opt;

	fd = fopen(cfg_path, "w");
	if (fd == NULL) {
		note(1, 0, WRITEERR);
		return;
	}

	/* ID zur identifizierung */
	fprintf(fd, "ID=qed %s\n", QED_VERSION);

	/* Autosave */
	write_cfg_bool("AutoSavePrj", as_prj);
	write_cfg_bool("AutoSavePrjAsk", as_prj_ask);
	write_cfg_int("AutoSavePrjMin", as_prj_min);
	write_cfg_bool("AutoSaveText", as_text);
	write_cfg_bool("AutoSaveTextAsk", as_text_ask);
	write_cfg_int("AutoSaveTextMin", as_text_min);

	/* BinÑr-Extionsions */
	for (i = 4; i < BIN_ANZ; i++)
		if (bin_extension[i][0] != EOS)
			write_cfg_str("BinExtension", bin_extension[i]);

	/* Defaultprojekt */
	write_cfg_str("DefaultPrj", def_prj_path);

	/* Fehlerzeilen */
	for (i = 0; i < FEHLERANZ; i++)
	{
		if (error[i][0] != EOS)
			write_cfg_str("Error", error[i]);
	}

	/* Globales */
	write_cfg_bool("GlobalAutosaveCfg", save_opt);
	write_cfg_bool("GlobalSaveWindows", save_win);
	write_cfg_int ("GlobalBinLineLen", bin_line_len);
	write_cfg_bool("GlobalBlinkCursor", blinking_cursor);
	write_cfg_bool("GlobalCtrlBlock", ctrl_mark_mode);
	write_cfg_bool("GlobalEmuKlammer", emu_klammer);
	write_cfg_bool("GlobalFtoDesk", f_to_desk);
	write_cfg_bool("GlobalGEMClip", clip_on_disk);
	write_cfg_bool("GlobalOlga", olga_autostart);
	write_cfg_bool("GlobalOverwrite", overwrite);
	write_cfg_int ("GlobalTransSize", transfer_size);
	write_cfg_bool("GlobalWindCycle", wind_cycle);
	write_cfg_bool("GlobalSyntaxActive", syntax_active);

	write_cfg_bool("SyntaxSetSameName", syntax_setsamename);

	/* Hilfeprogramm */
	write_cfg_str("HelpProgram", helpprog);

	/* Klammerpaare */
	write_cfg_str("KlammerAuf", klammer_auf);
	write_cfg_str("KlammerZu", klammer_zu);

	/* Lokales */
	for (i = 0; i < LOCAL_ANZ; i++)
	{
		lo_opt = &local_options[i];
		if (lo_opt->muster[0] != EOS)
		{
			write_cfg_str("LocalBegin", lo_opt->muster);
			write_cfg_bool("LocalBackup", lo_opt->backup);
			write_cfg_str("LocalBackupExt", lo_opt->backup_ext);
			write_cfg_bool("LocalInsert", lo_opt->einruecken);
			write_cfg_str("LocalKurzel", lo_opt->kurzel);
			write_cfg_bool("LocalTab", lo_opt->tab);
			write_cfg_int("LocalTabSize", lo_opt->tabsize);
			write_cfg_bool("LocalUmbruch", lo_opt->umbrechen);
			write_cfg_int("LocalUmbruchLineLen", lo_opt->lineal_len);
			write_cfg_bool("LocalUmbruchIns", lo_opt->format_by_paste);
			write_cfg_bool("LocalUmbruchLoad", lo_opt->format_by_load);
			write_cfg_str("LocalUmbruchAt", lo_opt->umbruch_str);
			write_cfg_bool("LocalUmbruchShow", lo_opt->show_end);
			write_cfg_str("LocalWordSet", lo_opt->wort_str);
			write_cfg_str("LocalEnd", lo_opt->muster);
		}
	}

	/* Makros */
	for (i = 0; i < MAKRO_ANZ; i++)
	{
		if (get_makro_str(i, buffer))
			fprintf(fd, "Makro=%s\n", buffer);
	}

	/* Marken */
	for (i = 0; i < MARKEN_ANZ; i++)
	{
		if (get_marke(i, path, buffer, &y, &x))
			fprintf(fd, "Marke=\"%s\" %d \"%s\" %ld %d\n", buffer, i, path, y, x);
	}

	/* Drucker */
	prn_save_cfg(cfg_path);

	/* Ersetzen */
	write_cfg_int("ReplaceMode", r_modus);
	write_cfg_str("ReplaceStr", r_str);
	for (i = 0; i < HIST_ANZ; i++)
	{
		if (r_history[i][0] != EOS)
			write_cfg_str("ReplaceHistory", r_history[i]);
	}
	write_cfg_int("ReplaceUmlautFrom", umlaut_from);
	write_cfg_int("ReplaceUmlautTo", umlaut_to);

	/* Suchen */
	write_cfg_str("SearchFileMask", ff_mask);
	write_cfg_bool("SearchFileRek", ff_rekursiv);
	write_cfg_bool("SearchDown", s_vorw);
	write_cfg_bool("SearchGlobal", s_global);
	write_cfg_bool("SearchGrkl", s_grkl);
	for (i = 0; i < HIST_ANZ; i++)
	{
		if (s_history[i][0] != EOS)
			write_cfg_str("SearchHistory", s_history[i]);
	}


	write_cfg_bool("SearchQuant", s_quant);
	write_cfg_bool("SearchRound", s_round);
	write_cfg_bool("SearchWord", s_wort);
	write_cfg_str("SearchStr", s_str);

	/* SE-Protokoll */
	write_cfg_bool("SESave", se_autosave);
	write_cfg_bool("SESearch", se_autosearch);
	write_cfg_bool("SEIgnoreClose", se_ignoreclose);
	for (i = 0; i < SHELLANZ - 1; i++)
	{
		if (se_shells[i].name[0] != EOS)
			write_cfg_str("SEShellName", se_shells[i].name);
	}

	fclose(fd);
	fd = NULL;

	/* Farb-abhÑngige Parameter sichern */
	split_filename(cfg_path, path, NULL);
	strcat(path, col_name);
	fd = fopen(path, "w");
	if (fd != NULL )
	{
		fprintf(fd, "ID=qed color configuration\n");

		/* Farb-Infos */
		write_cfg_int("GlobalBgColor", bg_color);
		write_cfg_int("GlobalFgColor", fg_color);
		write_cfg_int("GlobalBlockBgColor", bg_block_color);
		write_cfg_int("GlobalBlockFgColor", fg_block_color);

		fclose(fd);
		fd = NULL;
	}

	/* save resolution dependent parameters */
	/* this is a bit hacked of course - but I am planning to replace the whole config stuff
	   by one global config file ASAP */
	{
		bool copied_ok = FALSE;

		/* if the window settings shall not be saved, remove
		   the font settings from the file. These will be written, even if
		   save_win == FALSE. In this case The font settings are appended to the file. */

		if( !save_win )
		{
			FILE *tf;
			PATH tmp;
			char	var[30], *p;

			/* copy all lines except "WinFontID" and "WinFontSize" to a tempfile.
			   remove the old file and rename the tempfile to the new settings file. */
			split_filename(cfg_path, path, NULL);
			strcpy( tmp, path );
			strcat( path, dsp_name );
			strcat( tmp, "temp.qed" );
			fd = fopen( path, "r" );
			if (fd == NULL )
			{
				/* there is no such file -> write empty one */
				fd = fopen(path, "w" );
				if (fd != NULL ) {
						fprintf(fd, "ID=qed display configuration\n");
						copied_ok = TRUE;
						fclose(fd);
				}
			} else {
				tf = fopen( tmp, "w" );
				if( tf != NULL ) {
					while (fgets(buffer, (short)sizeof(buffer), fd) != NULL)
					{
						p = strchr(buffer, '=');
						if (p != NULL)
						{
							strncpy(var, buffer, p-buffer);
							var[p-buffer] = EOS;
							if( strcmp( var, "WinFontID" ) == 0 ||
							    strcmp( var, "WinFontSize" ) == 0 )
								continue;
						}
						if( strcmp( buffer, "ID=qed display configuration\n" ) == 0 )
							copied_ok = TRUE;
						fputs( buffer, tf );
					}
					fclose( tf );
					tf = NULL;
				}
				fclose(fd);
				fd = NULL;

				if( copied_ok )
				{
						remove( path );
						rename( tmp, path );
				} else {
						remove( tmp );
				}
			}
		}

		split_filename(cfg_path, path, NULL);
		strcat(path, dsp_name);
		fd = fopen(path, save_win ? "w" : "a" );
		if (fd != NULL )
		{
			if( save_win )
			{
				fprintf(fd, "ID=qed display configuration\n");

				/* geladene Dateien */
				do_all_text(save_open_text);

				/* Pos. der Replace-Ask-Box */
				fprintf(fd, "ReplaceBox=%d %d\n", rp_box_x, rp_box_y);

				/* Fensterposition */
				save_winlist(fd);
			}

			if( save_win || copied_ok )
			{
				/* Fensterfont */
				write_cfg_int("WinFontID", font_id);
				write_cfg_int("WinFontSize", font_pts);
			}

			fclose(fd);
			fd = NULL;
		}
	}

	split_filename(cfg_path, path, NULL);
	send_avpathupdate (path);
}
