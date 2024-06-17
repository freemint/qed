#include <support.h>

#include "global.h"
#include "ausgabe.h"
#include "hl.h"
#include "memory.h"
#include "options.h"
#include "rsc.h"
#include "window.h"

#define BLKANF  1
#define BLKEND  2
#define BLKFULL 4

/* lokale Variablen ********************************************************/
static char *text = NULL;
static short    text_len = 0;
#define MIN_TEXT_LEN    ((MAX_LINE_LEN+1) / 4)  /* StartlÑnge: 256 Bytes */
#define INSERT_CURSOR_WIDTH 3

/*!! Muessen am Anfang jeder Routine gesetzt werden !!*/
static bool 	tab;
static short    tab_size;
static bool 	umbrechen;
static bool 	show_end;
static short    draw_mode;

/* Statische Variablen fÅr str_out(); werden am Anfang von str_out()
 * initialisiert und dann von den Unterfunktionen aktualisiert
 */
static bool last_italic;  /* letzte Ausgabe war kursiv, initialisieren mit FALSE */
static bool line_start;   /* Ausgabe am Zeilenbeginn, init. mit TRUE */


void set_drawmode(void)
{
	draw_mode = MD_TRANS;
    vswr_mode(vdi_handle, draw_mode);
}


static void set_fill_color(short new)
{
    if (new != fill_color)
    {
        vsf_color(vdi_handle, new);
        fill_color = new;
    }
}

/*
 * Dynamischer Zeilenpuffer fÅr die expandierte Zeile. MAX_LINE_LEN reicht
 * nicht aus, wenn eine Zeile echte TABs enthÑlt!
 * Der angeforderte Puffer wird von line_to_str() benutzt.
*/
static void adjust_text(TEXTP t_ptr)
{
    short   need;

    if (t_ptr->max_line == NULL)
        need = -1;
    else
        need = t_ptr->max_line->exp_len;
    if (text == NULL || need > text_len)
    {
        text_len = max(need, MIN_TEXT_LEN);
        text = realloc(text, text_len + 1);
    }
}


/* Liefert die interne Position */
short inter_pos(short x, LINEP a, bool tabflag, short tabsize)
{
    short   len  = 0,
            tabH = tabsize,
            i    = 0;
    char *str;

    if (!tabflag)
        return min(x,a->len);
    str = TEXT(a);
    while(len < x && i < a->len)
    {
        if ((*str++) == '\t')
        {
            len += tabH;
            tabH = tabsize;
        }
        else
        {
            len++;
            if ((--tabH)==0)
                tabH = tabsize;
        }
        i++;
    }
    if (len > x)
        i--;
    return i;
}

short bild_len(LINEP a, bool tabflag, short tabsize)
{
    return bild_pos(a->len,a,tabflag,tabsize);
}

short bild_pos(short x, LINEP a, bool tabflag, short tabsize)
{
    short   len  = 0,
            tabH = tabsize;
    char    *str;

    if (!tabflag)
        return min(x, a->len);
    str = TEXT(a);
    while ((--x)>=0)
    {
        if ((*str++) == '\t')
        {
            len += tabH;
            tabH = tabsize;
        }
        else
        {
            len++;
            if ((--tabH)==0)
                tabH = tabsize;
        }
    }
    return len;
}

/*----------------------------------------------------------------------------
 * Cursor-Handling
*/
short cursor_xpos(TEXTP t_ptr, short position, bool *isitalicp)
{
	char *textptr;
	HL_LINE cacheline;
	HL_ELEM flags = 0;
	HL_ELEM len;
	short tabwidth;
	short i;
	_WORD pxy[8];
	short x = 0;
	char *line, *start_line;
	short done_expanded = 0;

    if (isitalicp)
        *isitalicp = FALSE;
        
    if (!position)
        return 0;

    /* bei nichtprop. Font wird der Syntax-Cache nicht ausgewertet, sondern
       es wird die Breite aus der Zeichenzellbreite berechnet */
    if (!font_prop)
    {
        if (isitalicp)
        {
            short tpos = position;
            cacheline = hl_get_zeile(t_ptr->cursor_line);
            while (!(*cacheline & HL_CACHEEND))
            {
                flags = *(cacheline++);    /* Hole LÑnge und Attribute aus dem Syntax-Cache */
                len = *(cacheline++);
                if (flags & HL_COLOR)     /* Farbinformationen sind irrelevant */
                    cacheline++;
                if (flags & HL_SELCOLOR)
                    cacheline++;
                if ((tpos -= (short) len) <= 0)
                    break;
            }
            *isitalicp = flags & HL_ITALIC;
        }
        return bild_pos(position, t_ptr->cursor_line, t_ptr->loc_opt->tab, t_ptr->loc_opt->tabsize) * font_wcell;
    }

    adjust_text(t_ptr);
    cacheline = hl_get_zeile(t_ptr->cursor_line);
    start_line = line = TEXT(t_ptr->cursor_line);
    tabwidth = t_ptr->loc_opt->tab ? t_ptr->loc_opt->tabsize : 1;

	while (!(*cacheline & HL_CACHEEND) && (short)(line - start_line) < position)
	{
		flags = *(cacheline++);    /* Hole LÑnge und Attribute aus dem Syntax-Cache */
		len = *(cacheline++);
		if (flags & HL_COLOR)     /* Farbinformationen sind irrelevant */
			cacheline++;
		if (flags & HL_SELCOLOR)
			cacheline++;
	
		for (textptr = text; len && (short)(line - start_line) < position; line++, len--)
		{
			if (*line == '\t') /* Tabs werden innerhalb der Kopierschleife expandiert */
			{
				for (i = ((short)(textptr - text) + done_expanded) % tabwidth; i < tabwidth; i++)
					*(textptr++) = ' ';
			}
			else
				*(textptr++) = *line;
		}
		done_expanded += (short)(textptr-text);
		*textptr = EOS;
		vst_effects(vdi_handle, flags & (HL_BOLD | HL_LIGHT));
		vqt_extent(vdi_handle, text, pxy);
		x += pxy[2] - pxy[0];
	}

	if (isitalicp)
		*isitalicp = flags & HL_ITALIC;

	return x;

}


static void _cursor(GRECT *r)
{
    _WORD pxy[4];

    pxy[0] = r->g_x;
    pxy[1] = r->g_y;
    pxy[2] = r->g_x + r->g_w - 1;
    pxy[3] = r->g_y + r->g_h - 1;
    set_fill_color(fg_color);
    vswr_mode(vdi_handle, MD_XOR);
    vr_recfl (vdi_handle, pxy);
    vswr_mode(vdi_handle, draw_mode);
}

/* schrÑger Cursor; in r werden die normalen Cursormaûe wie in _cursor()
   Åbergeben, die von _cursor_italic() dann schrÑg gesetzt werden */
static void _cursor_italic(GRECT *r)
{
    /* kleine & schrÑge Breiten bei v_fillarea werden im XOR-Modus absolut unbrauchbar
       dargestellt, daher Zeichnen des Cursors mit v_pline :-(*/
    _WORD pxy[4];
    short i;
    
    pxy[0] = r->g_x + font_right_italicoffset;
    pxy[1] = r->g_y;
    pxy[2] = r->g_x - font_left_italicoffset;
    pxy[3] = r->g_y + r->g_h - 1;
    
    set_fill_color(fg_color);
    vswr_mode(vdi_handle, MD_XOR);
    for (i=0; i < r->g_w; i++)
    {
        v_pline(vdi_handle, 2, pxy);
        pxy[0]++;
        pxy[2]++;
    }
    vswr_mode(vdi_handle, draw_mode);
}

void cursor(WINDOWP w, TEXTP t_ptr)
{
    _WORD       pxy[8];
    char        c[2];
    GRECT       curs_rect, clip, clip_curs_rect;
    long        zeile;
    bool        hidden;
    bool isitalic;

    /* Position ermitteln */
    curs_rect.g_x = cursor_xpos(t_ptr, t_ptr->xpos, &isitalic) - ((short) w->doc.x * font_wcell) + w->work.g_x;

    zeile = t_ptr->ypos - w->doc.y;
    curs_rect.g_y = (short) zeile * font_hcell + w->work.g_y;

    /* Cursor Åberhaupt sichtbar? */
    if (curs_rect.g_x < w->work.g_x || curs_rect.g_x > (w->work.g_x + w->work.g_w - 1) ||
         curs_rect.g_y < w->work.g_y || curs_rect.g_y > (w->work.g_y + w->work.g_h - 1) ||
         (w->flags & WI_ICONIFIED))
        return;

    /* Breite und Hîhe ermitteln */
    if (overwrite)
    {
        if (font_prop)
        {
            c[0] = TEXT(t_ptr->cursor_line)[t_ptr->xpos];
            c[1] = EOS;
            vqt_extent(vdi_handle, c, pxy);
            curs_rect.g_w = pxy[2] - pxy[0];
            if (curs_rect.g_w == 0)
                curs_rect.g_w = 1;
            curs_rect.g_h = pxy[7] - pxy[1];
        }
        else
        {
            curs_rect.g_w = font_wcell;
            curs_rect.g_h = font_hcell;
        }
    }
    else
    {
        curs_rect.g_w = INSERT_CURSOR_WIDTH;
        curs_rect.g_h = font_hcell;
    }

    /*
     * Am Rand (oben/unten) darf der Cursor nicht Åberstehen, ansonsten
     * 2 Pixel oben/unten.
     */
    if (zeile > 0)
    {
        curs_rect.g_y -= 2;
        curs_rect.g_h += 2;
    }
    if (zeile < w->w_height-1)
        curs_rect.g_h += 2;

    wind_update(BEG_UPDATE);
    hidden = hide_mouse_if_needed(&curs_rect);

    clip_curs_rect = curs_rect;
    if (isitalic)
    {
        clip_curs_rect.g_x -= font_left_italicoffset;
        clip_curs_rect.g_w += font_left_italicoffset + font_right_italicoffset;
    }
    if (rc_first(w->handle, &clip_curs_rect, &clip))
    {
        do
        {
            set_clip(TRUE, &clip);
            if (isitalic)
                _cursor_italic(&curs_rect);
            else
                _cursor(&curs_rect);
        }
        while (rc_next(w->handle, &clip));
    }
    if (hidden)
        show_mouse();
    wind_update(END_UPDATE);
}

static LINEP get_wline(TEXTP t_ptr, long y)
{
    LINEP  line;
    long        i;

    if (y < 0 || y >= t_ptr->text.lines)
        return NULL;
    i = t_ptr->ypos;
    line = t_ptr->cursor_line;
    if (i > y)
    {
        i -= y;
        while (TRUE)
        {
            PREV(line);
            if ((--i)==0)
                break;
        }
    }
    else if (i < y)
    {
        y -= i;
        while (TRUE)
        {
            NEXT(line);
            if ((--y)==0)
                break;
        }
    }
    return (line);
}

/*-----------------------------------------------------------------------------
 * FlÑche fÅllen.
 *
 * x ist Pixel-Koordinate
 * y ist Pixel-Koordinate
 * w ist die Breite in Pixel, die abgedeckt werden soll
*/
void fill_area(short x, short y, short w, short h, short color)
{
    _WORD   pxy[4];

    if (w <= 0)
        return;
    pxy[0] = x;
    pxy[1] = y;
    pxy[2] = x + w - 1;
    pxy[3] = y + h - 1;
    set_fill_color(color);
    vswr_mode(vdi_handle, MD_REPLACE);
    vr_recfl(vdi_handle, pxy);
    vswr_mode(vdi_handle, draw_mode);
}

/* FlÑche fÅllen mit BerÅcksichtigung kursiver Schrift;
 * mit isitalic wird festgelegt,
 * ob die FlÑche links im Duktus
 * der Schrift nach rechts geneigt sein soll. Rechts wird immer eine gerade
 * Kante gezeichnet.
 */
static void fill_area_italic(short x, short y, short w, short h, short color, bool isitalic)
{
    _WORD   pxy[8];

    if (line_start) /* Zeilenbeginn? */
    {
        isitalic = FALSE; /* ab jetzt kein Zeilenbeginn mehr */
        line_start = FALSE;  /* nie kursiv am Zeilenbeginn */
    }

    if (!isitalic) /* wenn nicht kursiv gezeichnet werden soll, wird das schnellere fill_area() benutzt */
    {
        fill_area(x,y, w+font_right_italicoffset, h, color);
        return;
    }
    if (w <= 0)
        return;
    pxy[0] = x + (isitalic ? font_right_italicoffset : 0);
    pxy[1] = y;
    pxy[2] = x + w - 1 + font_right_italicoffset;
    pxy[3] = y;
    pxy[4] = x + w - 1 + font_right_italicoffset;
    pxy[5] = y + h - 1;
    pxy[6] = x - (isitalic ? font_left_italicoffset : 0);
    pxy[7] = y + h - 1;
    
    set_fill_color(color);
    vswr_mode(vdi_handle, MD_REPLACE);
    v_fillarea(vdi_handle, 4, pxy);
    vswr_mode(vdi_handle, draw_mode);
}

/*
 * String mit v_gtext() ausgeben.
 *
 * x ist Pixel-Koordinate
 * y ist Pixel-Koordinate
 * w ist die Breite in Pixel, die abgedeckt werden soll
 * return ende der Textausgabe
*/
short out_s(short x, short y, short w, char *str)
{
    _WORD   pxy[8], len;
    
    vst_color(vdi_handle, fg_color);
	vst_effects(vdi_handle, 0);

    if (w <= 0)
        return x;

    fill_area(x, y, w, font_hcell, bg_color);

    v_gtext(vdi_handle, x, y, str);
    if (font_prop)
    {
        vqt_extent(vdi_handle, str, pxy);
        len = pxy[2]-pxy[0];
    }
    else
        len = (short) strlen(str) * font_wcell;

    if (len < w)
        return x + len;

    return x + w;
}

/*
 * String invers mit v_gtext ausgeben.
 *
 * x ist Pixel-Koordinate
 * y ist Pixel-Koordinate
 * w ist die Breite in Pixel, die abgedeckt werden soll
 * return ende der Textausgabe
*/
short out_sb(short x, short y, short w, char *str)
{
    _WORD   pxy[8], len;

    if (w <= 0)
        return x;

    /* Hintergrund mit fg_block_color fÅllen und Text mit fg_block_color drÅbermalen */
    fill_area(x, y, w, font_hcell, bg_block_color);

    vst_color(vdi_handle, fg_block_color);
	vst_effects(vdi_handle, 0);

    v_gtext(vdi_handle, x, y, str);

    if (font_prop)
    {
        vqt_extent(vdi_handle, str, pxy);
        len = pxy[2] - pxy[0];
    }
    else
        len = (short) strlen(str) * font_wcell;
    if (len > w)
        return x + w;
    return x + len;
}

/*
 * Absatzmarke zeichnen.
*/
static void draw_cr(short x, short y, bool inv)
{
    _WORD pxy[6], h, b;

    if (inv)
        vsl_color (vdi_handle, fg_block_color);
    else
        vsl_color (vdi_handle, fg_color);
    b = min(font_wcell, font_hcell);
    h = b >> 1;
    y += (font_hcell >> 1);
    pxy[0] = x + b - 1;         /* oben rechts */
    pxy[1] = y - h;
    pxy[2] = x + b - 1;         /* mitte rechst */
    pxy[3] = y;
    pxy[4] = x;                     /* mitte links */
    pxy[5] = y;
    v_pline (vdi_handle, 3, pxy);
    h = h >> 1;
    pxy[0] = x + h;             /* schrÑg oben */
    pxy[1] = y - h;
    pxy[2] = x;                     /* mitte links */
    pxy[3] = y;
    pxy[4] = x + h;             /* schrÑg unten */
    pxy[5] = y + h;
    v_pline (vdi_handle, 3, pxy);
    if (inv)
        vsl_color(vdi_handle, fg_color);
    else
        vsl_color(vdi_handle, fg_block_color);
}

/* Ausgabe von Text mit und ohne Effekte, Aufruf von str_out()
 * Aufgrund von starken Darstellungsungenauigkeiten bzw. verschiedener
 * Behandlung verschiedener Fonttypen im VDI mÅssen teilweise
 * die Fonttypen (proportional/nichtproportional, Bitmap/Vektor) und
 * Effekte (insbes. fett und kursiv) umstÑndlich und teilweise sehr
 * zeitaufwendig getrennt behandelt werden.
 * In der statischen Var. last_italic wird jeweils der Wert gehalten, ob die letzte Ausgabe kursiv war; vor
 * dem ersten Aufruf von output_text in einer Zeile muû der Wert auf false gesetzt werden.
 * Vorder-und Hintergrundfarbe werden gesetzt, wenn sie jeweils != -1 sind; kursive
 * Blockmarkierungen werden bearbeitet.
 */
static void output_text(short x, short y, short *width,
                        short vdiattr,

                        char *buffer,
                        short fgcolor, short bgcolor)
{
    _WORD pxy[8];
    bool isitalic = vdiattr & HL_ITALIC;

    if (fgcolor != -1)
        vst_color(vdi_handle, fgcolor);
        
    /* Proportionalfonts */
    if (font_prop)
    {
        vst_effects(vdi_handle, vdiattr & ~HL_ITALIC);
        vqt_extent(vdi_handle, buffer, pxy);
        *width = pxy[2] - pxy[0];
        if (bgcolor != -1)
            fill_area_italic(x, y, *width, font_hcell, bgcolor, isitalic || last_italic);
        vst_effects(vdi_handle, vdiattr);
        v_gtext(vdi_handle, x,y, buffer);
        last_italic = isitalic;
        return;
    }

    /* Kursiv, nichtproportional, Bitmapfont */
    if (isitalic && !font_vector)
    {
        if (bgcolor != -1)
            fill_area_italic(x, y,
                              *width, font_hcell, bgcolor,
                              TRUE);
        last_italic = TRUE;
        v_gtext(vdi_handle, x,y, buffer);

        return;
    }

    /* Fett , nichtproportional, Vektorfont: hier muû sehr umstÑndlich
       und zeitaufwendig Zeichen fÅr Zeichen ausgegeben werden,
       da sonst die AbstÑnde (auch mit v_justified()) nicht stimmen
       und die Schrift "schwabbelt". Dummerweise wird dieser Schrifttyp sehr
       hÑufig in QED benutzt werden. */
    if ( (vdiattr & HL_BOLD) && font_vector  )
    {
        char cbuf[2];
        cbuf[1] = '\0';
        
        if (bgcolor != -1)
            fill_area_italic(x, y, *width, font_hcell, bgcolor, isitalic || last_italic);
            
        while (*buffer)
        {
            cbuf[0] = *(buffer++);
            v_gtext(vdi_handle, x,y, cbuf);
            x += font_wcell;
        }
        last_italic = isitalic;
        return;
    }

    /* sonstige Effekte, nichtproportional, alle Fonts */
    if (vdiattr)
    {
        if (bgcolor != -1)
            fill_area_italic(x, y, *width, font_hcell, bgcolor, isitalic || last_italic);
        v_justified(vdi_handle, x,y, buffer, *width, TRUE, TRUE);
        last_italic = isitalic;
        return;
    }

    /* keine Effekte, nichtproportional, alle Fonts */
    if (bgcolor != -1)
        fill_area_italic(x, y, *width, font_hcell, bgcolor, last_italic);
    v_gtext(vdi_handle, x,y, buffer);
    last_italic = FALSE;
}

/* Ausgabe eines nicht expandierten Strings; Ersatz fÅr die alten Funktionen
 * str_out() und str_out_b(). String wird inline expandiert.
 * x,y,w legen Koordinaten fest, offset ist Zeichenoffset (nicht expandiert),
 * zp die auszugebende Zeile, block_start und block_end legen die Blockmarkierungen fest:
 * block_start == -1: kein Block, sonst: Blockbeginn
 * block_end == -1: kein Block oder Block bis Zeilenende (je nach block_start), sonst: Blockende
 */
   
static void str_out( short x, short y, short w, short offset, LINEP zp, short block_start, short block_end )
{
    char *curr_text;   /* derzeitige Zeichenposition im globalen Puffer text (fÅr kopieren/tab-Expansion) */
    char *curr_line;   /* aktuelle Zeichenposition in der nicht expandierten Zeile */
    HL_LINE cacheline; /* aktuelle Syntax-Cache-Zeile */
    HL_ELEM flags;     /* aktuelle Syntax-Cache-Flags */
    HL_ELEM len;       /* aktuelle TextlÑnge aus Syntax-Cache */

    short fgcolor;       /* aktuelle Vordergrund-(Text-)Farbe */
    short bgcolor;       /* aktuelle Hintergrundfarbe */
    short hlcolor;       /* Vordergrund-Farbe aus Syntax-Cache */
    short hlselcolor;    /* selektierte Vordergrund-Farbe aus Syntax-Cache */
    short vdiattr;       /* VDI-Attribute */
    short width;         /* aktuelle Breite des expandierten Strings in Pixeln */
    short chars_done = 0;/* bereits bearbeitete expandierte Zeichen (fÅr Offset-Berechnung) */
    bool isblock = block_start >= 0 || block_end > 0;    /* Ist ein Block in dieser Zeile? */
    bool blockmode = block_start == 0 && block_end != 0; /* Blockmodus ein? */
    
    /* Var. init. */
    curr_line = TEXT(zp);
    cacheline = hl_get_zeile(zp);
    line_start = TRUE;
    last_italic = FALSE;
            
    /* Vorder/Hintergrundfarbe zu Beginn setzen */
    if (blockmode)
    {
        bgcolor = bg_block_color;
        vst_color(vdi_handle, fg_block_color);
    }
    else
    {
        vst_color(vdi_handle, fg_color);
        if (isblock)
            bgcolor = bg_color;
        else
        {
            /* wenn kein Block in dieser Zeile, Hintergrund komplett durchfÑrben */
            fill_area(x, y, w, font_hcell, bg_color);
            bgcolor = -1;
        }
    }
    vswr_mode(vdi_handle, MD_TRANS);

    offset *= font_wcell;
    x -= offset;
    w += offset;

    while (!(*cacheline & HL_CACHEEND))
    {
        width = 0;
        curr_text = text; /* Pufferzeiger auf Pufferanfang */

        /* Syntax-Cache auswerten, Vorder/Hintergrundfarbe und Effekte setzen */
        flags = *(cacheline++);
        len = *(cacheline++);
        hlcolor = flags & HL_COLOR ? *(cacheline++) : -1;
        hlselcolor = flags & HL_SELCOLOR ? *(cacheline++) : -1;
        vdiattr = flags & (HL_ITALIC | HL_BOLD | HL_LIGHT);
        if (blockmode)
            fgcolor = (hlselcolor == -1 ? fg_block_color : hlselcolor);
        else
            fgcolor = (hlcolor == -1 ? fg_color : hlcolor);
        vst_effects(vdi_handle, vdiattr);
        vst_color(vdi_handle, fgcolor);

        while (len)
        {
            /* Zeichen in text-Puffer Åbertragen */
            if (tab && *curr_line == '\t') /* TABs werden inline expandiert */
            {
                short i;
                for (i= ((short)(curr_text-text)+chars_done) % tab_size; i<tab_size; i++)
                {
                    *(curr_text++) = ' ';
                    width += font_wcell;
                }
            }
            else if (*curr_line < min_ascii || *curr_line > max_ascii) /* nicht darstellbare Zeichen ersetzen */
            {
                *(curr_text++) = ' ';
                width += font_wcell;
            }
            else
            {
                *(curr_text++) = *curr_line;
                width += font_wcell;
            }
            curr_line++;
            len--;
            /* Bei Blockbeginn/Blockende Farbdarstellung umschalten */
            if (curr_line - TEXT(zp) == block_start)
            {
                vswr_mode(vdi_handle, MD_TRANS);
                blockmode = TRUE;
                chars_done += (short)(curr_text-text);
                *curr_text = EOS;
                output_text(x,y, &width, vdiattr, text, fgcolor, bgcolor);
                curr_text = text;
                fgcolor = hlselcolor == -1 ? fg_block_color : hlselcolor;
                bgcolor = bg_block_color;
                x += width;
                width = 0;
                if (!len)
                    goto outer_loop;
            }
            else if (curr_line - TEXT(zp) == block_end)
            {
                blockmode = FALSE;
                chars_done += (short)(curr_text-text);
                *curr_text = EOS;
                output_text(x,y, &width, vdiattr, text, fgcolor, bg_block_color);
                curr_text = text;
                fgcolor = (hlcolor == -1 ? fg_color : hlcolor);
                bgcolor = bg_color;
                x += width;
                width = 0;
                if (!len)
                    goto outer_loop;
            }
        }
        chars_done += (short)(curr_text-text);
        *curr_text = EOS;
        output_text(x,y, &width, vdiattr, text, fgcolor, bgcolor);
                    
outer_loop:;

        x += width;
    }
    
    /* Rest der Zeile fÑrben und Umbruchmarke */
    if (isblock)
        fill_area_italic(x, y, w, font_hcell, bgcolor, last_italic);
    if (IS_OVERLEN(zp) || (umbrechen && IS_ABSATZ(zp) && show_end))
        draw_cr(x, y, blockmode);
    
}


/* =========================================================== */

void line_out(WINDOWP window, TEXTP t_ptr, short wy)
{
    short       y;
    LINEP  col;

    adjust_text(t_ptr);                 /* Zeilenpuffer an exp_len anpassen */
    tab = t_ptr->loc_opt->tab;
    tab_size = t_ptr->loc_opt->tabsize;
    umbrechen = t_ptr->loc_opt->umbrechen;
    show_end = t_ptr->loc_opt->show_end;
    y = window->work.g_y+wy*font_hcell;
    col = get_wline(t_ptr, window->doc.y+wy);
    if (col != NULL)                                                /* Kommt vor */
    {
        if (t_ptr->block)
        {
            wy += (short) window->doc.y;
            if (wy<t_ptr->z1 || wy>t_ptr->z2)
                str_out(window->work.g_x, y, window->work.g_w, (short)window->doc.x, col, -1, -1);
            else
                str_out(window->work.g_x, y, window->work.g_w, (short)window->doc.x, col,
                        (wy == t_ptr->z1) ? t_ptr->x1 : 0,
                        (wy == t_ptr->z2) ? t_ptr->x2 : -1);
        }
        else
            str_out(window->work.g_x, y, window->work.g_w, (short) window->doc.x, col, -1, -1);
    }
    else
        fill_area(window->work.g_x, y, window->work.g_w, font_hcell, bg_color);
}

void bild_out(WINDOWP window, TEXTP t_ptr)
{
    short       x, y, w;
    LINEP  line;
    short       min_col, max_col, max_y;
    GRECT       c;

    adjust_text(t_ptr);                 /* Zeilenpuffer an exp_len anpassen */
    tab = t_ptr->loc_opt->tab;
    tab_size = t_ptr->loc_opt->tabsize;
    umbrechen = t_ptr->loc_opt->umbrechen;
    show_end = t_ptr->loc_opt->show_end;
    x = window->work.g_x;
    y = window->work.g_y;
    w = window->work.g_w;

    if (window->class == CLASS_EDIT)                        /* Kopf ausgeben */
        head_out(window, t_ptr);

    min_col = 0;
    max_col = (short) min(window->w_height - 1, t_ptr->text.lines - window->doc.y-1);
    max_y   = y + window->work.g_h-1;
    if (get_clip(&c))                   /* nicht alles malen */
    {
        short y2 = c.g_y - y;

        min_col = max(min_col, y2/font_hcell);
        max_col = min(max_col, (c.g_y + c.g_h - 1) / font_hcell);
        max_y = min(max_y, c.g_y + c.g_h - 1);
    }
    y += (min_col * font_hcell);
    line = get_wline(t_ptr, window->doc.y + min_col);
    if (line != NULL)
    {
        short   xoffset, i;

        xoffset = (short) window->doc.x;
        if (t_ptr->block)
        {
            long    y_r;

            y_r = window->doc.y + min_col;
            for (i = min_col ; i <= max_col; y_r++, y += font_hcell, NEXT(line), i++)
            {
                /* Block nicht sichtbar */
                if (y_r < t_ptr->z1 || y_r > t_ptr->z2)
                    str_out(x, y, w, xoffset, line, -1, -1);
                else
                    str_out(x, y, w, xoffset, line,
                            (y_r == t_ptr->z1) ? t_ptr->x1 : 0,
                            (y_r == t_ptr->z2) ? t_ptr->x2 : -1);
            }
        }
        else
        {
            for (i=min_col ; i<=max_col; y+=font_hcell,NEXT(line),i++)
                str_out(x,y,w,xoffset,line, -1, -1);
        }
    }

    /* Fenster hîher als Text lang ist -> Rest lîschen */
    if (y < max_y)
        fill_area(x, y, w, (max_y - y + 1), bg_color);
}

void bild_blkout(WINDOWP window, TEXTP t_ptr, long z1, long z2)
/* Alle Textzeilen zwischen z1 und z2 werden neu ausgegeben */
{
    short       i, x, y, w, xoffset;
    LINEP  line;
    short       max_col;
    long        lines, y_r;

    adjust_text(t_ptr);                 /* Zeilenpuffer an exp_len anpassen */
    tab = t_ptr->loc_opt->tab;
    tab_size = t_ptr->loc_opt->tabsize;
    umbrechen = t_ptr->loc_opt->umbrechen;
    show_end = t_ptr->loc_opt->show_end;
    if (z1>z2)
    {
        lines = z1;
        z1 = z2;
        z2 = lines;
    }
    x         = window->work.g_x;
    y         = window->work.g_y;
    w         = window->work.g_w;
    xoffset = (short) window->doc.x;
    max_col = (short) min(window->w_height-1, t_ptr->text.lines-window->doc.y-1);
    y_r   = window->doc.y;
    line = get_wline(t_ptr, y_r);

    if (t_ptr->block)
        for (i=0; i<=max_col; i++,y+=font_hcell,y_r++)
        {
            if (y_r>=z1 && y_r<=z2)
            {
                if (y_r<t_ptr->z1 || y_r>t_ptr->z2)
                    str_out(x,y,w,xoffset,line, -1, -1);
                else
                    str_out(x, y, w, xoffset, line,
                            (y_r == t_ptr->z1) ? t_ptr->x1 : 0,
                            (y_r == t_ptr->z2) ? t_ptr->x2 : -1);
            }
            NEXT(line);
        }
    else
        for (i=0; i<=max_col; i++,y+=font_hcell,y_r++)
        {
            if (y_r>=z1 && y_r<=z2)
                str_out(x,y,w,xoffset,line, -1, -1);
            NEXT(line);
        }
}


/***************************************************************************/

void head_out(WINDOWP window, TEXTP t_ptr)
{
    char    head_str[WINSTRLEN+1];
    short   len, head_len;

    if (t_ptr->info_str[0] != EOS)
    {
        strncpy(head_str, t_ptr->info_str, WINSTRLEN);
        head_str[WINSTRLEN] = EOS;
        head_len = (short) strlen(head_str);
    }
    else
    {
        if (t_ptr->text.ending != lns_binmode)
        {
            head_len = (short) strlen(rsc_string(HEADSTR));
            strcpy(head_str, rsc_string(HEADSTR));
            if (t_ptr->readonly)
                head_str[1] = '\x7F';

            switch (t_ptr->text.ending)
            {
                case lns_tos :
                    break;
                case lns_unix :
                    head_str[2] = 'U';
                    break;
                case lns_apple :
                    head_str[2] = 'A';
                    break;
                default:
                    head_str[2] = '?';
            }

            ltoa(t_ptr->ypos+1, head_str + 8, 10);
            head_str[strlen(head_str)] = ' ';
            ltoa(bild_pos(t_ptr->xpos,t_ptr->cursor_line,tab,tab_size)+1,head_str+18,10); /* STan: itoa -> ltoa */
            head_str[strlen(head_str)] = ' ';
        }
        else
        {
            long    p;

            head_len = (short) strlen(rsc_string(BHEADSTR));
            strcpy(head_str, rsc_string(BHEADSTR));
            if (t_ptr->readonly)
                head_str[1] = '\x7F';
            p = t_ptr->ypos * t_ptr->text.max_line_len + t_ptr->xpos + 1;
            ltoa(p, head_str + 24, 10);
        }
    }
    len = window->work.g_w / gl_wchar;
    if (len < head_len)
        head_str[len] = EOS;
    set_winfo(window, head_str);
}

/***************************************************************************/
