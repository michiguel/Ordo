#if !defined(H_MYSTR)
#define H_MYSTR
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#define STDSTRSIZE 256
#define EPD_STRSIZE 1025
#define EPD_STRLEN  (EPD_STRSIZE-1)

/*	XBLINE_STRSIZE should be at least
	EPD_STRLEN + strlen("setboard ") + 1;
*/
#define XBLINE_STRSIZE 1040

/*--------------------------*
	Function prototypes
 *--------------------------*/

void mystrncpy (char *s, const char* p, int n);


/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
