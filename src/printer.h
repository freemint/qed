#ifndef _qed_printer_h_
#define _qed_printer_h_

typedef struct
{
	short	wp_mode;
	short	wp_s_len;
	short	wp_z_len;
	PATH	wp_treiber;
	bool	wp_nlq;
	bool	vorschub;
	bool	pruef_prn;

	bool	use_gdos;
	bool	use_pdlg;
	short	font_id;
	short	font_pts;	

	bool	num_zeilen;
	bool	num_seiten;
	short	rand_len;

	void	*pdlg;		/* PRN_SETTINGS */
	
	/* momentane Einstellungen, werden nicht gesichert! */
	bool	ausdruck;	/* FALSE: normale Konfig, TRUE: vor Ausdruck */
	bool	block;		/* Bei start=TRUE: Block oder alles */
	_WORD	handle;		/* VDI/GEMDOS-Handle */
	short	height;		/* H�he der Druckseite */
} PRN_CFG;


/* Konfiguration: prn_cfg.c */
extern PRN_CFG	*prn;

extern void		prn_cfg_dial	(void);
extern bool		prn_start_dial	(bool *block);

extern void		prn_save_cfg	(char	*cfg_file);
extern bool		prn_get_cfg		(char *var, char *buffer);

extern void		init_printer	(void);
extern void		term_printer	(void);


/* Ausgabe: prn_out.c */
extern void		blk_drucken		(char *name, TEXTP t_ptr);
extern void		txt_drucken		(char *name, TEXTP t_ptr);

/*
 * from prn_cfg.c
*/
bool open_printer	(void);
void close_printer	(void);

#endif
