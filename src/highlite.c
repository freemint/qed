/***************************************************************************
                          highlite.c  -  description

 * flexible cached syntax highlighting
 * supports bold, italic and light text style (changeable)
 * supports different colors for normal text and selected text
 * supports three different resolutions: 2 colors, 16 colors and >=256 colors
 * loading and saving of ascii files which contain the syntax settings; so
   it can easily be changed for new languages

 highlite.c is intended for syntax highlighting of ascii texts. For wide chars, you
 will have to convert the chars first.
 The syntax informations are hold in a cache. Use the functions Hl_Update(),
 Hl_InsertLine(), Hl_RemoveLine(), Hl_GetLine(), Hl_New() and Hl_Free()
 to handle this cache. Be sure to always update this cache when your source
 line changes and, of course, to free it when you discard your source line!
 Hl_GetLine() returns the syntax information. It is coded in an array of
 HL_ELEM:
 - the first element contains flags. A set flag HL_CACHEEND marks the end
   of the cache line. It contains the attrib flags too.
 - the second element contains the length of the current settings
 - (only if HL_COLOR in flags is set) the next element contains the normal
   color
 - (only if HL_SELCOLOR in flags is set) the next element contains the selected
   color
 The next elements contain the next settings of the cache line, until
 HL_CACHEEND is set in the first element.


 ***************************************************************************/

#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <ctype.h>
#include "highlite.h"
#include "lists.h"

#include "global.h"

/* memory allocation is defined as macros to give you the choice
   to use an own memory handler. */

#define HL_MALLOC(a) malloc(a)
#define HL_CALLOC(a,b) calloc(a,b)
#define HL_REALLOC(a,b) realloc(a,b)
#define HL_FREE(a) free(a)

/* Of course we must use this also for lists. */
#undef LISTALLOC
#undef LISTFREE
#define LISTALLOC(a) HL_CALLOC(1,a)
#define LISTFREE(a) HL_FREE(a)

/* error-macros */
/****************/

#define HL_ERROR( errn, linen, ret )\
{\
	if( error_callback )\
		error_callback( curr_synfile, errn, linen );\
	else\
		fprintf( stderr, "hl-Error %i in file %s in line %i\n",(errn),curr_synfile,(linen) );\
	{ret}\
}

#define HL_MEM_ERROR( ret )\
{\
	Hl_Exit();\
	if( error_callback )\
		error_callback( NULL, E_HL_MEMORY, 0 );\
	else\
		fprintf( stderr, "hl-fatal memory error\n" );\
	memory_error = TRUE;\
	{ret}\
}

/* list allocation error macro */
#undef LISTERROR
#define LISTERROR \
{\
	Hl_Exit();\
	if( error_callback )\
		error_callback( NULL, E_HL_MEMORY, 0 );\
	else\
		fprintf( stderr, "out of memory while allocating list!\n");\
	memory_error = TRUE;\
}

/* text rule flags */
#define TXTRULEF_CASE 1      /* text is case-sensitive */
#define TXTRULEF_ACTIVE 2

/* rule flags */
#define RULEF_EOL 1
#define RULEF_NESTED 8
#define RULEF_FIRSTCOLUMN 16

/* other defines */
#define BUFBLK 256           /* size of a linebuffer block */

#ifndef BOOLEAN
typedef short BOOLEAN;
#endif
#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

/* rule types */
typedef enum {
	RULE_KEYWORD,  /* rule is a keyword, e.g. C keywords: <if>,<while>, ... */
	RULE_FROM,     /* start of a to- or while-rule, e.g. Pascal-Comment: "(*", c-hex-constants: "0x"  */
	RULE_TO,       /* to-rule (eg. Pascal-Comment: "*)") */
	RULE_WHILE     /* while-rule (e.g. c-hex-constants: <0-9>,<a-f>) */
} RULETYPE;

/* chained list string */
typedef struct stringentry {
	int len;
	char *name;
	struct stringentry *next;
} STRINGENTRY;

/* style struct; attribs and colors */
typedef struct {
	HL_ELEM attribs;
	long color;
	long selcolor;
} HL_STYLEINFO;

/* resolution type */
typedef enum {
	COLOR2,
	COLOR16,
	COLOR256,
	NUM_RESTYPES
} RESTYPE;

/* a rule; description for detecting keywords/chars and setting the corresponding colors and flags */
typedef struct rule {
	char *name;                       /* name of the rule, e.g. "Comment" */
	BOOLEAN dup;                      /* duplicate rule; another rule of the same name already exists, only the attributes of the 1st rule are valid */
	RULETYPE type;                    /* rule type, RULE_... */
	int flags;                        /* rule flags, RULEF_... */
	HL_STYLEINFO style[NUM_RESTYPES]; /* attributes & colors for different resolutions */
	HL_ELEM attribs;                  /* actual text attributes */
	long color;                       /* actual text color */
	long selcolor;                    /* actual selected text color */
	STRINGENTRY *kwstring[256];       /* all strings to be searched for, indexed by the first char */
	char kwsinglechar[256];           /* all single chars of this rule */
	char kwchar[256];                 /* all beginning chars of kwsinglechar and kwstring (pre filter) */
	char quotechar;                   /* quote char, as e.g. \ in C strings */
	struct rule *link;                /* stoprule, if type == RULE_FROM, or link to startrule if nested rule */
	struct rule *next;                /* next rule struct */
} RULE;

/* Rule for a text type, e.g. "c"/"h", "pas", "s"... */
typedef struct txtrule {
	char *name;                  /* name of the text type, e.g. "C source" */
	char *filename;              /* filename of this rule */
	STRINGENTRY *txttypes;       /* txttypes of the text type, e.g "c" and "h" for c files */
	char token[256];             /* all chars this text type uses in its rules */
	char kwstartchar[256];       /* pre filter for start rules */
	char kwendchar[256];         /* pre filter for end rules */
	int flags;                   /* text flags, TXTRULEF_... */
	RULE *rules;                 /* the rules for this text type */
	BOOLEAN checked;             /* this textrule is checked already in check_rules() */
	struct txtrule *next;        /* next text type */
} TXTRULE;

/* a cache entry */
typedef struct cacheline {
	char *line;                  /* source text row */
	RULE *startrule;             /* rule which is active at the beginning of the row */
	RULE *endrule;               /* rule which is active at the end of the row */
	HL_LINE cachetok;            /* the encoded text format */
	struct cacheline *next;      /* next cache entry */
	int startcount;              /* for nested rules: count at the start of the cache line */
	int endcount;                /* count at the end of the cache line */
} CACHE;

/* the base for the cache of a text */
typedef struct cacheb {
	CACHE *cache;                /* cache entries */
	TXTRULE *txtrule;            /* text rule (is it c, pascal...? */
	struct cacheb *next;
} CACHEBASE;

/* struct for the temporary saving of rule settings by
 * Hl_SaveSettings() etc.
 */
typedef struct saveinf {
	HL_ELEM attribs;             /* text attributes */
	long color;                  /* color */
	long selcolor;               /* selected color */
	BOOLEAN isactive;            /* corresponding text rule is active */
	struct saveinf *next;
} SAVEINFO;


/******************************************************************************
 *local static vars
 ******************************************************************************/

static SAVEINFO *saveinfo = NULL;     /* anchor for the temporary saving of rule settings */
static TXTRULE *txtruleanchor = NULL; /* anchor for all text rules */
static CACHEBASE *cacheanchor = NULL; /* anchor for syntax cache */
static HL_LINE linebuffer = NULL;     /* line buffer, is allocated and resized dynamically */
static int bufflen=0;                 /* current length of buffer */
static int maxbufflen=0;              /* current maximum length of buffer */
static RESTYPE colidx;                /* color index (to access rule.style) of the current resol. */
static void (*error_callback)( char *currfile, HL_ERRTYPE err, int linenr ) = NULL; /* user error callback routine */
static BOOLEAN memory_error = FALSE;  /* set to true if a memory error occurred, all syntax highlighting
                                         is stopped then, and the portion of memory, which can be released,
                                         is freed */
static char *curr_synfile=NULL;       /* currently read config file */

/******************************************************************************/
/* various small subroutines */
/******************************************************************************/


/* trim a string from the chars in searchstr, both sides */
static char *str_xtrim( char *str, const char *searchstr )
{
	char *oldstr = str;
	char *endstr = str+strlen( str );

	while( *str && strchr( searchstr, *str ))
		str++;
	if( *str )
		while( strchr( searchstr, *(--endstr) ))
			*endstr = '\0';
	strcpy( oldstr, str );
	return oldstr;
}

/* remove quoted chars (with \ before) from a string
 * used for the internal quotes of the *.syn file
 */
static char *str_remove_quotes( char *st )
{
	char *rd = st, *wr = st;

	do {
		if( *rd == '\\' )
			rd++;
	} while( (*(wr++) = *(rd++)) != '\0' );
	return st;
}


/* expand all " and \ in a string to \" and \\. Used only for the *.syn-File,
 * not for the 'quotechar' option. A buffer big enough to contain
 * all expanded chars must be handed over.
 */
static char *str_expand_quote( char *buffer, char *st )
{
	char *rd = st; char *wr = buffer;
	do
	{
		if( *rd == '\"' || *rd == '\\' )
			*(wr++) = '\\';
	} while( (*(wr++) = *(rd++)) != '\0' );
	return buffer;
}


/* expand " and \ to \" and \\. Used only for the *.syn-File,
 * not for the 'quotechar' option
 */
static char *chr_expand_quote( char *buffer, char c )
{
	buffer[1] = buffer[2] = '\0';

	if( c == '\"' || c == '\\' )
	{
		buffer[0] = '\\';
		buffer[1] = c;
	}
	else
		buffer[0] = c;
	return buffer;
}

/* append a new STRINGENTRY struct to a STRINGENTRY list */
static STRINGENTRY *append_stringentry( STRINGENTRY **anchor, char *st, short linenr )
{
	STRINGENTRY *new_strentry;
	LIST_APPEND( STRINGENTRY, new_strentry, *anchor );
	if( memory_error )
		return NULL;
	if( new_strentry )
	{
		if( (new_strentry->name = strdup( st )) == NULL )
			HL_MEM_ERROR( return NULL; );
		new_strentry->len = (int) strlen(new_strentry->name);
	}
	return new_strentry;
}
/* free a complete stringentry list */
static void free_stringentrylist( STRINGENTRY **base )
{
	STRINGENTRY *strentry, *nxt_strentry;
	for( strentry = *base; strentry; strentry = nxt_strentry )
	{
		nxt_strentry = strentry->next;
		HL_FREE( strentry->name );
		HL_FREE( strentry );
	}
	*base = NULL;
}

/******************************************************************************
 * Hl_Init()
 * Sets the error callback.
 ******************************************************************************/

void Hl_Init( void (*err_callback)( char *currfile, HL_ERRTYPE err, int linenr ) )
{
	error_callback = err_callback;
}

/******************************************************************************
 * Hl_Exit()
 * Frees all memory allocated for syntax highlighting.
 * Hl_exit is also called if a memory error occurs!
 ******************************************************************************/

void Hl_Exit( void )
{
	TXTRULE *trule, *nxt_trule;
	RULE *rule, *nxt_rule;
	int i;

	for( trule = txtruleanchor; trule; trule = nxt_trule )
	{
		nxt_trule = trule->next;
		HL_FREE( trule->name );
		HL_FREE( trule->filename );

		free_stringentrylist( &trule->txttypes );
		for( rule = trule->rules; rule; rule = nxt_rule )
		{
			nxt_rule = rule->next;
			HL_FREE( rule->name );
			if( !(trule->flags & TXTRULEF_CASE) ) /* upper case string lists in case */
				for( i='A'; i<='Z'; i++ )           /* insensitive languages are just pointers */
					rule->kwstring[i] = NULL;         /* to the lower entrys and mustn't be freed */
			for( i=0; i<256; i++ )
				free_stringentrylist( &rule->kwstring[i] );
			if( rule->link )
			{
				HL_FREE( rule->link->name );
				for( i=0; i<256; i++ )
					free_stringentrylist( &rule->link->kwstring[i] );
				HL_FREE( rule->link );
			}
			HL_FREE( rule );
		}
		HL_FREE( trule );
	}
	while( cacheanchor )
		Hl_Free( (HL_HANDLE) cacheanchor );

}

/******************************************************************************
 *
 * Routines for loading and saving the .syn (config) file
 * Hl_ReadSyn() and Hl_WriteSyn()
 *
 ******************************************************************************/


/******************************************************************************/
/* subroutines for Hl_read_syn()
/******************************************************************************/

/* find rule by name
 */
static RULE *rule_by_name( TXTRULE *trule, char *name )
{
	RULE *rule = trule->rules;

	while( rule != NULL )
	{
		if( stricmp( rule->name, name ) == 0 )
			return rule;
		rule = rule->next;
	}

	return NULL;
}

/* find text rule by name
 */
static TXTRULE *txtrule_by_name( char *txtname )
{
	TXTRULE *trule = txtruleanchor;

	while( trule != NULL )
	{
		if( stricmp( trule->name, txtname ) == 0 )
			return trule;
		trule = trule->next;
	}

	return NULL;
}


/* read a string to null byte or ",
 * remove quotes, return pointer to this
 * string and  a pointer to the next string
 * if ended by " in str (or NULL if no next string)
 */
static char *rreadstr( char **str )
{
	char lastchar = '\0';
	char *retstr = ++(*str);
	while( **str )
	{
		if( **str == '\"' && lastchar != '\\' )
		{
			**str = '\0';
			if( !retstr[0] )
				return NULL;
			return str_remove_quotes( retstr );
		}
		if( lastchar == '\\' && **str == '\\' )
			lastchar = '\0';
		else
			lastchar = **str;
		(*str)++;
	}
	return NULL;
}

/* tokenize a rule string (e.g. "1"-"3","test","no\"quote",EOL)
 * first call with string to tokenize in str, next calls with NULL
 * (like strtok())
 * It returns the tokens of the string (in the example <1> <2> <3> <test> <no"quote>)
 * or NULL if no more tokens.
 * Also, flags are returned in <flags>, if the original string contains special
 * keywords as <EOL>.
 */
static char *ruletok( char *str, int *flags, int linenr )
{
	static char *nextpos = NULL;
	static BOOLEAN charmode = FALSE;
	static char currchar;
	static char endchar;
	static char charretstr[2];
	char *s1 = NULL, *s2 = NULL;

	*flags = 0;

	if( str )
		nextpos = str;
	else
		str = nextpos;

	/* return chars entered as e.g. "0"-"9" */
	if( charmode )
	{
		currchar++;
		if( currchar > endchar )
			charmode = FALSE;
		else
		{
			charretstr[0] = currchar;
			charretstr[1] = '\0';
			return charretstr;
		}
	}

	if( !nextpos )
		return NULL;

	for(;;)
	{
		switch( *str )
		{
			case '\"': 	if( !s1 ) /* entry in " */
			           	{
			            	s1 = rreadstr( &str );
			            	if( !s1 )
			              	HL_ERROR( E_HL_SYNTAX, linenr, return NULL; );
			           	}
			           	else if( !charmode ) /* charmode means two entries in ", separated by - */
			           	{
			            	HL_ERROR( E_HL_SYNTAX, linenr, return NULL; );
			            }
			           	else if( !s2 ) /* second entry of charmode */
			           	{
			            	s2 = rreadstr( &str );
			            	if( !s2 )
			              	HL_ERROR( E_HL_SYNTAX, linenr, return NULL; );
			              if( strlen(s1) > 1 || strlen(s2) > 1 )
			              	HL_ERROR( E_HL_SYNTAX, linenr, return NULL; );
			              currchar = s1[0];
			              endchar = s2[0];
			              if( currchar > endchar )
			              	HL_ERROR( E_HL_SYNTAX, linenr, return NULL; );
			           	}
			           	else
			           	{
			            	HL_ERROR( E_HL_SYNTAX, linenr, return NULL; );
			            }
			            break;

			case '-':		if( charmode || !s1 )
			            	HL_ERROR( E_HL_SYNTAX, linenr, return NULL; );
			            charmode = TRUE;
			            break;

			case ';':
			case '\0':	if( !s1 || (charmode && !s2) )
			            	HL_ERROR( E_HL_SYNTAX, linenr, return NULL; );
			            nextpos = NULL;
			            return s1;
			case ',':		if( !s1 || (charmode && !s2) )
			            	HL_ERROR( E_HL_SYNTAX, linenr, return NULL; );
									str++;
									if( !*str )
										HL_ERROR( E_HL_SYNTAX, linenr, return NULL; );
			            nextpos = str;
                  return s1;
			case ' ':
			case '\t':	break;

			default:		if( strnicmp( str, "EOL", 3 ) == 0 )
		              {
		              	*flags |= RULEF_EOL;
		              	nextpos=str+3;
		              	if( !*nextpos )
		              		nextpos=NULL;
		              	return "";
		              }
			            HL_ERROR( E_HL_SYNTAX, linenr, return NULL; );
		}
		str++;
	}


}


/* Parse a rule. Set the according flags and add an endrule,
 * if needed.
 */
static BOOLEAN parse_rule( TXTRULE *txtrule, RULE *rule, char *keywords, RULETYPE ruletype,
                        int linenr, RESTYPE curr_colidx )
{
	char *st;
	int flags;
	BOOLEAN caseindiff = !(txtrule->flags & TXTRULEF_CASE);

	switch( ruletype )
	{
		case RULE_TO:
		case RULE_WHILE:
			if( !rule->type )
				HL_ERROR( E_HL_TOFROM, linenr, return TRUE; );
			if( rule->type == RULE_KEYWORD )
				HL_ERROR( E_HL_MIXED, linenr, return TRUE; );
			if( rule->type == RULE_FROM )
			{
				int i;

				if( !rule->link )
					rule->link = (RULE *) HL_CALLOC( 1, sizeof( RULE ));
				if( !rule->link )
					HL_MEM_ERROR( return FALSE; );

				for( i=COLOR2; i<=COLOR256; i++ )
				{
					rule->link->style[i].attribs = 0;
					rule->link->style[i].color = -1;
					rule->link->style[i].selcolor = -1;
				}
				rule->link->quotechar = rule->quotechar;
				rule->link->style[curr_colidx].color = rule->style[curr_colidx].color;
				rule->link->style[curr_colidx].selcolor = rule->style[curr_colidx].selcolor;
				rule->link->style[curr_colidx].attribs = rule->style[curr_colidx].attribs;
				rule = rule->link;
				if( rule->type && rule->type != ruletype )
					HL_ERROR( E_HL_MIXED, linenr, return TRUE; );
			}
			break;
		case RULE_KEYWORD:
			if( rule->type && rule->type != RULE_KEYWORD )
				HL_ERROR( E_HL_MIXED, linenr, return TRUE; );
			break;
		case RULE_FROM:
			if( rule->type && rule->type != ruletype )
				HL_ERROR( E_HL_MIXED, linenr, return TRUE; );
			break;
	}

	rule->type = ruletype;
	if( caseindiff )        /* case indifferent languages always have lower keywords */
		strlwr( keywords );

	for( st=ruletok( keywords, &flags, linenr ); st; st=ruletok( NULL, &flags, linenr ) )
	{
		rule->flags |= flags;
		if( strlen( st ) )
		{
			if( ruletype == RULE_FROM || ruletype == RULE_KEYWORD )
			{
				txtrule->kwstartchar[ (int) st[0] ] = TRUE;
				if( caseindiff )
					txtrule->kwstartchar[ toupper(st[0]) ] = TRUE;
			}
			else
			{
				txtrule->kwendchar[ (int) st[0] ] = TRUE;
				if( caseindiff )
					txtrule->kwendchar[ toupper(st[0]) ] = TRUE;
			}
			if( strlen( st ) > 1 )
			{
				if( !append_stringentry( &rule->kwstring[st[0]], st, linenr ))
					return FALSE;
				/* upper stringentries for case indifferent languages are set
				   as pointers in check_rules() lateron */
			}
			else
			{
				rule->kwsinglechar[ (int) st[0] ] = TRUE;
				if( caseindiff )
					rule->kwsinglechar[ toupper(st[0]) ] = TRUE;
			}
			rule->kwchar[ (int) st[0] ] = TRUE;
			if( caseindiff )
				rule->kwchar[ toupper(st[0]) ] = TRUE;
		}
	}
	return TRUE;
}

/* read a boolean */
static void read_boolflag( int *var, int flag, char *str, int linenr )
{
	if( stricmp( str, "TRUE" ) == 0 )
		*var |= flag;
	else if( stricmp( str, "FALSE" ) == 0 )
		*var &= ~flag;
	else
   	HL_ERROR( E_HL_SYNTAX, linenr, return; );
}

/* parse attributes like <BOLD>, <ITALIC> */
static void parse_attribs( RULE *rule, char *attribs, int linenr, RESTYPE curr_colidx )
{
	char *st = (char*)strtok( attribs, "," );

	if( !st )
   	HL_ERROR( E_HL_SYNTAX, linenr, return; );
	while( st )
	{
		str_xtrim( st, " " );
		if( stricmp( st, "bold" ) == 0 )
			rule->style[curr_colidx].attribs |= HL_BOLD;
		else if( stricmp( st, "italic" ) == 0 )
			rule->style[curr_colidx].attribs |= HL_ITALIC;
		else if( stricmp( st, "light" ) == 0 )
			rule->style[curr_colidx].attribs |= HL_LIGHT;
		else if( stricmp( st, "none" ) == 0 )
			rule->style[curr_colidx].attribs = 0;
		else
   		HL_ERROR( E_HL_SYNTAX, linenr, return; );
   	st = strtok( NULL, "," );
	}
	if( rule->link )
		rule->link->style[curr_colidx].attribs = rule->style[curr_colidx].attribs;

}

/* atoi with check for min/max and error if wrong value.
   returns -1 if wrong value */
static int getvalue( char *st, int minval, int maxval, int linenr )
{
	int ret = atoi( st );

	if( ret < minval || ret > maxval )
		HL_ERROR( E_HL_WRONGVAL, linenr, return -1; );

	return ret;
}

/* parse a complete line of the *.syn-file */
static BOOLEAN parse_line( char *linebuf, int linenr, char *filename, BOOLEAN settingsonly )
{
	static TXTRULE *trule = NULL;
	static RULE *rule = NULL;
	char *keyword;
	char *value;
	static RESTYPE curr_colidx;
	static BOOLEAN duptext = FALSE;

	str_xtrim( linebuf, " \n\r" );
	if( !linebuf[0] || linebuf[0] == ';' )
		return TRUE;

	/* new text rule */
	if( linebuf[ 0 ] == '[' )
	{
		str_xtrim( linebuf, "[]" );
		if( settingsonly )
		{
			trule = txtrule_by_name( linebuf );
			if( !trule )
				HL_ERROR( E_HL_UNKNTEXT, linenr, return TRUE; );
			trule->checked = FALSE;
		}
		else
		{
			if( txtrule_by_name( linebuf ) != NULL )
			{
				trule = NULL;
				duptext = TRUE;
				HL_ERROR( E_HL_DUPTEXT, linenr, return TRUE; );
			}
			duptext = FALSE;
			LIST_APPEND( TXTRULE, trule, txtruleanchor );
			if( memory_error )
				return FALSE;
			trule->name = strdup( linebuf );
			trule->filename = strdup( filename );
			if( !trule->name || !trule->filename )
				HL_MEM_ERROR( return FALSE; );
			trule->flags = TXTRULEF_ACTIVE;
			trule->checked = FALSE;
		}
		rule = NULL;
		return TRUE;
	}

	if( (duptext || settingsonly) && !trule )
		return TRUE;

	/* new rule */
	if( linebuf[ 0 ] == '<' )
	{
		int i;
		if( !trule )
			HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );
		str_xtrim( linebuf, "<>" );
		if( settingsonly )
		{
			rule = rule_by_name( trule, linebuf );
			if( !rule )
				HL_ERROR( E_HL_UNKNRULE, linenr, return TRUE; );
			return TRUE;
		}
		else
		{
			BOOLEAN dup = rule_by_name( trule, linebuf ) != NULL;
			LIST_APPEND( RULE, rule, trule->rules );
			if( memory_error )
				return FALSE;
			rule->dup = dup;
			rule->name = strdup( linebuf );
			rule->quotechar = '\0';
			if( !rule->name )
				HL_MEM_ERROR( return FALSE; );
			for( i=COLOR2; i<=COLOR256; i++ )
			{
				rule->style[i].attribs = 0;
				rule->style[i].color = -1;
				rule->style[i].selcolor = -1;
			}
		}
		curr_colidx = COLOR256;                 /* Preset */
		return TRUE;
	}

	if( stricmp( linebuf, "(color2)" ) == 0)
	{
		if( !rule )
			HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );
		curr_colidx = COLOR2;
		return TRUE;
	}
	else if( stricmp( linebuf, "(color16)" ) == 0)
	{
		if( !rule )
			HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );
		curr_colidx = COLOR16;
		return TRUE;
	}
	else if( stricmp( linebuf, "(color256)" ) == 0)
	{
		if( !rule )
			HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );
		curr_colidx = COLOR256;
		return TRUE;
	}

	value = strchr( linebuf,'=' );
	if( !value || value == linebuf )
		HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );
	*value = '\0';
	value++;
	str_xtrim( linebuf, " " );
	str_xtrim( value, " " );
	keyword = linebuf;

	if( txtruleanchor == NULL )
		HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );

	if( !settingsonly && stricmp( keyword, "Txttype" ) == 0)
	{
		char *st = rreadstr( &value );

		if( rule )
			HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );

		if( !st )
			HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );
		while( st )
		{
			append_stringentry( &trule->txttypes, st, linenr );
			value++;
			if( !*value )
				return TRUE;
			str_xtrim( value, " " );
			if( *value != ',' )
				HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );
			value++;
			str_xtrim( value, " " );
			if( *value != '\"' )
				HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );
			st = rreadstr( &value );
		}
		return TRUE;
	}
	else if( !settingsonly && stricmp( keyword, "CaseSensitive" ) == 0)
	{
		if( rule )
			HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );
		read_boolflag( &trule->flags, TXTRULEF_CASE, value, linenr );
		return TRUE;
	}
	else if( !settingsonly && stricmp( keyword, "Token" ) == 0)
	{
		char *st = rreadstr( &value );
		if( rule || !st || *(++value) )
			HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );

		while( *st )
			trule->token[ *(st++) ] = TRUE;
		return TRUE;
	}
	else if( stricmp( keyword, "Active" ) == 0)
	{
		if( rule )
			HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );
		read_boolflag( &trule->flags, TXTRULEF_ACTIVE, value, linenr );
		return TRUE;
	}

	if( !rule )
		if( settingsonly )
			return TRUE;
		else
			HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );

	if( !settingsonly && stricmp( keyword, "keyword" ) == 0)
	{
		if( !parse_rule( trule, rule, value, RULE_KEYWORD, linenr, curr_colidx ))
			return FALSE;
	}
	else if( !settingsonly && stricmp( keyword, "from" ) == 0)
	{
		if( !parse_rule( trule, rule, value, RULE_FROM, linenr, curr_colidx ))
			return FALSE;
	}
	else if( !settingsonly && stricmp( keyword, "to" ) == 0)
	{
		if( !parse_rule( trule, rule, value, RULE_TO, linenr, curr_colidx ))
			return FALSE;
	}
	else if( !settingsonly && stricmp( keyword, "while" ) == 0)
	{
		if( !parse_rule( trule, rule, value, RULE_WHILE, linenr, curr_colidx ))
			return FALSE;
	}
	else if( stricmp( keyword, "color" ) == 0)
	{
		switch( curr_colidx )
		{
			case COLOR2:   rule->style[curr_colidx].color = getvalue( value, -1, 1, linenr );   break;
			case COLOR16:  rule->style[curr_colidx].color = getvalue( value, -1, 15, linenr );  break;
			case COLOR256: rule->style[curr_colidx].color = getvalue( value, -1, 255, linenr ); break;
		}
		if( rule->link )
			rule->link->style[curr_colidx].color = rule->style[curr_colidx].color;
	}
	else if( stricmp( keyword, "selcolor" ) == 0)
	{
		switch( curr_colidx )
		{
			case COLOR2:   rule->style[curr_colidx].selcolor = getvalue( value, -1, 1, linenr );   break;
			case COLOR16:  rule->style[curr_colidx].selcolor = getvalue( value, -1, 15, linenr );  break;
			case COLOR256: rule->style[curr_colidx].selcolor = getvalue( value, -1, 255, linenr ); break;
		}
		if( rule->link )
			rule->link->style[curr_colidx].selcolor = rule->style[curr_colidx].selcolor;
	}
	else if( stricmp( keyword, "attribs" ) == 0)
		parse_attribs( rule, value, linenr, curr_colidx );
	else if( !settingsonly && stricmp( keyword, "nested" ) == 0)
		read_boolflag( &rule->flags, RULEF_NESTED, value, linenr );
	else if( !settingsonly && stricmp( keyword, "firstcolumn" ) == 0)
		read_boolflag( &rule->flags, RULEF_FIRSTCOLUMN, value, linenr );
	else if( !settingsonly && stricmp( keyword, "Quotechar" ) == 0)
	{
		char *st = rreadstr( &value );
		if( !st || !*st || *(++value) || strlen( st ) > 1 )
			HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );
		rule->quotechar = st[0];
		if( rule->link )
			rule->link->quotechar = rule->quotechar;
	}
	/* ... */
	else
	{
		HL_ERROR( E_HL_SYNTAX, linenr, return TRUE; );
	}
	return TRUE;
}

/* count the number of stringentries in a list */
static int count_stringentry( STRINGENTRY *strentry )
{
	int count=0;
	while( strentry )
	{
		count++;
		strentry=strentry->next;
	}
	return count;
}

/* a simple bubblesort for stringentries;
 * the longest strings must reside at the beginning of the list,
 * because otherwise e.g. if you search for 'btst' and have
 * stringentries of 'bt' and 'btst', 'bt' would be found
 */
static void sort_stringentry( STRINGENTRY *start_se )
{
	STRINGENTRY *strentry;
	int i,j;
	int count = count_stringentry( start_se );
	BOOLEAN noswap;

	for(i=0; i<count-1; i++)
	{
		strentry=start_se;
		noswap = TRUE;
		for(j=i; j<count-1; j++)
		{
 			if( strentry->len < strentry->next->len )
			{
				char *t = strentry->name;
				int tl = strentry->len;
				strentry->name = strentry->next->name;
				strentry->len = strentry->next->len;
				strentry->next->name = t;
				strentry->next->len = tl;
				noswap = FALSE;
			}
			strentry = strentry->next;
		}
		if( noswap )
			return;
	}
}



/* do some work which must be done after the loading of the syntax file*/
static void check_rules( BOOLEAN settingsonly )
{
	static TXTRULE *trule;
	static RULE *rule;
	RULE *duprule;
	int i;


	for( trule=txtruleanchor; trule; trule=trule->next )
	{
		if( trule->checked )
			continue;
		for( rule=trule->rules; rule; rule=rule->next )
		{
			if( !settingsonly )
			{
			/* case indifferent languages need the search strings also on upper positions */
			if( !(trule->flags & TXTRULEF_CASE) )
				for( i='a'; i<='z'; i++ )
					rule->kwstring[toupper(i)] = rule->kwstring[i]; /* set it as pointers to the lower case strings */

			/* stuff for count rules (e.g. nested comments) */
			if( (rule->flags & RULEF_NESTED)
			&&   rule->link)
			{
				rule->link->flags |= RULEF_NESTED;
				rule->link->link = rule; /* point back to start rule */
				for( i=0; i<256; i++ )   /* nested rules need to check not only for the */
					if( rule->kwchar[i] )  /* end chars in an end rule but also for the start chars! */
						rule->link->kwchar[i] = TRUE;
			}

			/* quotechars */
			if( rule->link && rule->link->quotechar ) /* set quote char in filter (otherwise it wouldn't */
				rule->link->kwchar[rule->link->quotechar] = TRUE; /* be recognized) */

			/* firstcolumn */
			if( (rule->flags & RULEF_FIRSTCOLUMN)
			&&   rule->link)
				rule->link->flags |= RULEF_FIRSTCOLUMN;

			/* sort stringentries (see sort_stringentry() for cause) */
			for( i=0; i<256; i++ )
				sort_stringentry( rule->kwstring[i] );
			if( rule->link )
				for( i=0; i<256; i++ )
					sort_stringentry( rule->link->kwstring[i] );
			}
			/* duplicate rules - only the first rules color and attribs are valid */
			if( rule->dup )
			{
				duprule = rule_by_name( trule, rule->name );
				rule->attribs = duprule->attribs;
				rule->color = duprule->color;
				rule->selcolor = duprule->selcolor;
			}
			else
			{
				/* set attribs and color according to current resolution */
				rule->attribs = rule->style[colidx].attribs;
				rule->color = rule->style[colidx].color;
				rule->selcolor = rule->style[colidx].selcolor;
			}

			if( rule->link )
			{
				rule->link->attribs = rule->attribs;
				rule->link->color = rule->color;
				rule->link->selcolor = rule->selcolor;
				rule->link->dup = rule->dup;
			}
			trule->checked = TRUE;
		}
	}

}

/******************************************************************************
 * Hl_read_syn()
 * read the *.syn file in <filenname>
 * returns 0 when error.
 ******************************************************************************/

int Hl_ReadSyn( char *filename, int curr_planes, int settingsonly )
{
	FILE *synfile;
	char linebuf[ 256 ];
	int linenr=1;

	if( curr_planes >= 8 )
		colidx = COLOR256;
	else if( curr_planes >= 4 )
		colidx = COLOR16;
	else
		colidx = COLOR2;

	if( (synfile = fopen( filename, "r" )) == NULL )
		HL_ERROR( E_HL_WRONGFILE, 0, return FALSE; );

	curr_synfile = filename;

	while( fgets( linebuf, 256, synfile ))
		if( !parse_line( linebuf, linenr++, filename, (BOOLEAN) settingsonly ))
			break; /* fatal error */

	curr_synfile = NULL;

	fclose( synfile );
	check_rules( settingsonly );

	return TRUE;
}



/******************************************************************************/
/* subroutines for Hl_write_syn()
/******************************************************************************/

/* fprint all STRINGENTRY structs in <stringentry> vector with prefix in <prefix>
 * eg. <keyword="auto","break","case">
 */
static void fprint_kwstring( FILE *file, char *prefix, STRINGENTRY *kwstring[256], BOOLEAN iscase )
{
	int i;
	BOOLEAN printit = FALSE;
	char buffer[ 80 ];
	char quotebuffer[ 160 ];
	STRINGENTRY *strentry;

	strcpy( buffer, prefix );
	for( i=0; i<256; i++ )
	{
		strentry = kwstring[i];
		if( !iscase && tolower(i) != i && kwstring[tolower(i)] )
			continue;
		while( strentry )
		{
			printit = TRUE;
			if( strlen( buffer ) + strlen( strentry->name ) + 4 > 76 )
			{
				str_xtrim( buffer, "," );
				fprintf( file, "%s\n", buffer );
				strcpy( buffer, prefix );
			}
			sprintf( buffer, "%s\"%s\",", buffer, str_expand_quote( quotebuffer, strentry->name ));
			strentry = strentry->next;
		}
	}
	if( printit )
	{
		str_xtrim( buffer, "," );
		fprintf( file, "%s\n", buffer );
		strcpy( buffer, prefix );
	}
}

/* fprint all single chars in <kwchar> with <prefix>
 * e.g. <while="0"-"9","a"-"f","A"-"F">
 */
static void fprint_singlechars( FILE *file, char *prefix, char kwchar[256] )
{
	char c1,c2;
	int charidx = 1;
	char buffer[ 80 ];
	char quotebuf1[ 3 ], quotebuf2[ 3 ];
	BOOLEAN printit;

	while( charidx < 256 )
	{
		c1 = '\0'; c2 = '\0';
		printit = FALSE;
		sprintf( buffer,"%s", prefix );

		while( strlen( buffer ) + 11 <= 76 && charidx < 256 )
		{
			if( kwchar[ charidx ] )
			{
				printit = TRUE;
				c1 = (char) charidx;
				while( charidx < 255 && kwchar[ charidx+1 ] )
					c2 = (char) ++charidx;
				if( c2 == c1+1 )
				{
					c2 = '\0';
					--charidx;
				}
				if( c2 )
					sprintf( buffer, "%s\"%s\"-\"%s\",",
					         buffer,
					         chr_expand_quote( quotebuf1, c1),
					         chr_expand_quote( quotebuf2, c2) );
				else
					sprintf( buffer, "%s\"%s\",",buffer, chr_expand_quote( quotebuf1, c1 ) );
				c2 = '\0';
			}
			charidx++;
		}
		if( printit )
		{
			str_xtrim( buffer, "," );
			fprintf( file, "%s\n", buffer );
		}
	}
}

/* fprint all STRINGENTRY structs in <anchor> with prefix in <prefix>
 * eg. <keyword="auto","break","case">
 */
static void fprint_stringentry( FILE *file, char *prefix, STRINGENTRY *anchor )
{
	STRINGENTRY *strentry = anchor;
	char buffer[ 80 ];
	char quotebuffer[ 160 ];

	while( strentry )
	{
		strcpy( buffer, prefix );
		while( strentry && strlen( buffer ) + strlen( strentry->name ) + 4 <= 76 )
		{
			sprintf( buffer, "%s\"%s\",", buffer, str_expand_quote( quotebuffer, strentry->name ));
			strentry = strentry->next;
		}
		str_xtrim( buffer, "," );
		fprintf( file, "%s\n", buffer );
	}
}

/* fprint attributes */
static void	fprint_attribs( FILE *file, HL_ELEM attribs, BOOLEAN settingsonly  )
{
	char buffer[ 80 ];
	if( !settingsonly && !attribs )
		return;
	strcpy( buffer, "Attribs =" );
	if( settingsonly && !attribs )
		strcat( buffer, " NONE" );
	if( attribs & HL_BOLD )
		strcat( buffer, " BOLD," );
	if( attribs & HL_ITALIC )
		strcat( buffer, " ITALIC," );
	if( attribs & HL_LIGHT )
		strcat( buffer, " LIGHT" );
	str_xtrim( buffer, "," );
	fprintf( file, "%s\n", buffer );
}

/* fprint a complete rule with all keywords
 */
static void fprint_rule( FILE *file, RULE *rule, BOOLEAN iscase, BOOLEAN settingsonly )
{
	char *prefix;
	char buffer[80];
	char quotebuffer[ 160 ];
	int i;

	if( !settingsonly )
	{
		switch( rule->type )
		{
			case RULE_FROM:    prefix = "From = ";    break;
			case RULE_TO:      prefix = "To = ";      break;
			case RULE_WHILE:   prefix = "While = ";   break;
			case RULE_KEYWORD: prefix = "Keyword = "; break;
		}

		if( rule->flags & RULEF_EOL )
			fprintf( file, "%sEOL\n", prefix );

		fprint_kwstring( file, prefix, rule->kwstring, iscase );
		fprint_singlechars( file, prefix, rule->kwsinglechar );
	}
	if( rule->type != RULE_FROM )
	{
		if( !settingsonly )
		{
			if( rule->type == RULE_TO )
			{
				if( rule->flags & RULEF_NESTED )
					fprintf( file, "Nested = TRUE\n" );
				if( rule->quotechar )
					fprintf( file, "Quotechar = \"%s\"\n", chr_expand_quote( quotebuffer, rule->quotechar ) );
			}
				if( rule->flags & RULEF_FIRSTCOLUMN )
					fprintf( file, "Firstcolumn = TRUE\n" );
		}

		if( rule->dup )
			return;

		for( i=COLOR2; i<=COLOR256; i++ )
		{
			if( !settingsonly )
			{
				if( !rule->style[i].attribs
				&&  rule->style[i].color == -1
				&&  rule->style[i].selcolor == -1 )
					continue;
			}
			switch( i )
			{
				case COLOR2:   fprintf( file, "(Color2)\n");   break;
				case COLOR16:  fprintf( file, "(Color16)\n");  break;
				case COLOR256: fprintf( file, "(Color256)\n"); break;
			}
			fprint_attribs( file, rule->style[i].attribs, settingsonly );
			if( settingsonly || rule->style[i].color != -1 )
			{
				sprintf( buffer, "Color = %li", rule->style[i].color );
				fprintf( file, "%s\n", buffer );
			}
			if( settingsonly || rule->style[i].selcolor != -1 )
			{
				sprintf( buffer, "Selcolor = %li", rule->style[i].selcolor );
				fprintf( file, "%s\n", buffer );
			}
		}
	}
}


/* copy the current settings of attribs and colors to the
 * <style> array for saving the values in <style>
 */
static void set_style( void )
{
	TXTRULE *trule;
	RULE *rule;

	for( trule=txtruleanchor; trule; trule=trule->next )
	{
		/* set attribs and color according to current resolution */
		for( rule=trule->rules; rule; rule=rule->next )
		{
			rule->style[colidx].attribs = rule->attribs;
			rule->style[colidx].color = rule->color;
			rule->style[colidx].selcolor = rule->selcolor;
			if( rule->link )
			{
				rule->link->style[colidx].attribs = rule->link->attribs;
				rule->link->style[colidx].color = rule->link->color;
				rule->link->style[colidx].selcolor = rule->link->selcolor;
			}
		}
	}

}


/******************************************************************************
 * Hl_write_syn()
 * write the *.syn file in <filename>
 * returns 0 when error.
 ******************************************************************************/

int Hl_WriteSyn( char *filename, int settingsonly )
{
	FILE *synfile;
	TXTRULE *trule = txtruleanchor;
	RULE *rule;
	char quotebuffer[ 256 ];
	char buffer[ 256 ];
	int i, j;

	if( filename )
		if( (synfile = fopen( filename, "w" )) == NULL
		||  memory_error )
			return FALSE;

	set_style();

	while( trule )
	{
		if( !filename )
			if( (synfile = fopen( trule->filename, "w" )) == NULL
			||  memory_error )
				return FALSE;

		fprintf( synfile, "\n;********************************************************\n" );
		fprintf( synfile, "[%s]\n",trule->name );
		fprintf( synfile, ";********************************************************\n" );
		if( !settingsonly )
			fprint_stringentry( synfile, "Txttype = ", trule->txttypes );
		fprintf( synfile, "Active = %s\n",trule->flags & TXTRULEF_ACTIVE ? "TRUE" : "FALSE" );
		if( !settingsonly )
		{
			fprintf( synfile, "CaseSensitive = %s\n",trule->flags & TXTRULEF_CASE ? "TRUE" : "FALSE" );
			memset( buffer, 0, 256 );
			for( i=1, j=0; i<256; i++ )
				if( trule->token[i] )
					buffer[j++]=(char) i;
			fprintf( synfile, "Token = \"%s\"\n\n", str_expand_quote( quotebuffer, buffer ) );
		}
		rule=trule->rules;
		while( rule )
		{
			if( !(settingsonly && rule->dup) )
			{
				fprintf( synfile, "\n<%s>\n", rule->name );
				fprint_rule( synfile, rule, trule->flags & TXTRULEF_CASE, settingsonly );
				if( rule->link )
					fprint_rule( synfile, rule->link, trule->flags & TXTRULEF_CASE, settingsonly );
			}
			rule=rule->next;
		}
		if( !filename )
			fclose( synfile );
		trule = trule->next;
	}

	if( filename )
		fclose( synfile );

	return TRUE;
}


/******************************************************************************
 *
 * Routines for handling the syntax cache
 * Hl_Update(), Hl_InsertLine(), Hl_RemoveLine(), Hl_GetLine(), Hl_New(),
 * Hl_Free()
 *
 ******************************************************************************/


/******************************************************************************/
/* subroutines for Hl_Update()
/******************************************************************************/

/* check and fix - if necessary - the size of the linebuffer in
 * which the format infos are saved temporary
 */
static BOOLEAN fix_buffsize( void )
{
	if( bufflen + sizeof( HL_ELEM ) * 4 < maxbufflen )
		return TRUE;
	if( !maxbufflen )
	{
		maxbufflen = BUFBLK;
		linebuffer = (HL_LINE) HL_MALLOC( maxbufflen );
	}
	else
	{
		maxbufflen += BUFBLK;
		linebuffer = (HL_LINE) HL_REALLOC( linebuffer, maxbufflen );
	}
	if( !linebuffer )
		HL_MEM_ERROR( return FALSE; );

	return TRUE;
}

/* add a rule to the linebuffer */
static BOOLEAN add_to_linebuffer( RULE *rule, int len )
{
	if( !fix_buffsize() )
		return FALSE;

	if( rule )
	{
		linebuffer[ bufflen++ ]=    rule->attribs
		                     | (rule->color != -1 ? HL_COLOR : 0)
		                     | (rule->selcolor != -1 ? HL_SELCOLOR : 0);
		linebuffer[ bufflen++ ]= len;
		if( rule->color != -1 )
			linebuffer[ bufflen++ ] = rule->color;
		if( rule->selcolor != -1 )
			linebuffer[ bufflen++ ]= rule->selcolor;
	}
	else
	{
		linebuffer[ bufflen++ ]= 0;
		linebuffer[ bufflen++ ]= len;
	}
	return TRUE;
}

/* get next token to explore */
static char *next_token( char *st, char token[256], char search[256] )
{
	for(;;)
	{
		if( token[*st] )
			for(;token[*st];st++);
		else
			st++;
		if( !*st || search[*st] )
			return( st );
	}
}

/* compare string for equality; return NULL if
 * not equal, pointer to string if equal
 */
static char *str_xeq( char *s1, char *s2 )
{
	char *checkst = s1;
	while( *checkst && *s2 )
		if( *(checkst++) != *(s2++) )
			return NULL;
	return *s2 ? NULL : checkst;
}

/* same 'semi' case indifferent; s2 must be lower case */
static char *str_xieq( char *s1, char *s2 )
{
	char *checkst = s1;
	while( *checkst && *s2 )
		if( tolower(*(checkst++)) != *(s2++) )
			return NULL;
	return *s2 ? NULL : checkst;
}

/* the main function for interpreting a source line.
 * Searches <st> for matching rules. If an end rule is searched,
 * (e.g. in the previous line was a comment start without matching
 * comment end) the rule is to be handed over in <rule>. create_cachetok()
 * searches for this rule then. Else, NULL is to be handed over in <rule>;
 * create_cachetok() searches for a keyword- or from-rule.
 * <txtrule> is to be handed over to access the txtrule flags.
 * It returns the rule which is active at the end of the line in <newrule>
 * (or NULL if no rule is active). It creates the format infos in a buffer
 * and at the end duplicates it and returns a pointer to the duplicated
 * buffer. In a case of mem error it calls the error display callback,
 * sets the global var memory_error to TRUE (check it after create_cachetok!)
 * and returns NULL.
 */
static HL_LINE create_cachetok( char *st, RULE *rule, RULE **newrule, int count, int *newcount, TXTRULE *txtrule )
{
	HL_LINE retline;
	char *nextst;
	STRINGENTRY *found;
	char *lastst = st;
	char *startst = st;
	char *(*cmpfunc)(char *s1, char *s2);

	if( !(txtrule->flags & TXTRULEF_ACTIVE))
	{
		*newrule = NULL;
		return NULL;
	}
	cmpfunc = (txtrule->flags & TXTRULEF_CASE) ? str_xeq : str_xieq;
	bufflen = 0;

	for( ;; )
	{
		if( !*st )
			break;
		found = NULL;

		/* search for a matching "to" or "while" rule */
		if( rule )
		{
			/* if quotechar of this rule found, skip the following char */
			if( *st == rule->quotechar )
			{
				if( !(*(++st)) )
					break;
				++st;
				continue;
			}

			switch( rule->type )
			{
				case RULE_TO:                       /* search for the first char or string matching */
					if( rule->kwchar[st[0]])
					{
						/* search for "from" rule when a nested rule is running */
						if( rule->flags & RULEF_NESTED )
						{
							LIST_FINDTERM( found, rule->link->kwstring[st[0]],
						  	                 ((nextst = cmpfunc( st, found->name )) != NULL));
							if( found
							||  rule->link->kwsinglechar[st[0]])
							{
								count++;
								if( found )
									st = nextst;
								else
									st++;
								goto continue_outer_loop;
							}
						}

						LIST_FINDTERM( found, rule->kwstring[st[0]],
					  	                 ((nextst = cmpfunc( st, found->name )) != NULL));
						if( found
						||  rule->kwsinglechar[st[0]])
						{
							if( found )
								st = nextst;
							else
								st++;
							if( count )
							{
								count--;
								goto continue_outer_loop;
							}
							if( !add_to_linebuffer( rule, (int) (st-lastst) ))
								HL_MEM_ERROR( return NULL; );
							lastst = st;
							rule = NULL;
							goto continue_outer_loop;
						}
						st = next_token( st, txtrule->token, rule->kwchar );
						goto continue_outer_loop;
					}
					else
					{
						st = next_token( st, txtrule->token, rule->kwchar );
						goto continue_outer_loop;
					}

				case RULE_WHILE:                    /* search for the first NOT matching! */
					if( !rule->kwchar[st[0]])
					{
						if( !add_to_linebuffer( rule, (int) (st-lastst) ))
							HL_MEM_ERROR( return NULL; );
						lastst = st;
						rule = NULL;
						goto continue_outer_loop;
					}
					LIST_FINDTERM( found, rule->kwstring[st[0]],
					 	             ((nextst = cmpfunc( st, found->name )) != NULL));
					if( found
					||  rule->kwsinglechar[st[0]])
					{
						st++;
						goto continue_outer_loop;
					}

					if( !add_to_linebuffer( rule, (int) (st-lastst) ))
						HL_MEM_ERROR( return NULL; );
					lastst = st;
					rule = NULL;
					goto continue_outer_loop;

			}
		}

		/* no rule active, search for a "from" or "keyword" rule */
		if( txtrule->kwstartchar[st[0]])
		{
			RULE *search_rule;
			for( search_rule = txtrule->rules; search_rule; search_rule = search_rule->next )
			{
				LIST_FINDTERM( found, search_rule->kwstring[st[0]],
				  	           ((nextst = cmpfunc( st, found->name )) != NULL));
				if( found
				||  search_rule->kwsinglechar[st[0]])
				{
					/* if char after found string or char is valid token (e.g. a function called goto_loop(), */
					/* get next token (only for keywords) */
					if( search_rule->type == RULE_KEYWORD )
					{
						if( (   found
						     && txtrule->token[*(nextst-1)]
						     && txtrule->token[*nextst] )
						||  (   !found
						     && txtrule->token[st[0]]
						     && txtrule->token[st[1]]))
						{
							st = next_token( st, txtrule->token, txtrule->kwstartchar );
							goto continue_outer_loop;
						}
					}

					if( (search_rule->flags & RULEF_FIRSTCOLUMN) && st != startst ) /* if rule only valid in first column and it isn^t first column */
					{
						st = next_token( st, txtrule->token, txtrule->kwstartchar );
						goto continue_outer_loop;
					}

					if( st-lastst )
					{
						if( !add_to_linebuffer( NULL, (int) (st-lastst) ))
							HL_MEM_ERROR( return NULL; );
						lastst = st;
					}
					if( found )
						st = nextst;
					else
						st++;
					if( !search_rule->link )
					{
						if( !add_to_linebuffer( search_rule, (int) (st-lastst) ))
							HL_MEM_ERROR( return NULL; );
						lastst = st;
					}
					rule = search_rule->link;
					goto continue_outer_loop;
				}
			}
			st = next_token( st, txtrule->token, txtrule->kwstartchar );
			goto continue_outer_loop;
		}

		st = next_token( st, txtrule->token, rule ? txtrule->kwendchar : txtrule->kwstartchar );

		continue_outer_loop:;

	}

	if( st-lastst )
		if( !add_to_linebuffer( rule, (int) (st-lastst) ))
			HL_MEM_ERROR( return NULL; );


	if( (retline = HL_MALLOC( sizeof( HL_ELEM) * (bufflen+1) )) == NULL)
		HL_MEM_ERROR( return NULL; );

	retline[ bufflen ] = HL_CACHEEND;
	while( bufflen-- )
		retline[ bufflen ] = linebuffer[ bufflen ];

	if( rule
	&&  (    rule->type == RULE_WHILE
	     || (rule->flags & RULEF_EOL) ))
		rule = NULL;
	*newrule = rule;
	*newcount = count;
	return retline;
}


/******************************************************************************
 * Hl_Update()
 * update a cache line. Must be called every time when your source line changes.
 * <anchor> is the text anchor as handed over by Hl_New(), <handle> is the line
 * handle as handed over by Hl_InsertLine(), <line> is your source line,
 * <updatenext> is a boolean. Hand over 1 if you want the following source lines
 * to be updated (according to rule changes in your source line).
 * The function returns the number of the updated cache lines.
 ******************************************************************************/

long Hl_Update( HL_HANDLE anchor, HL_LINEHANDLE handle, char *line, int updatenext )
{
	CACHEBASE *ca_base = (CACHEBASE *) anchor;
	CACHE *ca = (CACHE *) handle;
	long counter = 1;

	if( memory_error
	||  !anchor )
		return 0L;

	ca->line = line;
	for(;;)
	{
		if( ca->cachetok )
		{
			HL_FREE( ca->cachetok );
			ca->cachetok = NULL;
		}
		if( !(ca_base->txtrule->flags & TXTRULEF_ACTIVE) )
			return 0L;

		ca->cachetok = create_cachetok( ca->line,
		                                              ca->startrule, &ca->endrule,
	  	                                            ca->startcount, &ca->endcount,
	  	                                            ca_base->txtrule );
	 	if( memory_error )
	 		return 0L;
	 	if( !updatenext
	 	||  !ca->next
	 	||  (   ca->endrule == ca->next->startrule
	 	     && ca->endcount == ca->next->startcount))
	 		break;
	 	ca->next->startrule = ca->endrule;
	 	ca->next->startcount = ca->endcount;
	 	ca = ca->next;
	 	counter++;
	}
	return counter;
}


/******************************************************************************
 * Hl_InsertLine()
 * Insert a cache line.
 * <anchor> is the text anchor as handed over by Hl_New(), <prev> is the line
 * handle of the previous line, <line> is your source line,
 * <inserted> returns the handle of the new cache line, <update> is a boolean.
 * Hand over 1 if you want the following source lines
 * to be updated (according to rule changes in your source line).
 * The function returns the number of the updated cache lines.
 ******************************************************************************/

long Hl_InsertLine( HL_HANDLE anchor, HL_LINEHANDLE prev, char *line,
                         HL_LINEHANDLE *inserted, int update )
{
	CACHEBASE *ca_base = (CACHEBASE *) anchor;
	CACHE *prv_ca = (CACHE *) prev;
	CACHE *ca;
	RULE *rule = NULL;
	int count = 0;

	*inserted = NULL;

	if( memory_error
	|| !anchor )
		return 0L;

	if( prv_ca )
	{
		LIST_ADD( CACHE, ca, prv_ca->next );
		if( memory_error )
			return 0L;
		rule = prv_ca->endrule;
		count = prv_ca->endcount;
	}
	else
	{
		LIST_ADD( CACHE, ca, ca_base->cache );
		if( memory_error )
			return 0L;
	}

	if( ca )
	{
		*inserted = (HL_LINEHANDLE) ca;
		ca->startrule = rule;
		ca->startcount = count;
		ca->line = line;
		ca->cachetok = create_cachetok( line,
		                                        rule, &ca->endrule,
		                                        count, &ca->endcount,
		                                        ca_base->txtrule );
		if( memory_error )
			return 0l;

		if( ca->next
		&&  (   ca->next->startrule != ca->endrule
		     || ca->next->startcount != ca->endcount))
		{
			ca->next->startrule = ca->endrule;
			ca->next->startcount = ca->endcount;
			if( update )
				return 1L + Hl_Update( anchor, (HL_LINEHANDLE) ca->next,
			                              ca->next->line, TRUE );
		}

	}
	return 1L;
}


/******************************************************************************
 * Hl_RemoveLine()
 * Remove a cache line.
 * <anchor> is the text anchor as handed over by Hl_New(), <prev> is the line
 * handle of the previous line,
 * <curr> is the handle of the cache line to delete (returned to NULL),
 * <update> is a boolean. Hand over 1 if you want the following source lines
 * to be updated (according to rule changes in your source line).
 * The function returns the number of the updated cache lines.
 ******************************************************************************/

long Hl_RemoveLine( HL_HANDLE anchor, HL_LINEHANDLE prev, HL_LINEHANDLE *curr, int update )
{
	CACHEBASE *ca_base = (CACHEBASE *) anchor;
	CACHE *prv_ca = (CACHE *) prev;
	CACHE *ca = (CACHE *) *curr;
	CACHE *nxt_ca;

	*curr = NULL;

	if( memory_error
	|| !ca_base || !ca_base->cache  )
		return 0L;

	if( !prv_ca )
	{
		ca = ca_base->cache;
		ca_base->cache = ca->next;
		if( ca->next )
		{
			ca->next->startrule = NULL;
			ca->next->startcount = 0;
		}
	}
	else
	{
		ca = prv_ca->next;
		prv_ca->next = ca->next;
		if( ca->next )
		{
			ca->next->startrule = prv_ca->endrule;
			ca->next->startcount = prv_ca->endcount;
		}
	}
	nxt_ca = ca->next;

	if( ca->cachetok )
		HL_FREE( ca->cachetok );
	HL_FREE( ca );
	if( !nxt_ca
	||  !update )
		return 0L;
	return Hl_Update( anchor, (HL_LINEHANDLE) nxt_ca,
	                       nxt_ca->line, TRUE );

}


/******************************************************************************
 * Hl_GetLine()
 * get the cache line for <handle>
 * if there's no cache line for this handle (handle == NULL),
 * the function returns NULL. You will have to do your color and attribs for this
 * source line yourself.
 ******************************************************************************/

HL_LINE Hl_GetLine( HL_LINEHANDLE handle )
{
	if( memory_error
	||  handle == NULL )
		return NULL;
	return (HL_LINE) ((CACHE *) handle)->cachetok;
}

/* subroutine for Hl_New(): find a txtrule by txttype */
static TXTRULE *find_txtrule( char *txttype )
{
	TXTRULE *trule = txtruleanchor;
	STRINGENTRY *found;

	while( trule )
	{
		LIST_FINDISTR( found, trule->txttypes, name, txttype );
		if( found )
			return trule;
		trule = trule->next;
	}
	return NULL;
}


/******************************************************************************
 * Hl_New()
 * create a new text for txttype <txttype>
 * return a text handle. This may be NULL, this doesn't mean
 * an error, just that <txttype> doesnt match with any of the text rules.
 * You may use this NULL handle as a text handle for any of the other functions.
 ******************************************************************************/

HL_HANDLE Hl_New( char *txttype, int resvd )
{
	TXTRULE *trule;
	CACHEBASE *ca_base;

	if( memory_error
	||  !txttype || !txttype[0] )
		return NULL;

	trule = find_txtrule( txttype );
	if( !trule )
		return NULL;

	LIST_ADD( CACHEBASE, ca_base, cacheanchor );
	if( memory_error )
		return 0L;
	if( ca_base )
		ca_base->txtrule = trule;
	return (HL_HANDLE) ca_base;

}


/******************************************************************************
 * Hl_Free()
 * Free a complete text with all of its cache lines
 * <anchor> is the text handle as returned by Hl_New()
 ******************************************************************************/

void Hl_Free( HL_HANDLE anchor )
{
	CACHEBASE *ca_base = (CACHEBASE *) anchor;
	CACHE *ca, *nxt_ca;

	if( memory_error
	||  !ca_base )
		return;

	ca = ca_base->cache;
	LIST_RM( CACHEBASE, ca_base, cacheanchor );

	for( ; ca; ca = nxt_ca )
	{
		nxt_ca = ca->next;
		if( ca->cachetok )
			HL_FREE( ca->cachetok );
		HL_FREE( ca );
	}
}


/******************************************************************************
 *
 * Routines for asking and setting the syntax cache options
 * Hl_EnumTxtNames(), Hl_EnumTxtTypes(), Hl_EnumRules(), Hl_ChangeRule(),
 * Hl_SaveSettings(), Hl_DeleteSaveSettings(), Hl_RestoreSettings(),
 * Hl_TxtIndexByTxttype(), Hl_IsActive(), Hl_SetActive()
 *
 ******************************************************************************/

/* subroutine for syntax cache options:
 * get text rule from list by index
 */
static TXTRULE *txtrule_by_idx( int idx )
{
	int i;
	TXTRULE *trule = txtruleanchor;
	if( !trule )
		return NULL;

	for( i=0; i<idx; i++ )
		if( (trule = trule->next) == NULL )
			return NULL;
	return trule;

}

/* subroutine for syntax cache options:
 * get rule from list by index
 */
static RULE *rule_by_idx( int txtidx, int idx )
{
	TXTRULE *trule;
	RULE *rule;
	int i;

	if( (trule = txtrule_by_idx( txtidx )) == NULL )
		return NULL;
	if( (rule = trule->rules) == NULL )
		return NULL;
	for( i=0; i<idx; i++ )
	{
		if( !rule->next )
			return NULL;
		while( stricmp( rule->name, rule->next->name ) == 0 )
		{
			rule = rule->next;
			if( !rule->next )
				return NULL;
		}
		rule = rule->next;
	}
	return rule;
}


/******************************************************************************
 * Hl_EnumTxtNames()
 * Enum the text rule names (e.g. for building a popup in a dialog)
 * <txtname> returns the text name. <idx> must be set to 0 before the first call.
 * Hl_EnumTxtNames() returns TRUE (1) while txtname is valid.
 ******************************************************************************/

int Hl_EnumTxtNames( char **txtname, int *idx )
{
	TXTRULE *trule = txtruleanchor;

	if( memory_error
	|| (trule = txtrule_by_idx( *idx )) == NULL )
		return FALSE;
	if( txtname )
		*txtname = trule->name;
	(*idx)++;
	return TRUE;
}

/******************************************************************************
 * Hl_EnumTxtTypes()
 * Enum the text rule types (e.g. for building a popup in a dialog)
 * <txtidx> is the index of the text.
 * <txttype> returns the text type. <idx> must be set to 0 before the first call.
 * Hl_EnumTxtTypes() returns TRUE (1) while txttype is valid.
 ******************************************************************************/

int Hl_EnumTxtTypes(  int txtidx, char **txttype, int *idx )
{
	int i;
	TXTRULE *trule;
	STRINGENTRY *type;

	if( memory_error
	|| (trule = txtrule_by_idx( txtidx )) == NULL )
		return FALSE;

	if( (type = trule->txttypes) == NULL )
		return FALSE;
	for( i=0; i<*idx; i++ )
		if( (type = type->next) == NULL )
			return FALSE;

	if( txttype )
		*txttype = type->name;
	(*idx)++;
	return TRUE;
}


/******************************************************************************
 * Hl_EnumRules()
 * Enum rules (e.g. for building a popup in a dialog)
 * <txtidx> is the index of the text.
 * <ri> returns the rule info (see higlite.h for details).
 * <idx> must be set to 0 before the first call.
 * Hl_EnumRules() returns TRUE (1) while ri is valid.
 ******************************************************************************/

int Hl_EnumRules( int txtidx, HL_RULEINFO *ri, int *idx )
{
	int i;
	TXTRULE *trule;
	RULE *rule;

	if( memory_error
	|| (trule = txtrule_by_idx( txtidx )) == NULL )
		return FALSE;

	if( (rule = trule->rules) == NULL )
		return FALSE;
	for( i=0; i<*idx; i++ )
	{
		if( !rule->next )
			return FALSE;
		while( stricmp( rule->name, rule->next->name ) == 0 )
		{
			rule = rule->next;
			if( !rule->next )
				return FALSE;
		}
		rule = rule->next;
	}

	if( ri )
	{
		ri->name = rule->name;
		ri->attribs = rule->attribs
		                    | ((rule->color == -1) ? 0 : HL_COLOR)
		                    | ((rule->selcolor == -1) ? 0 : HL_SELCOLOR);
		ri->color = rule->color;
		ri->selcolor = rule->selcolor;
	}
	(*idx)++;
	return TRUE;
}


/******************************************************************************
 * Hl_DeleteSaveSettings()
 * Release the temporary memory for settings saved by Hl_SaveSettings()
 * Either Hl_DeleteSaveSettings() or Hl_RestoreSettings() _must_ be called
 * if you saved the settings with Hl_SaveSettings(), except when
 * Hl_SaveSettings() returned FALSE!
 ******************************************************************************/

void Hl_DeleteSaveSettings( void )
{
	SAVEINFO *si, *nxt_si;

	if( memory_error )
		return;

	for( si = saveinfo; si; si = nxt_si )
	{
		nxt_si = si->next;
		HL_FREE( si );
	}
	saveinfo = NULL;
}


/******************************************************************************
 * Hl_RestoreSettings()
 * Restore settings which were previously saved by Hl_SaveSettings().
 * All changes by Hl_ChangeRule() are discarded.
 * Either Hl_DeleteSaveSettings() or Hl_RestoreSettings() _must_ be called
 * if you saved the settings with Hl_SaveSettings(), except when
 * Hl_SaveSettings() returned FALSE!
 ******************************************************************************/

void Hl_RestoreSettings( void )
{
	TXTRULE *trule;
	RULE *rule;
	SAVEINFO *si = saveinfo;

	if( memory_error )
		return;

	for( trule = txtruleanchor; trule; trule = trule->next )
	{
		trule->flags &= ~TXTRULEF_ACTIVE;
		for( rule = trule->rules; rule; rule = rule->next, si = si->next )
		{
			if( si->isactive )
				trule->flags |= TXTRULEF_ACTIVE;
			rule->attribs = si->attribs;
			rule->color = si->color;
			rule->selcolor = si->selcolor;
			if( rule->link )
			{
				si = si->next;
				rule->link->attribs = si->attribs;
				rule->link->color = si->color;
				rule->link->selcolor = si->selcolor;
			}
		}
	}
	Hl_DeleteSaveSettings();
}

/******************************************************************************
 * Hl_SaveSettings()
 * Save the current rule settings (attribs and colors) in temporary memory.
 * You may change the rule settings now. If you want to discard the changes
 * (e.g. User presses <Cancel> in a dialog, call Hl_RestoreSettings to discard
 * the changes.
 * HlSaveSettings returns FALSE if it runs out of memory.
 * Either Hl_DeleteSaveSettings() or Hl_RestoreSettings() _must_ be called
 * if you saved the settings with Hl_SaveSettings(), except when
 * Hl_SaveSettings() returned FALSE!
 ******************************************************************************/

int Hl_SaveSettings( void )
{
	TXTRULE *trule;
	RULE *rule;
	SAVEINFO *si;

	if( memory_error )
		return FALSE;

	for( trule = txtruleanchor; trule; trule = trule->next )
	{
		for( rule = trule->rules; rule; rule = rule->next )
		{
			LIST_APPEND( SAVEINFO, si, saveinfo );
			if( !si )
				goto error;
			si->isactive = trule->flags & TXTRULEF_ACTIVE;
			si->attribs = rule->attribs;
			si->color = rule->color;
			si->selcolor = rule->selcolor;
			if( rule->link )
			{
				LIST_APPEND( SAVEINFO, si, saveinfo );
				if( !si )
					goto error;
				si->attribs = rule->link->attribs;
				si->color = rule->link->color;
				si->selcolor = rule->link->selcolor;
			}
		}
	}
	return TRUE;

	error:
	Hl_DeleteSaveSettings();
	return FALSE;
}


/******************************************************************************
 * Hl_ChangeRule()
 * change the attribs and the colors of a rule.
 * <txtidx> is the text rule index as to be found out with Hl_EnumTxtRules(),
 * <idx> is the rule index (Hl_EnumRules()), <ri> must contain the new settings
 * (attribs and color). The member <name> of <ri> is ignored. For more details of
 * HT_RULEINFO see highlite.h
 ******************************************************************************/

int Hl_ChangeRule( int txtidx, int idx, HL_RULEINFO *ri )
{
	RULE *rule;

	if( memory_error
	||  (rule = rule_by_idx( txtidx, idx )) == NULL )
		return FALSE;
	for(;;)
	{
		rule->attribs  = ri->attribs & ~(HL_COLOR | HL_SELCOLOR);
		rule->color    = (ri->attribs & HL_COLOR) ? ri->color : -1;
		rule->selcolor = (ri->attribs & HL_SELCOLOR) ? ri->selcolor : -1;
		if( rule->link )
		{
			rule->link->attribs  = ri->attribs & ~(HL_COLOR | HL_SELCOLOR);
			rule->link->color    = (ri->attribs & HL_COLOR) ? ri->color : -1;
			rule->link->selcolor = (ri->attribs & HL_SELCOLOR) ? ri->selcolor : -1;
		}
		if( !rule->next )
			return TRUE;
		if( stricmp( rule->name, rule->next->name ) != 0 )
			return TRUE;
		rule = rule->next;
	}

}


/******************************************************************************
 * Hl_TxtIndexByTxttype()
 * return the text rule index by text type.
 ******************************************************************************/

int Hl_TxtIndexByTxttype( char *txttype )
{
	TXTRULE *trule;
	STRINGENTRY *found;
	int idx;

	if( memory_error )
		return -1;

	for( trule = txtruleanchor, idx = 0; trule; trule = trule->next, idx++ )
	{
		LIST_FINDISTR( found, trule->txttypes, name, txttype );
		if( found )
			return idx;
	}
	return -1;
}


/******************************************************************************
 * Hl_IsActive()
 * checks if a text rule is active
 ******************************************************************************/

int Hl_IsActive( int txtidx )
{
	TXTRULE *txtrule = txtrule_by_idx( txtidx );
	if( txtrule )
		return txtrule->flags & TXTRULEF_ACTIVE;
	return FALSE;
}


/******************************************************************************
 * Hl_SetActive()
 * set a text rule active or inactive
 ******************************************************************************/

void Hl_SetActive( int txtidx, int active )
{
	TXTRULE *txtrule = txtrule_by_idx( txtidx );
	if( txtrule )
	{
		if( active )
			txtrule->flags |= TXTRULEF_ACTIVE;
		else
			txtrule->flags &= ~TXTRULEF_ACTIVE;
	}
}
