#ifndef _qed_file_h_
#define _qed_file_h_

extern void 	open_error		(char *filename, short error_code);

extern short 		load_from_fd	(short fd, char *name, RINGP t, bool verbose, bool *null_byte, long size);
extern short 		load_datei		(char *name, RINGP t, bool verbose, bool *null_byte);
extern short		load				(TEXTP t_ptr, bool verbose);
extern short		infoload			(char *name, long *bytes, long *lines);

extern short 		save_to_fd		(short fd, char *name, RINGP t, bool verbose);
extern short		save_datei		(char *name, RINGP t, bool verbose);
extern short		save				(TEXTP t_ptr);
extern short		save_as			(TEXTP t_ptr, char *name);
extern bool		save_new			(char *name, char *mask, char *title);

extern bool		select_single 	(char *filename, char *mask, char *title);
extern bool		select_path 	(char *pathname, char *title);
extern void 	select_multi	(bool binary);

extern void 	store_path		(char *path);

extern void		init_file		(void);
extern void		term_file		(void);

#endif
