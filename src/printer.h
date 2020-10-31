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
	
	/* current settings; these are not saved! */
	bool	ausdruck;	/* FALSE: normal config, TRUE: before printout */
	bool	block;		/* at start=TRUE: Block or everything */
	_WORD	handle;		/* VDI/GEMDOS-handle */
	short	height;		/* height of print page */
} PRN_CFG;


/* from prn_cfg.c */
extern PRN_CFG	*prn;

void		prn_cfg_dial	(void);
bool		prn_start_dial	(bool *block);

void		prn_save_cfg	(char	*cfg_file);
bool		prn_get_cfg		(char *var, char *buffer);

void		init_printer	(void);
void		term_printer	(void);

bool open_printer	(void);
void close_printer	(void);


/* from prn_out.c */
void		blk_drucken		(char *name, TEXTP t_ptr);
void		txt_drucken		(char *name, TEXTP t_ptr);

#endif
