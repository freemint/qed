/* Highlite.c
 * ----------
 * Autor: Heiko Achilles hornsk@talknet.de
 *
 * Flexibles Syntax-Highlighting
 * Dieser Quelltext ist freie Software.
 * Diese Aussage bezieht sich nur auf diesen Quelltext, die dazugeh”rigen Headerdateien
 * und die dazugeh”rige Dokumentation sowie auf die beiliegenden *.syn-Dateien.
 */


#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include "highlite.h"

#include "lists.h"


typedef int boolean;
#define TRUE 1
#define FALSE 0

#define MAX_ACTIVERULES 10

#define RULE_TO      1
#define RULE_WHILE   2
#define RULE_KEYWORD 3
#define RULE_FROM    4

#define TXTRULEF_CASE 1

#define RULEF_EOL 1
#define RULEF_ISSTRING 2
#define RULEF_BLOCKING 4
#define RULEF_COUNTING 8

#define ATTRIBS_BOLD 1
#define ATTRIBS_ITALIC 2

#define E_HL_MEMORY 1
#define E_HL_TOFROM 2
#define E_HL_MIXED  3
#define E_HL_SYNTAX 4

/* error-macro: 2b replaced by your own error handler */
#define ERROR( errn, linen, ret )\
{\
	printf( "hl-Error %i in line %i\n",(errn),(linen) );\
	{ret}\
}

typedef struct stringentry {
	int len;
	char *name;
	struct stringentry *next;
} STRINGENTRY;



typedef struct rule {
	char *name;
	int type;
	int flags;
	int attribs;
	int color;
	STRINGENTRY *kwstring;
	char kwstringchar[256];
	char kwsinglechar[256];
	struct rule *endrule;
	struct rule *next;
} RULE;

typedef struct txtrule {
	char *name;
	STRINGENTRY *extender;
	char token[256];
	char kwstartchar[256];
	char kwendchar[256];
	int flags;
	RULE *rules;
	struct txtrule *next;
} TXTRULE;

typedef struct cacheline {
	char *line;
	RULE *rule;
	char *cachetok;
	struct cacheline *next;
} CACHE;

typedef struct cacheb {
	CACHE *cache;
	TXTRULE *txtrule;
	struct cacheb *next;
} CACHEBASE;


static TXTRULE *txtrulebase = NULL;
static CACHEBASE *cachebase = NULL;

/******************************************************************************/
/* Verschiedene kleine Unterroutinen */

static char *str_xltrim( char *str, const char *searchstr )
{
	int count = 0;
	int i;
	int len = (int) strlen( str );
	if( str == NULL )
		return( NULL );
	while( strchr( searchstr, str[count]) != NULL && count < len )
		count++;
	if( count == len )
		str[ 0 ] = (char) 0;
	else if( count )
		for( i=0; i<len-count+1 ; i++ )
			str[ i ] = str[ i+count ];
	return( str );
}

static char *str_xrtrim( char *str, const char *searchstr )
{
	if( str == NULL )
		return( NULL );
	strrev( str );
	str_xltrim( str, searchstr );
	strrev( str );
	return( str );
}

static char *str_xtrim( char *str, const char *searchstr )
{
	str_xltrim( str, searchstr );
	str_xrtrim( str, searchstr );
	return( str );
}

static char *str_remove_quotes( char *st )
{
	char *rd = st, *wr = st;
	
	do {
		if( *rd == '\\' )
			rd++;
		*wr = *rd;
		wr++;
		rd++;
	} while( *(rd -1) );
	return st;
}

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

static char *str_expand_quote( char *buffer, char *st )
{
	char *rd = st; char *wr = buffer;
	do
	{
		if( *rd == '\"' || *rd == '\\' )
		{
			*wr = '\\';
			wr++;
		}
		*wr = *rd;
		wr++;
		rd++;
	} while( *(rd-1) );
	return buffer;
}

static STRINGENTRY *add_stringentry( STRINGENTRY **anchor, char *st, int linenr )
{
	STRINGENTRY *newstringentry;
	ADDLISTENDENTRY( STRINGENTRY, newstringentry, *anchor, next );
	if( newstringentry )
		if( (newstringentry->name = strdup( st )) == NULL )
			LISTERROR;
	newstringentry->len = (int) strlen(newstringentry->name);
	return newstringentry;
}


/******************************************************************************/
/* Einlesen und Auswerten der Konfigurationsdatei */

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

static char *ruletok( char *str, int *flags, int linenr )
{
	static char *nextt;
	static boolean charmode = FALSE;
	static char currchar;
	static char endchar;
	static char charretstr[2];
	char *s1 = NULL, *s2 = NULL;

	*flags = 0;
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
	
	if( str )
		nextt = str;
	str = nextt;
	if( !nextt )
		return NULL;

	for(;;)
	{
		switch( *str )
		{
			case '\"': 	if( !s1 )
			           	{
			            	s1 = rreadstr( &str );
			            	if( !s1 )
			              	ERROR( E_HL_SYNTAX, linenr, {return NULL;} );			             
			           	}
			           	else if( !charmode )
			           	{
			            	ERROR( E_HL_SYNTAX, linenr, {return NULL;} );
			            }
			           	else if( !s2 )
			           	{
			            	s2 = rreadstr( &str );
			            	if( !s2 )
			              	ERROR( E_HL_SYNTAX, linenr, {return NULL;} );
			              if( strlen(s1) > 1 || strlen(s2) > 1 )
			              	ERROR( E_HL_SYNTAX, linenr, {return NULL;} );
			              currchar = s1[0];
			              endchar = s2[0];
			              if( currchar > endchar )
			              	ERROR( E_HL_SYNTAX, linenr, {return NULL;} );
			           	}
			           	else
			           	{
			            	ERROR( E_HL_SYNTAX, linenr, {return NULL;} );
			            }
			            break;

			case '-':		if( charmode || !s1 )
			            	ERROR( E_HL_SYNTAX, linenr, {return NULL;} );
			            charmode = TRUE;
			            break;

			case ';':
			case '\0':	if( !s1 || (charmode && !s2) )
			            	ERROR( E_HL_SYNTAX, linenr, {return NULL;} );
			            nextt = NULL;
			            return s1;
			case ',':		if( !s1 || (charmode && !s2) )
			            	ERROR( E_HL_SYNTAX, linenr, {return NULL;} );
									str++;
									if( !*str )
										ERROR( E_HL_SYNTAX, linenr, {return NULL;} );
			            nextt = str;
                  return s1;
			case ' ':
			case '\t':	break;

			default:		if( strnicmp( str, "EOL", 3 ) == 0 )
		              {
		              	*flags |= RULEF_EOL;
		              	nextt=str+3;
		              	if( !*nextt )
		              		nextt=NULL;
		              	return "";
		              }
			            ERROR( E_HL_SYNTAX, linenr, {return NULL;} );
		}
		str++;
	}

	
}

static void parse_rule( TXTRULE *txtrule, RULE *rule, char *keywords, int ruletype, int linenr )
{
	char *st;
	int flags;

	switch( ruletype )
	{
		case RULE_TO:
		case RULE_WHILE:
			if( !rule->type )
				ERROR( E_HL_TOFROM, linenr, {return;} );
			if( rule->type == RULE_KEYWORD )
				ERROR( E_HL_MIXED, linenr, {return;} );
			if( rule->type == RULE_FROM )
			{
				if( !rule->endrule )
					rule->endrule = (RULE *) calloc( 1, sizeof( RULE ));
				if( !rule->endrule )
					ERROR( E_HL_MEMORY, linenr, {return;} );
				rule = rule->endrule;
				if( rule->type && rule->type != ruletype )
					ERROR( E_HL_MIXED, linenr, {return;} );
			}
			break;
		case RULE_KEYWORD:
			if( rule->type && rule->type != RULE_KEYWORD )
				ERROR( E_HL_MIXED, linenr, {return;} );
			break;
		case RULE_FROM:
			if( rule->type && rule->type != ruletype )
				ERROR( E_HL_MIXED, linenr, {return;} );
			break;
	}	

	rule->type = ruletype;
	for( st=ruletok( keywords, &flags, linenr ); st; st=ruletok( NULL, &flags, linenr ) )
	{
		rule->flags |= flags;
		if( strlen( st ) )
		{
			if( ruletype == RULE_FROM || ruletype == RULE_KEYWORD )
				txtrule->kwstartchar[ (int) st[0] ] = TRUE;
			else
				txtrule->kwendchar[ (int) st[0] ] = TRUE;
			if( strlen( st ) == 1 )
				rule->kwsinglechar[ (int) st[0] ] = TRUE;
			else
			{
				if( add_stringentry( &rule->kwstring, st, linenr ))
					rule->kwstringchar[ (int) st[0] ] = TRUE;
			}
		}
	}
}

static void read_boolflag( int *var, int flag, char *str, int linenr )
{
	if( stricmp( str, "TRUE" ) == 0 )
		*var |= flag;
	else if( stricmp( str, "FALSE" ) == 0 )
		*var &= ~flag;
	else
   	ERROR( E_HL_SYNTAX, linenr, {return;} );
}

static void parse_attribs( RULE *rule, char *attribs, int linenr )
{
	char *st = strtok( attribs, "," );
	
	if( !st )
   	ERROR( E_HL_SYNTAX, linenr, {return;} );
	while( st )
	{
		str_xtrim( st, " " );
		if( stricmp( st, "bold" ) == 0 )
			rule->attribs |= ATTRIBS_BOLD;
		else if( stricmp( st, "italic" ) == 0 )
			rule->attribs |= ATTRIBS_ITALIC;
		else
   		ERROR( E_HL_SYNTAX, linenr, {return;} );
   	st = strtok( NULL, "," );
	}
		
}


static void parse_line( char *linebuf, int linenr )
{
	static TXTRULE *curr_txtrule = NULL;
	static RULE *curr_rule = NULL;
	char *keyword;
	char *value;
	
	str_xtrim( linebuf, " \n\d" );
	if( !linebuf[0] || linebuf[0] == ';' )
		return;
		
	if( linebuf[ 0 ] == '[' )                /* new text rule */
	{
		str_xtrim( linebuf, "[]" );
		ADDLISTENDENTRY( TXTRULE, curr_txtrule, txtrulebase, next );
		if( !curr_txtrule )
			return;
		curr_txtrule->name = strdup( linebuf );
		if( !curr_txtrule->name )
			LISTERROR;
		return;
	}

	if( linebuf[ 0 ] == '<' )                /* new rule */
	{
		if( !curr_txtrule )
			ERROR( E_HL_SYNTAX, linenr, {return;} );
		str_xtrim( linebuf, "<>" );
		ADDLISTENDENTRY( RULE, curr_rule, curr_txtrule->rules, next );
		curr_rule->name = strdup( linebuf );
		if( !curr_rule->name )
			ERROR( E_HL_MEMORY, linenr, {return;} );
		return;
	}

	value = strchr( linebuf,'=' );
	if( !value || value == linebuf )
		ERROR( E_HL_SYNTAX, linenr, {return;} );
	*value = '\0';
	value++;
	keyword = linebuf;
	
	if( txtrulebase == NULL )
		ERROR( E_HL_SYNTAX, linenr, {return;} );
		
	if( stricmp( keyword, "Extender" ) == 0)
	{
		char *st = rreadstr( &value );
		
		if( !st )
			ERROR( E_HL_SYNTAX, linenr, {return;} );
		while( st )
		{
			add_stringentry( &curr_txtrule->extender, st, linenr );
			value++;
			if( !*value )
				return;
			str_xtrim( value, " " );
			if( *value != ',' )
				ERROR( E_HL_SYNTAX, linenr, {return;} );
			value++;			
			str_xtrim( value, " " );
			if( *value != '\"' )
				ERROR( E_HL_SYNTAX, linenr, {return;} );
			st = rreadstr( &value );
		}
		return;
	}
	else if( stricmp( keyword, "CaseSensitive" ) == 0)
	{
		read_boolflag( &curr_txtrule->flags, TXTRULEF_CASE, value, linenr );
		return;
	}
	else if( stricmp( keyword, "Token" ) == 0)
	{
		char *st = rreadstr( &value );
		if( !st || *(++value) )
			ERROR( E_HL_SYNTAX, linenr, {return;} );
		while( *st )
			curr_txtrule->token[ *(st++) ] = TRUE;
		return;
	}
	
	if( !curr_rule )
		ERROR( E_HL_SYNTAX, linenr, {return;} );

	if( stricmp( keyword, "keyword" ) == 0)
		parse_rule( curr_txtrule, curr_rule, value, RULE_KEYWORD, linenr );
	else if( stricmp( keyword, "from" ) == 0)
		parse_rule(  curr_txtrule, curr_rule, value, RULE_FROM, linenr );
	else if( stricmp( keyword, "to" ) == 0)
		parse_rule(  curr_txtrule, curr_rule, value, RULE_TO, linenr );
	else if( stricmp( keyword, "while" ) == 0)
		parse_rule(  curr_txtrule, curr_rule, value, RULE_WHILE, linenr );
	else if( stricmp( keyword, "color" ) == 0)
		curr_rule->color = atoi( value );
	else if( stricmp( keyword, "bgcolor" ) == 0)
		;
	else if( stricmp( keyword, "attribs" ) == 0)
		parse_attribs( curr_rule, value, linenr );
	else if( stricmp( keyword, "blocking" ) == 0)
		read_boolflag( &curr_rule->flags, RULEF_BLOCKING, value, linenr );
	else if( stricmp( keyword, "counting" ) == 0)
		read_boolflag( &curr_rule->flags, RULEF_COUNTING, value, linenr );
	/* ... */	
	else
	{
		ERROR( E_HL_SYNTAX, linenr, {return;} );
	}
	
}

int Hl_ReadSyn( char *filename )
{
	const char def_filename[] = "qed.syn";
	FILE *synfile;
	char linebuf[ 256 ];
	int linenr=1;
printf( "\33H\n\n\n" );	
	if( !filename )
		filename = (char *) def_filename;
	if( (synfile = fopen( filename, "r" )) == NULL )
		return FALSE;
	while( fgets( linebuf, 256, synfile ))
		parse_line( linebuf, linenr++ );
	fclose( synfile );
	return TRUE;
}

static void fprint_stringentry( FILE *file, char *prefix, STRINGENTRY *anchor )
{
	STRINGENTRY *curr_stringentry = anchor->next;
	char buffer[ 80 ];
	char quotebuffer[ 160 ];
	
	while( curr_stringentry )
	{
		strcpy( buffer, prefix );
		while( curr_stringentry && strlen( buffer ) + strlen( curr_stringentry->name ) + 4 <= 76 )
		{
			sprintf( buffer, "%s\"%s\",", buffer, str_expand_quote( quotebuffer, curr_stringentry->name ));
			curr_stringentry = curr_stringentry->next;
		}
		str_xtrim( buffer, "," );
		fprintf( file, "%s\n", buffer );
	}
}

static void fprint_singlechars( FILE *file, char *prefix, char kwchar[256] )
{
	char c1,c2;
	int charidx = 1;
	char buffer[ 80 ];
	char quotebuf1[ 3 ], quotebuf2[ 3 ];
	boolean printit;
	
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
				if( c2 )
					sprintf( buffer, "%s\"%s\"-\"%s\",",
					         buffer,
					         chr_expand_quote( quotebuf1, c1),
					         chr_expand_quote( quotebuf2, c2) );
				else
					sprintf( buffer, "%s\"%s\",",buffer, chr_expand_quote( quotebuf1, c1 ) );					
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

static void fprint_keywords( FILE *file, RULE *rule )
{
	char *prefix;
	switch( rule->type )
	{
		case RULE_FROM:    prefix = "From = ";    break;
		case RULE_TO:      prefix = "To = ";      break;
		case RULE_WHILE:   prefix = "While = ";   break;
		case RULE_KEYWORD: prefix = "Keyword = "; break;
	}

	if( rule->flags & RULEF_EOL )
		fprintf( file, "%sEOL\n", prefix );
	if( rule->kwstring )
		fprint_stringentry( file, prefix, rule->kwstring );
	fprint_singlechars( file, prefix, rule->kwsinglechar );
	
}

int Hl_WriteSyn( char *filename )
{
	const char def_filename[] = "qed.syn";
	FILE *synfile;
	TXTRULE *curr_txtrule = txtrulebase;
	RULE *curr_rule;
	char quotebuffer[ 256 ];
	char buffer[ 256 ];
	int i, j=0;
	
	if( !filename )
		filename = (char *) def_filename;
	if( (synfile = fopen( filename, "w" )) == NULL )
		return FALSE;
	
	while( curr_txtrule )
	{
		fprintf( synfile, "\n;********************************************************\n" );
		fprintf( synfile, "[%s]\n",curr_txtrule->name );
		fprintf( synfile, ";********************************************************\n" );
if( curr_txtrule->extender )
		fprint_stringentry( synfile, "Extender = ", curr_txtrule->extender );
		fprintf( synfile, "CaseSensitive = %s\n",curr_txtrule->flags & TXTRULEF_CASE ? "TRUE" : "FALSE" );
		memset( buffer, 0, 256 );
		for( i=1; i<256; i++ )
			if( curr_txtrule->token[i] )
				buffer[j++]=(char) i;
		fprintf( synfile, "Token = \"%s\"\n\n", str_expand_quote( quotebuffer, buffer ) );
		
		curr_rule=curr_txtrule->rules;
		while( curr_rule )
		{
			fprintf( synfile, "\n<%s>\n", str_expand_quote( quotebuffer, curr_rule->name ));
			fprint_keywords( synfile, curr_rule );
			if( curr_rule->endrule )
				fprint_keywords( synfile, curr_rule->endrule );
			curr_rule=curr_rule->next;
		}
		curr_txtrule = curr_txtrule->next;
	}
	
	fclose( synfile );
	return TRUE;
}


#define BUFBLK 256

static char *create_buffer( void )
{
	char *buffer = (char *) calloc( 1, BUFBLK );
	return( buffer );
}

static boolean fix_buffsize( char **buffer, int bufflen, int *maxbufflen )
{
	if( bufflen + 10 < *maxbufflen )
		return TRUE;
	if( !*maxbufflen )
	{
		*maxbufflen = BUFBLK;
		*buffer = (char *) malloc( *maxbufflen );
	}
	else
	{
		*maxbufflen += BUFBLK;
		*buffer = (char *) realloc( *buffer, *maxbufflen );
	}
	return( *buffer ? TRUE : FALSE );
}

static char *next_token( char *st, char token[256], int *tokenlength )
{
	boolean b = token[*st];
	*tokenlength = 0;
	while( *st )
	{
		if( b != token[*st] )
			return st;
		(*tokenlength)++; st++;
	}
	return NULL;
}

static boolean add_to_buffer( char *buffer, RULE *rule, int len, int *bufflen )
{		
	buffer[(*bufflen)++ ]= len;
	if( rule )
	{
		buffer[(*bufflen)++ ]= rule->attribs & 3;
		buffer[(*bufflen)++ ]= rule->color;
	}
	else
	{
		buffer[(*bufflen)++ ]= 0;
		buffer[(*bufflen)++ ]= 0;
	}
	return TRUE;
}

static char *create_cachetok( char *st, RULE *rule, RULE **newrule, TXTRULE *txtrule )
{
	static char *buffer = NULL;
	static int maxbufflen = 0;
	int bufflen = 0;
	int len = 0;
	char *retstr;
	char *nextst;
	int tokenlength;
	STRINGENTRY *found;
	RULE *curr_rule;

	while( st )
	{
		fix_buffsize( &buffer, bufflen, &maxbufflen );
		nextst = next_token( st, txtrule->token, &tokenlength );
		len += tokenlength;
		if( rule )
		{
			if( txtrule->kwendchar[st[0]])
			{
				found = NULL;
				if( rule->kwstringchar[st[0]] )
					FINDLISTENTRYTERM( found, rule->kwstring, next,
				  	                 (strncmp(found->name, st, found->len) == 0));
				if( found 
				||  rule->kwsinglechar[st[0]])
				{
					add_to_buffer( buffer, rule, len, &bufflen );
					len = 0;
					rule = NULL;
				}
			}
		}
		else
		{
			if( txtrule->kwstartchar[st[0]])
			{
			
				curr_rule = txtrule->rules;
				while( curr_rule )
				{
					found = NULL;
					if( curr_rule->kwstringchar[st[0]] )
						FINDLISTENTRYTERM( found, curr_rule->kwstring, next,
					  	                 (strncmp(found->name, st, found->len) == 0));
					if( found 
					||  curr_rule->kwsinglechar[st[0]])
					{
						len -= tokenlength;
						if( len )
							add_to_buffer( buffer, NULL, len, &bufflen );
						len = tokenlength;
						if( !curr_rule->endrule )
						{
							add_to_buffer( buffer, curr_rule, len, &bufflen );
							len = 0;
						}	
						rule = curr_rule->endrule;
						break;
					}
					curr_rule = curr_rule->next;
				}
			}
		}
		st = nextst;
	}

	if( len )
		add_to_buffer( buffer, rule, len, &bufflen );

	retstr = malloc( bufflen+1 );
	if( retstr )
	{
		retstr[ bufflen ] = HL_CACHEEND;
		while( bufflen )
		{
			bufflen--;
			retstr[ bufflen ] = buffer[ bufflen ];
		}
	}	
	*newrule = rule;
	return retstr;
}

long Hl_UpdateCache( HL_CACHEANCHOR anchor, HL_CACHEHANDLE handle, char *line )
{
	CACHEBASE *curr_cachebase = (CACHEBASE *) anchor;
	CACHE *curr_cachestruct = (CACHE *) handle;
	RULE *curr_rule = NULL;
	RULE *new_rule;
	long counter = 1;
	
	curr_cachestruct->line = line;
	for(;;)
	{
		if( curr_cachestruct->cachetok )
			free( curr_cachestruct->cachetok );
		
		curr_cachestruct->cachetok = create_cachetok( line, curr_rule, &new_rule,
	  	                                            curr_cachebase->txtrule );
	 	if( !curr_cachestruct->next || new_rule == curr_cachestruct->next->rule )
	 		break;
	 	curr_cachestruct = curr_cachestruct->next;
	 	curr_cachestruct->rule = new_rule;
	 	counter++;
	}
	return counter;
}

long Hl_InsertCacheLine( HL_CACHEANCHOR anchor, HL_CACHEHANDLE prev, char *line, HL_CACHEHANDLE *inserted )
{
	CACHEBASE *curr_cachebase = (CACHEBASE *) anchor;
	CACHE *prev_cachestruct = (CACHE *) prev;
	CACHE *curr_cachestruct;
	RULE *curr_rule = NULL;
	RULE *new_rule;

	*inserted = NULL;
	
	if( !anchor )
	{
		*inserted = NULL;
		return 0L;
	}
	
	if( prev_cachestruct )
	{
		ADDLISTENTRY( CACHE, curr_cachestruct, prev_cachestruct->next, next );
		curr_rule = prev_cachestruct->rule;
	}
	else
		ADDLISTENTRY( CACHE, curr_cachestruct, curr_cachebase->cache, next );

	if( curr_cachestruct )
	{
		*inserted = (HL_CACHEHANDLE) curr_cachestruct;
		curr_cachestruct->cachetok = create_cachetok( line, curr_rule, &new_rule,
		                                              curr_cachebase->txtrule );
		if( curr_cachestruct->next
		&&  curr_cachestruct->next->rule != new_rule )
		{
			curr_cachestruct->next->rule = new_rule;
			return 1L + Hl_UpdateCache( anchor, (HL_CACHEHANDLE) curr_cachestruct->next,
			                            curr_cachestruct->next->line );
		}
		
	}
	return 1L;
}


long Hl_RemoveCacheLine( HL_CACHEHANDLE handle )
{
}

CACHELINE Hl_GetCacheLine( HL_CACHEHANDLE handle )
{
	if( handle == NULL )
		return NULL;
	return (CACHELINE) ((CACHE *) handle)->cachetok;
}

HL_CACHEANCHOR Hl_NewCache( char *txttype, int resvd )
{
	TXTRULE *curr_txtrule = txtrulebase;
	STRINGENTRY *found;
	CACHEBASE *curr_cachebase;
	
	while( curr_txtrule )
	{
		FINDLISTSTRENTRY( found, curr_txtrule->extender, name, next, txttype );
		if( found )
		{
			ADDLISTENTRY( CACHEBASE, curr_cachebase, cachebase, next );
			if( curr_cachebase )
				curr_cachebase->txtrule = curr_txtrule;
			return (HL_CACHEANCHOR) curr_cachebase;
		}
		curr_txtrule = curr_txtrule->next;
	}
	return NULL;
}

int Hl_ChangeTxtType( HL_CACHEANCHOR anchor, char *txttype, int resvd )
{
}

void Hl_FreeCache( HL_CACHEANCHOR anchor )
{
}





