#ifndef hilite_h
#define hilite_h

/* cache line flags; originally intended for easy use
 * with GEM VDI, change these for your own purposes, if
 * needed (within a HL_ELEM)
 */
#define HL_BOLD 1         /* set bold text fx */
#define HL_LIGHT 2        /* set light text fx */
#define HL_ITALIC 4       /* set italic text fx */
#define HL_COLOR 0x20     /* set color */
#define HL_SELCOLOR 0x40  /* set selected color */
#define HL_CACHEEND 0x80  /* end of the cache line */

typedef void * HL_HANDLE;        /* Handle of a complete Text */
typedef void * HL_LINEHANDLE;    /* Handle of a line */
typedef unsigned char HL_ELEM;   /* one element of a cache line - change it if you need sth bigger */
typedef HL_ELEM * HL_LINE;       /* Cache line */

/* rule info; for getting and changing rule */
typedef struct {
	char *name;        /* rule name, ignored when setting (Hl_ChangeRule()) */
	HL_ELEM attribs;   /* text attributes and HL_COLOR, HL_SELCOLOR if colors valid */
	long color;        /* text color */
	long selcolor;     /* selected text color */
} HL_RULEINFO;

/* error numbers */
typedef enum { 
	E_HL_MEMORY,       /* memory error */
	E_HL_TOFROM,       /* "to" or "while" keyword before "from" */
	E_HL_MIXED,        /* "from"-"to"/"while" and "keyword" mixed */        
	E_HL_SYNTAX,       /* syntax error */
	E_HL_WRONGVAL      /* wrong numerical value */
} HL_ERRTYPE;

/* init and exit routines */
void Hl_Init( void (*err_callback)( HL_ERRTYPE err, int linenr ) );
void Hl_Exit( void );

/* read and write syntax file */
int Hl_ReadSyn( char *filename, int curr_planes );
int Hl_WriteSyn( char *filename );

/* handle syntax cache (main functions) */
HL_HANDLE     Hl_New( char *txttype, int resvd );
long Hl_InsertLine( HL_HANDLE anchor, HL_LINEHANDLE prev, char *line,
                    HL_LINEHANDLE *inserted, int update );
long Hl_Update( HL_HANDLE anchor, HL_LINEHANDLE handle, char *line, int updatenext );
long Hl_RemoveLine( HL_HANDLE anchor, HL_LINEHANDLE prev, HL_LINEHANDLE *curr, int update );
HL_LINE Hl_GetLine( HL_LINEHANDLE handle );
void Hl_Free( HL_HANDLE anchor );

/* get, set and temporarily save settings */
int Hl_EnumTxtNames( char **txtname, int *idx );
int Hl_EnumTxtTypes( int txtidx, char **txttype, int *idx );
int Hl_EnumRules(  int txtidx, HL_RULEINFO *ruleinfo, int *idx );
int Hl_IsActive( int txtidx );
void Hl_SetActive( int txtidx, int active );
void Hl_RestoreSettings( void );
void Hl_DeleteSaveSettings( void );
int Hl_SaveSettings( void );
int Hl_ChangeRule( int txtidx, int idx, HL_RULEINFO *ri );
int Hl_TxtIndexByTxttype( char *txttype );

#endif

