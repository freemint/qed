/* QED - hl.c:
 * Dieser Quelltext bildet das Interface zwischen den Datentypen von QED
 * und den Syntax-Highlighting-Funktionen aus highlight.c
 *
 * Autor: Heiko Achilles
 */
 
#include <dirent.h>
#include <support.h>
#include <assert.h>

#include "global.h"
#include "ausgabe.h"
#include "edit.h"
#include "hl.h"
#include "memory.h"
#include "options.h"
#include "rsc.h"
#include "text.h"
#include "window.h"
#include "av.h"

#define SYN_DIR "syntax"
#define SYNCFGNAME "syn_cfg.qed"

static void hl_error_callback( char *currfile, HL_ERRTYPE err, int linenr)
{
	char f[20];
	if( currfile )
		make_shortpath( currfile, f, 19 );
	else
		f[0] = EOS;
		
	switch (err)
	{
		case E_HL_MEMORY:   note(1,0, SYN_MEM);               break;
		case E_HL_TOFROM:   sinote(1,0, SYN_TOFROM, f, linenr );   break;
		case E_HL_MIXED:    sinote(1,0, SYN_MIXED, f, linenr );    break;
		case E_HL_SYNTAX:   sinote(1,0, SYN_SYNTAX, f, linenr );   break;
		case E_HL_WRONGVAL: sinote(1,0, SYN_WRONGVAL, f, linenr ); break;
		case E_HL_DUPTEXT:  sinote(1,0, SYN_DUPTEXT, f, linenr ); break;
		default:;
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


/* Highlight-Cache fr aktuelle Cursor-Zeile updaten
 * (mu jedesmal aufgerufen werden, wenn die Zeile verndert wird)
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

/* Highlight-Cache fr einen kompletten Text updaten
 * z.B. wenn Tabs durch Leerzeichen ersetzt werden,
 * _nicht_, wenn sich der Texttyp ndert z.B. .txt -> .c,
 * dafr ist hl_change_text_type() zustndig.
 */
void hl_update_text(TEXTP t_ptr)
{
	LINEP curr_zeile = t_ptr->text.head.next;
	
	if (!syntax_active)
		return;
		
	while (curr_zeile != &t_ptr->text.tail)
	{
		Hl_Update(t_ptr->text.hl_anchor,
	                  curr_zeile->hl_handle,
	                  TEXT(curr_zeile), FALSE);
	  curr_zeile = curr_zeile->next;
		
	}
	
	make_chg(t_ptr->link,TOTAL_CHANGE,0);
}

static void textredraw(TEXTP t_ptr)
{
	make_chg(t_ptr->link,TOTAL_CHANGE,0);
}

/* alle Texte komplett updaten; wenn nderungen im
 * Syntaxdialog vorgenommen worden sind, mssen alle Texte
 * an evtl. nderungen angepasst werden.
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

/* Highlight-Cache fr eine Zeile updaten
 */
void hl_update_zeile(RINGP r_ptr, LINEP z_ptr)
{
	if (!syntax_active)
		return;

	assert(r_ptr);
	assert(z_ptr);
		
	Hl_Update(r_ptr->hl_anchor,
	                z_ptr->hl_handle,
	                TEXT(z_ptr), TRUE);
}

/* Highlight-Cache fr einen Block updaten
 */
void hl_update_block(RINGP r_ptr, LINEP first, LINEP last)
{
	LINEP curr = first;
	if (!syntax_active)
		return;

	assert(r_ptr);
	assert(curr);
		
	for (;;)
	{
  	Hl_Update(r_ptr->hl_anchor,
	                  curr->hl_handle,
	                  TEXT(curr), FALSE);
		if (curr == last)
		{
			if (curr->next && curr->next->hl_handle)
		  	Hl_Update(r_ptr->hl_anchor,
			                  curr->next->hl_handle,
			                  TEXT(curr->next), TRUE);
			return;
		}
		curr = curr->next;
	}
}

/* Highlight-Cache fr eine Zeile entfernen
 * Mu immer aufgerufen werden, wenn eine Zeile gelscht wird
 * (z.B. wenn temporre RINGs benutzt werden), sonst Speicherlecks!
 * Oder natrlich die anderen Funktionen (s.u.), um Syntax-Cache zu
 * entfernen.
 */
void hl_remove(RINGP r_ptr, LINEP z_ptr)
{
	if (!syntax_active)
		return;

	assert(r_ptr);
	assert(z_ptr);
		
	if (z_ptr->prev)
		Hl_RemoveLine(r_ptr->hl_anchor,
	  	                  z_ptr->prev->hl_handle, 
	  	                  &z_ptr->hl_handle,
	  	                  TRUE);
	else
		Hl_RemoveLine(r_ptr->hl_anchor,
	  	                  NULL,
	  	                  &z_ptr->hl_handle,
	  	                  TRUE);
}

/* Highlight-Cache fr eine Zeile einfgen
 */
void hl_insert(RINGP r_ptr, LINEP z_ptr)
{
	if (!syntax_active)
		return;

	assert(r_ptr);
	assert(z_ptr);
		
	if (z_ptr->hl_handle)
		hl_remove(r_ptr, z_ptr);
		
	if (z_ptr->prev)
		Hl_InsertLine(r_ptr->hl_anchor,
	  	                  z_ptr->prev->hl_handle,
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

/* wie hl_remove(), nur da kein Update der nachfolgenden
 * Zeilen durchgefhrt wird (fr Block einfgen/lschen)
 */
static void hl_remove_noupdate(RINGP r_ptr, LINEP z_ptr)
{
	assert(z_ptr);

	if (z_ptr->prev)
		Hl_RemoveLine(r_ptr->hl_anchor,
	  	                  z_ptr->prev->hl_handle, 
	  	                  &z_ptr->hl_handle,
	  	                  TRUE);
	else
		Hl_RemoveLine(r_ptr->hl_anchor,
	  	                  NULL,
	  	                  &z_ptr->hl_handle,
	  	                  TRUE);
}

/* wie hl_insert(), nur da kein Update der nachfolgenden
 * Zeilen durchgefhrt wird (fr Block einfgen/lschen)
 */
static void hl_insert_noupdate(RINGP r_ptr, LINEP z_ptr)
{
	if (z_ptr->hl_handle)
		hl_remove_noupdate(r_ptr, z_ptr);
		
	if (z_ptr->prev)
		Hl_InsertLine(r_ptr->hl_anchor,
	  	                  z_ptr->prev->hl_handle,
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

/* Syntax-Highlighting fr Block einfgen
 */
void hl_insert_block(RINGP r_ptr, LINEP first, LINEP last)
{
	LINEP curr = first;

	if (!syntax_active)
		return;
		
	assert(r_ptr);
	assert(curr);

	for(;;)
	{
		hl_insert_noupdate(r_ptr, curr);
		if (curr == last)
		{
			if (curr->next && curr->next->hl_handle)
		  	Hl_Update(r_ptr->hl_anchor,
			                  curr->next->hl_handle,
			                  TEXT(curr->next), TRUE);
			return;
		}
		curr = curr->next;
	}
}

/* Syntax-Highlighting fr Block entfernen
 */
void hl_remove_block(RINGP r_ptr, LINEP first, LINEP last)
{
	LINEP curr = first;
	bool updateit = FALSE;

	if (!syntax_active)
		return;

	assert(r_ptr);
		
	for (;;)
	{
		if (curr == last)
			updateit = TRUE;
		if (first->prev)
			Hl_RemoveLine(r_ptr->hl_anchor,
		  	                  first->prev->hl_handle, 
		  	                  &curr->hl_handle, updateit);
		else
			Hl_RemoveLine(r_ptr->hl_anchor,
		  	                  NULL,
		  	                  &curr->hl_handle, updateit);
		if (curr == last)
			return;
		curr = curr->next;
	}
}

/* Syntax-Highlighting fr kompletten Text erstellen */
void hl_init_text(TEXTP t_ptr)
{
	PATH tfilename, ext;
	LINEP curr_zeile = t_ptr->text.head.next;

	if (!syntax_active)
	{
		t_ptr->text.hl_anchor = NULL;
		return;
	}
		
	split_filename(t_ptr->filename, NULL, tfilename);
	if ((t_ptr->text.hl_anchor = Hl_New(tfilename, 0)) == NULL) /* erst mal nachsehen, ob ggf ein ganzer Dateiname */
	{
		split_extension(tfilename, NULL, ext);      /* wenn nicht, probieren wirs mit dem Extender */
		t_ptr->text.hl_anchor = Hl_New(ext, 0);       /* wenn das auch NULL zurckliefert, ist das o.k. */
	}
	t_ptr->text.head.hl_handle = NULL;
	
	while (curr_zeile != &t_ptr->text.tail)
	{
		Hl_InsertLine(t_ptr->text.hl_anchor,
		               curr_zeile->prev->hl_handle,
		               TEXT(curr_zeile),
		               &curr_zeile->hl_handle,
		               TRUE);
		curr_zeile = curr_zeile->next;
	}
}

/* Syntax-Highlighting fr kompletten Text entfernen */
void hl_free(TEXTP t_ptr)
{
	if (!syntax_active)
		return;
		
	hl_remove_block(&t_ptr->text, t_ptr->text.head.next, t_ptr->text.tail.prev);
	Hl_Free(t_ptr->text.hl_anchor);
}

/* .syn-Datei lesen */
bool hl_read_syn(void)
{
	PATH syn_path;
	PATH curr_syn_file;
	FILENAME ext;
	
	DIR	*dh;
	struct dirent	*entry;

	/* read default syntax settings */
	strcpy( syn_path,SYN_DIR );
	if(!get_config_file( syn_path, TRUE ))
		return FALSE;

	dh = opendir(syn_path);
	if( dh <= (DIR*)NULL )
		return FALSE;
		
	if (dh < 0)
		return FALSE;
	for(;;)
	{
		entry = readdir(dh);
		if( !entry )
			break;

		split_extension( entry->d_name, NULL, ext );
		if( stricmp( ext, "syn" ) != 0 )
			continue;
		strcpy( curr_syn_file, syn_path );
		strcat( curr_syn_file, "\\" );
		strcat( curr_syn_file, entry->d_name );

		if( !Hl_ReadSyn( curr_syn_file, gl_planes, FALSE ))
		{
			closedir( dh );
			return FALSE;
		}	
	}
	closedir( dh );

	/* read user config */
	get_config_dir( syn_path );
	strcat( syn_path, SYNCFGNAME );
	if( file_exists( syn_path ))
		Hl_ReadSyn( syn_path, gl_planes, TRUE );
		
	return TRUE;
}

/* .syn-Datei schreiben */
bool hl_write_syn(void)
{
	PATH cfg_path;
	PATH tmp;

  /* write the default *.syn - files - only if explicitly compiled for this.
     in the normal user program, only the user syntax config file is written */
	#ifdef WRITE_SYN	
		if( !Hl_WriteSyn(NULL, FALSE))
			return FALSE;
		strcpy( cfg_path,SYN_DIR );
		if(get_config_file( cfg_path, TRUE ))
		{
			strcat( cfg_path, "\\" );
			send_avpathupdate( cfg_path );
		}		
		debug("*.syn files written\n");
	#endif
	/* and write an additional config file - to keep your own syntax settings */
	get_config_dir( cfg_path );
	strcat( cfg_path, SYNCFGNAME );
	
	if( !Hl_WriteSyn( cfg_path, TRUE ))
		return FALSE;
	split_filename( cfg_path, tmp, NULL );
	send_avpathupdate( tmp );		
	debug("syn_cfg.qed written\n");

	return TRUE;

}

/* Syntax-Highlighting-Cache fr eine Zeile abfragen.
 * Anders als Hl_GetLine() liefert hl_get_zeile() nicht
 * NULL zurck, wenn es fr diesen Texttyp kein Syntax-Highlighting
 * gibt, sondern eine Syntax-Cache-Zeile ohne Farb/Attributinformationen.
 */
HL_LINE hl_get_zeile(LINEP z)
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

/* ndert den Typ eines Textes, z.B. .txt->.c; in extension
 * mu der neue Extender bergeben werden.
 */
void hl_change_text_type(TEXTP t_ptr, char *extension)
{
	if (!syntax_active)
		return;
		
	Hl_Free(t_ptr->text.hl_anchor);
	t_ptr->text.hl_anchor = NULL;
	hl_init_text(t_ptr);
}
