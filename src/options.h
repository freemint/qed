#ifndef _qed_options_h_
#define _qed_options_h_

/* 
 * Autosave 
*/
extern bool	as_text, as_prj;						/* Autosave ? */
extern bool	as_text_ask, as_prj_ask;			/*  nachfragen? */
extern short	as_text_min, as_prj_min;			/*  Minuten */

extern void		set_autosave_options(void);

/* 
 * Globale Optionen 
*/
extern bool	clip_on_disk, wind_cycle, f_to_desk,
				save_opt, save_win, overwrite, blinking_cursor, ctrl_mark_mode,
				olga_autostart, emu_klammer;
extern bool syntax_active;
extern short	transfer_size, bin_line_len;
extern short	fg_color, bg_color;
extern short fg_block_color, bg_block_color;
extern PATH	helpprog;
#define BIN_ANZ	10
extern char	bin_extension[BIN_ANZ][MASK_LEN+1];

extern void		set_global_options	(void);
extern void		set_syntax_options	(void);

/*
 * Klammerpaare
*/
extern char	klammer_auf[],
				klammer_zu[];

extern void	set_klammer_options(void);


/* 
 * Lokale Optionen 
*/

/* Anzahl der lokalen Optionen */
#define LOCAL_ANZ	20
extern LOCOPT	local_options[LOCAL_ANZ];

extern void		set_local_options(void);


/* 
 * Datei 
*/
extern void 	write_cfg_str		(char *var, char *value);
extern void 	write_cfg_int		(char *var, short value);
extern void 	write_cfg_long		(char *var, long value);
extern void 	write_cfg_bool		(char *var, bool value);
extern void 	read_cfg_bool		(char *str, bool *val);
extern void 	read_cfg_str		(char *str, char *val);

extern void		option_load			(POSENTRY **arglist);
extern void 	option_save			(void);


extern void		init_default_var	(void);

#endif
