/* QED - hl.c:
 * Dieser Quelltext bildet das Interface zwischen den Datentypen von QED
 * und den Syntax-Highlighting-Funktionen aus highlight.c
 *
 * Autor: Heiko Achilles
 */
 
#include <support.h>

#include "global.h"
#include "ausgabe.h"
#include "edit.h"
#include "hl.h"
#include "memory.h"
#include "options.h"
#include "rsc.h"
#include "text.h"
#include "window.h"

#define SYN_FILENAME "syntax.cfg"

static void hl_error_callback(HL_ERRTYPE err, int linenr)
{
	switch (err)
	{
		case E_HL_MEMORY:   note(1,0, SYN_MEM);               break;
		case E_HL_TOFROM:   inote(1,0, SYN_TOFROM, linenr);   break;
		case E_HL_MIXED:    inote(1,0, SYN_MIXED, linenr);    break;
		case E_HL_SYNTAX:   inote(1,0, SYN_SYNTAX, linenr);   break;
		case E_HL_WRONGVAL: inote(1,0, SYN_WRONGVAL, linenr); break;
	}
}

void hl_init(void)
{
	Hl_Init(hl_error_callback);
}

void hl_exit(void)
{
	Hl_Exit();
}


/* Highlight-Cache fr aktuelle Cursor-Zeile updaten
 * (muž jedesmal aufgerufen werden, wenn die Zeile ver„ndert wird)
 */
void hl_update(TEXTP t_ptr)
{
	if (!syntax_active)
		return;
	if (Hl_Update(t_ptr->text.hl_anchor,
	                        t_ptr->cursor_line->hl_handle,
	                        TEXT(t_ptr->cursor_line), TRUE) > 1)
		make_chg(t_ptr->link,TOTAL_CHANGE,0);
}

/* Highlight-Cache fr einen kompletten Text updaten
 * z.B. wenn Tabs durch Leerzeichen ersetzt werden,
 * _nicht_, wenn sich der Texttyp „ndert z.B. .txt -> .c,
 * dafr ist hl_change_text_type() zust„ndig.
 */
void hl_update_text(TEXTP t_ptr)
{
	ZEILEP curr_zeile = t_ptr->text.head.nachf;
	
	if (!syntax_active)
		return;
		
	while (curr_zeile != &t_ptr->text.tail)
	{
		Hl_Update(t_ptr->text.hl_anchor,
	                  curr_zeile->hl_handle,
	                  TEXT(curr_zeile), FALSE);
	  curr_zeile = curr_zeile->nachf;
		
	}
	
	make_chg(t_ptr->link,TOTAL_CHANGE,0);
}

static void textredraw(TEXTP t_ptr)
{
	make_chg(t_ptr->link,TOTAL_CHANGE,0);
}

/* alle Texte komplett updaten; wenn Žnderungen im
 * Syntaxdialog vorgenommen worden sind, mssen alle Texte
 * an evtl. Žnderungen angepasst werden.
 */
void hl_update_all(void)
{
	if (!syntax_active)
		return;		
	do_all_text(hl_update_text);
	restore_edit();
}


/* Alle Texte komplett freigeben; wenn Syntax-Highlighting
 * ausgeschaltet wird
 */
void hl_disable(void)
{
	if (!syntax_active)
		return;		
	do_all_text(hl_free);
	do_all_text(textredraw);
	restore_edit();
}

/* alle Texte komplett initialisieren; wenn Syntax-Highlighting
 * (wieder) aktiviert wird.
 */
void hl_enable(void)
{
	if (!syntax_active)
		return;		
	do_all_text(hl_init_text);
	do_all_text(textredraw);
	restore_edit();
}

/* Highlight-Cache fr eine Zeile updaten
 */
void hl_update_zeile(RINGP r_ptr, ZEILEP z_ptr)
{
	if (!syntax_active)
		return;
		
	Hl_Update(r_ptr->hl_anchor,
	                z_ptr->hl_handle,
	                TEXT(z_ptr), TRUE);
}

/* Highlight-Cache fr einen Block updaten
 */
void hl_update_block(RINGP r_ptr, ZEILEP first, ZEILEP last)
{
	ZEILEP curr = first;
	if (!syntax_active)
		return;
		
	for (;;)
	{
  	Hl_Update(r_ptr->hl_anchor,
	                  curr->hl_handle,
	                  TEXT(curr), FALSE);
		if (curr == last)
		{
			if (curr->nachf)
		  	Hl_Update(r_ptr->hl_anchor,
			                  curr->nachf->hl_handle,
			                  TEXT(curr->nachf), TRUE);
			return;
		}
		curr = curr->nachf;
	}
}

/* Highlight-Cache fr eine Zeile entfernen
 * Muž immer aufgerufen werden, wenn eine Zeile gel”scht wird
 * (z.B. wenn tempor„re RINGs benutzt werden), sonst Speicherlecks!
 * Oder natrlich die anderen Funktionen (s.u.), um Syntax-Cache zu
 * entfernen.
 */
void hl_remove(RINGP r_ptr, ZEILEP z_ptr)
{
	if (!syntax_active)
		return;
		
	if (z_ptr->vorg)
		Hl_RemoveLine(r_ptr->hl_anchor,
	  	                  z_ptr->vorg->hl_handle, 
	  	                  &z_ptr->hl_handle,
	  	                  TRUE);
	else
		Hl_RemoveLine(r_ptr->hl_anchor,
	  	                  NULL,
	  	                  &z_ptr->hl_handle,
	  	                  TRUE);
}

/* Highlight-Cache fr eine Zeile einfgen
 */
void hl_insert(RINGP r_ptr, ZEILEP z_ptr)
{
	if (!syntax_active)
		return;
		
	if (z_ptr->hl_handle)
		hl_remove(r_ptr, z_ptr);
		
	if (z_ptr->vorg)
		Hl_InsertLine(r_ptr->hl_anchor,
	  	                  z_ptr->vorg->hl_handle,
	    	                TEXT(z_ptr),
		                    &z_ptr->hl_handle,
		                    TRUE);
	else
		Hl_InsertLine(r_ptr->hl_anchor,
	  	                  NULL,
	    	                TEXT(z_ptr),
		                    &z_ptr->hl_handle,
		                    TRUE);
	                        
}

/* wie hl_remove(), nur daž kein Update der nachfolgenden
 * Zeilen durchgefhrt wird (fr Block einfgen/l”schen)
 */
static void hl_remove_noupdate(RINGP r_ptr, ZEILEP z_ptr)
{
	if (z_ptr->vorg)
		Hl_RemoveLine(r_ptr->hl_anchor,
	  	                  z_ptr->vorg->hl_handle, 
	  	                  &z_ptr->hl_handle,
	  	                  TRUE);
	else
		Hl_RemoveLine(r_ptr->hl_anchor,
	  	                  NULL,
	  	                  &z_ptr->hl_handle,
	  	                  TRUE);
}

/* wie hl_insert(), nur daž kein Update der nachfolgenden
 * Zeilen durchgefhrt wird (fr Block einfgen/l”schen)
 */
static void hl_insert_noupdate(RINGP r_ptr, ZEILEP z_ptr)
{
	if (z_ptr->hl_handle)
		hl_remove_noupdate(r_ptr, z_ptr);
		
	if (z_ptr->vorg)
		Hl_InsertLine(r_ptr->hl_anchor,
	  	                  z_ptr->vorg->hl_handle,
	    	                TEXT(z_ptr),
		                    &z_ptr->hl_handle,
		                    FALSE);
	else
		Hl_InsertLine(r_ptr->hl_anchor,
	  	                  NULL,
	    	                TEXT(z_ptr),
		                    &z_ptr->hl_handle,
		                    FALSE);
	                        
}

/* Syntax-Highlighting fr Block einfgen
 */
void hl_insert_block(RINGP r_ptr, ZEILEP first, ZEILEP last)
{
	ZEILEP curr = first;

	if (!syntax_active)
		return;
		
	for(;;)
	{
		hl_insert_noupdate(r_ptr, curr);
		if (curr == last)
		{
			if (curr->nachf)
		  	Hl_Update(r_ptr->hl_anchor,
			                  curr->nachf->hl_handle,
			                  TEXT(curr->nachf), TRUE);
			return;
		}
		curr = curr->nachf;
	}
}

/* Syntax-Highlighting fr Block entfernen
 */
void hl_remove_block(RINGP r_ptr, ZEILEP first, ZEILEP last)
{
	ZEILEP curr = first;
	bool updateit = FALSE;

	if (!syntax_active)
		return;
		
	for (;;)
	{
		if (curr == last)
			updateit = TRUE;
		if (first->vorg)
			Hl_RemoveLine(r_ptr->hl_anchor,
		  	                  first->vorg->hl_handle, 
		  	                  &curr->hl_handle, updateit);
		else
			Hl_RemoveLine(r_ptr->hl_anchor,
		  	                  NULL,
		  	                  &curr->hl_handle, updateit);
		if (curr == last)
			return;
		curr = curr->nachf;
	}
}

/* Syntax-Highlighting fr kompletten Text erstellen */
void hl_init_text(TEXTP t_ptr)
{
	PATH tfilename, ext;
	ZEILEP curr_zeile = t_ptr->text.head.nachf;

	if (!syntax_active)
	{
		t_ptr->text.hl_anchor = NULL;
		return;
	}
		
	split_filename(t_ptr->filename, NULL, tfilename);
	if ((t_ptr->text.hl_anchor = Hl_New(tfilename, 0)) == NULL) /* erst mal nachsehen, ob ggf ein ganzer Dateiname */
	{
		split_extension(tfilename, NULL, ext);      /* wenn nicht, probieren wirs mit dem Extender */
		t_ptr->text.hl_anchor = Hl_New(ext, 0);       /* wenn das auch NULL zurckliefert, ist das o.k. */
	}
	t_ptr->text.head.hl_handle = NULL;
	
	while (curr_zeile != &t_ptr->text.tail)
	{
		Hl_InsertLine(t_ptr->text.hl_anchor,
		               curr_zeile->vorg->hl_handle,
		               TEXT(curr_zeile),
		               &curr_zeile->hl_handle,
		               TRUE);
		curr_zeile = curr_zeile->nachf;
	}
}

/* Syntax-Highlighting fr kompletten Text entfernen */
void hl_free(TEXTP t_ptr)
{
	if (!syntax_active)
		return;
		
	hl_remove_block(&t_ptr->text, t_ptr->text.head.nachf, t_ptr->text.tail.vorg);
	Hl_Free(t_ptr->text.hl_anchor);
}

static PATH hl_filename;

/* .syn-Datei lesen */
bool hl_read_syn(void)
{
	strcpy(hl_filename, SYN_FILENAME);
	if (get_config_file(hl_filename))
		return Hl_ReadSyn(hl_filename, gl_planes);
	
	hl_filename[0] = EOS;
	return FALSE;
}

/* .syn-Datei schreiben */
bool hl_write_syn(void)
{
	if (hl_filename[0] == EOS)
		return FALSE;
	return Hl_WriteSyn(hl_filename);
}

/* Syntax-Highlighting-Cache fr eine Zeile abfragen.
 * Anders als Hl_GetLine() liefert hl_get_zeile() nicht
 * NULL zurck, wenn es fr diesen Texttyp kein Syntax-Highlighting
 * gibt, sondern eine Syntax-Cache-Zeile ohne Farb/Attributinformationen.
 */
HL_LINE hl_get_zeile(ZEILEP z)
{
	HL_LINE ret;
	static HL_ELEM linefake[ 4 * (MAX_LINE_LEN / 255 + 2) ];
	HL_ELEM *linefake_p = linefake;
	int len;
	
	if (syntax_active && (ret = Hl_GetLine(z->hl_handle)) != NULL)
		return ret;

	len = z->len;
	while (len-255 > 0)
	{
		len -= 255;
		*(linefake_p++) = (HL_ELEM) 0;
		*(linefake_p++) = (HL_ELEM) 255;
	}
	*(linefake_p++) = (HL_ELEM) 0;
	*(linefake_p++) = (HL_ELEM) len;
	*linefake_p = HL_CACHEEND;
	return linefake;
}

/* Žndert den Typ eines Textes, z.B. .txt->.c; in extension
 * muž der neue Extender bergeben werden.
 */
void hl_change_text_type(TEXTP t_ptr, char *extension)
{
	if (!syntax_active)
		return;
		
	Hl_Free(t_ptr->text.hl_anchor);
	t_ptr->text.hl_anchor = NULL;
	hl_init_text(t_ptr);
}







