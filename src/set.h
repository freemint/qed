#ifndef _qed_set_h_
#define _qed_set_h_

extern short		setfree	(SET set);				/* kleinste freie Nummer */
extern short		setmax	(SET set);				/* gr”žte belegte Nummer */
extern short		setmin	(SET set);				/* kleinste belegte Nummer */
extern void		setcpy	(SET set1, SET set2);
extern void		setall	(SET set);
extern void		setclr	(SET set);
extern void		setnot	(SET set);
extern void		setand	(SET set1, SET set2);
extern void		setor		(SET set1, SET set2);
extern void		setxor	(SET set1, SET set2);
extern void		setincl	(SET set, short elt);
extern void		setexcl	(SET set, short elt);
extern void		setchg	(SET set, short elt);
extern bool		setin		(SET set, short elt);
extern bool		setcmp	(SET set1, SET set2);
extern short		setcard	(SET set);
extern void		str2set	(char *str, SET set);

#endif
