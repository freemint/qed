#ifndef _qed_ausgabe_h_
#define _qed_ausgabe_h_

/* Liefert die interne Position */
extern short		inter_pos		(short x, ZEILEP a, bool tab, short tab_size);

/* Liefert die L„nge auf dem Bild */
extern short		bild_len			(ZEILEP a, bool tab, short tab_size);

/* Liefert die Position auf dem Bild */
extern short		bild_pos			(short x, ZEILEP a, bool tab, short tab_size);

extern void		fill_area		(short x, short y, short w, short h, short color);
extern short		out_s				(short x, short y, short w, char *str);
extern short		out_sb			(short x, short y, short w, char *str);

extern void		head_out			(WINDOWP window, TEXTP t_ptr);
extern void		line_out			(WINDOWP window, TEXTP t_ptr, short wy);
extern void		bild_out			(WINDOWP window, TEXTP t_ptr);
extern void		bild_blkout		(WINDOWP window, TEXTP t_ptr, long z1, long z2);
extern short		cursor_xpos		(TEXTP t_ptr, short pos);
extern void		cursor			(WINDOWP window, TEXTP t_ptr);

extern void 	set_drawmode	(void);

#endif
