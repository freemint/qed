#ifndef LISTS_H
#define LISTS_H

/* list macros */

/* some defines which you may change for your own purposes */
/***********************************************************/

/* memory allocation (must be set to 0, normally use calloc(1,n), 
   memory free and error. Error is always a memory error. */
#ifndef LISTALLOC
	#define LISTALLOC( size )  calloc( 1, size )
#endif
#ifndef LISTFREE
	#define LISTFREE( elem )  free( (void *) elem )
#endif
#ifndef LISTERROR
	#define LISTERROR
#endif

/* the names of the 'next' and 'previous' elements */
#ifndef NEXTELEMENT
	#define NEXTELEMENT next
#endif
#ifndef PREVELEMENT
	#define PREVELEMENT prev
#endif

/*
   find an entry in the list
   in retptr the found entry or NULL is returned
   listname: list pointer
   element: element name
   refelement: with this the element is compared
*/
#define LIST_FIND( retptr, listname, element, refelement ) {\
	(retptr) = (listname);\
	while( retptr )\
	{\
		if( (retptr)->element == (refelement) )\
			break;\
		(retptr) = (retptr)->NEXTELEMENT;\
	}\
}

/*
   find an string entry in the list
   in retptr the found entry or NULL is returned
   listname: list pointer
   element: element name
   refelement: with this the element is compared
*/
#define LIST_FINDSTR( retptr, listname, element, refelement ) \
{\
	(retptr) = (listname);\
	while( (retptr) )\
	{\
		if( strcmp( (retptr)->element, (refelement) ) == 0 )\
			break;\
		(retptr) = (retptr)->NEXTELEMENT;\
	}\
}

/*
   find an string entry in the list, case indifferent
   in retptr the found entry or NULL is returned
   listname: list pointer
   element: element name
   refelement: with this the element is compared
*/
#define LIST_FINDISTR( retptr, listname, element, refelement ) \
{\
	(retptr) = (listname);\
	while( retptr )\
	{\
		if( stricmp( (retptr)->element, (refelement) ) == 0 )\
			break;\
		(retptr) = (retptr)->NEXTELEMENT;\
	}\
}

/*
   find an entry in the list by a free comparison term
   in retptr the found entry or NULL is returned
   listname: list pointer
   comparison_term: this term is used to find the element. The parts of this term
   should be the according element of the searching struct WITH the name of retptr
   and the value to be compared.
   for example: If you got a structure
   typedef struct st {
     int s_a;
     int s_b;
     struct st *st_next;
   } ST;
   and you want to find an element by comparing a variable a with s_a and a
   variable b with s_b, your call of LIST_FINDTERM() is:
   LIST_FINDTERM( my_retptr, my_listname, st_next, (a == my_retptr->s_a) && (b == my_retptr->s_b)); 
*/

#define LIST_FINDTERM( retptr, listname, comparison_term ) \
{\
	(retptr) = (listname);\
	while( retptr )\
	{\
		if( comparison_term )\
			break;\
		(retptr) = (retptr)->NEXTELEMENT;\
	}\
}

/*
   add an entry to a list (at the beginning )
   type: type of the struct
   retptr: struct pointer, contains pointer to new element after macro or NULL if alloc failed
   listname: list pointer
*/    
#define LIST_ADD( type, retptr, listname ) \
{\
	(retptr) = (type *) LISTALLOC( sizeof(type) );\
\
	if( retptr )\
	{\
		(retptr)->NEXTELEMENT = (listname);\
		(listname) = (retptr);\
	}\
	else\
		LISTERROR;\
}

/*
   add an entry to a list (at the end). In most cases it's better to use
   LIST_ADD() instead, because it's faster and shorter.
   type: type of the struct
   retptr: struct pointer, contains pointer to new element after macro or NULL if alloc failed
   listname: list pointer
*/    

#define LIST_ADDEND( type, retptr, listname ) \
{\
	type **listadr = &(listname);\
	(retptr) = *listadr;\
\
	while( retptr )\
	{\
		(listadr) = &((retptr)->NEXTELEMENT);\
		(retptr) = (retptr)->NEXTELEMENT;\
	}\
	(retptr) = (type *) LISTALLOC( sizeof(type) );\
	if( retptr )\
		(retptr)->NEXTELEMENT = 0;\
	else\
		LISTERROR;\
	*listadr = (retptr);\
}

/*
   remove an entry from a list.
   type: type of the struct
   rmptr: pointer to element to remove. After the macro it contains NULL if
          it was removed and the previous value if it was not in list.
   listname: list pointer
*/    

#define LIST_RM( type, rmptr, listname ) \
{\
	type *lptr = (listname);\
\
	if( (rmptr) == (listname) )\
	{\
		(listname) = (rmptr)->NEXTELEMENT;\
		LISTFREE( rmptr );\
		(rmptr) = 0;\
	}\
  else\
		while( lptr )\
		{\
			if( lptr->NEXTELEMENT == (rmptr) )\
			{\
				lptr->NEXTELEMENT = (rmptr)->NEXTELEMENT;\
				LISTFREE( rmptr );\
				(rmptr) = 0;\
				break;\
			}\
			lptr = lptr->NEXTELEMENT;\
		}\
}


#endif

