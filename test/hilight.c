#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "..\highlite.h"

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

static void print_buffer( unsigned char *buffer )
{
	while( !( *buffer & HL_CACHEEND ))
	{
		printf( "(%3i)", *(buffer++) );
		printf( "(0x%2x)", *(buffer++) );
		printf( "(%3i)", *(buffer++) );
		printf( ";" );
	}
	printf( "\n" );
}

#define PRINTIT
void main( int argc, char *argv[] )
{
	char filename[ 256 ];
	char linebuf[ 2000 ];
	unsigned char *buffer;
	FILE *file;
	HL_CACHEANCHOR anchor;
	HL_CACHEHANDLE handle = NULL;
	printf( "\33E" );
	strcpy( filename, argv[1] );
	printf( "filename:%s\n",filename );
	Hl_ReadSyn( NULL );
	file = fopen( filename, "r" );
	if( !file ) { printf("fopen err\n");return; }

	anchor = Hl_NewCache( "c", 0 );

	while( fgets( linebuf, 2000, file ))
	{
		str_xtrim( linebuf, "\d\n" );
#ifdef PRINTIT
		printf( "%s\n", linebuf );
#endif
		Hl_InsertCacheLine( anchor, handle, linebuf, &handle ); 
		buffer = Hl_GetCacheLine( handle );
#ifdef PRINTIT
		print_buffer( buffer );
		getchar();
#endif
	}
	
/*	Hl_WriteSyn( "Test.syn" ); */
	

}
