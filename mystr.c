#include <assert.h>
#include <stdlib.h>
#include "mystr.h"

/*------------------------------------------------------*\
	mystrncpy:

	Copy a string from 'src' to 'dest'.
	'n' is the maximum number of characters that 'dest' 
	can receive. 'n' cannot be 0 or negative.
	'n' generally is the size of the array 'dest'.
	It is guaranteed that 'dest' will end in '\0'.
	So, the maximum length that the function can copy
	is  'n-1' to give space to the terminator char '\0'
	and if there are more characters in src they will be 
	truncated.
\*------------------------------------------------------*/

void
mystrncpy (char *dest, const char *src, int n)
{
	enum {	/* for debugging purposes */
		FILLINGBYTE = 'y'
	};
	int c; 

	assert (n > 0);
	assert (NULL != dest);
	assert (NULL != src);

	c = '\0';
	while (n > 1) {
		n--;
		c = *dest++ = *src++;
		if (c == '\0')
			break;
	}
	if (c != '\0') {
		n--;
		*dest++ = '\0';
	}

	#ifndef NDEBUG
	while (n > 0) {
		n--;
		*dest++ = FILLINGBYTE;
	}
	#endif

}

