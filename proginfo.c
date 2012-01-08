#include <stdio.h>
#include "progname.h"
#include "proginfo.h"
#include "version.h"

static const char *author = "Miguel A. Ballicora";
static const char *compilation_date = __DATE__;

const char *proginfo_author(void) {return author;}

const char *proginfo_current_year(void) {return &compilation_date[7];}

const char *proginfo_version(void) 
{
	const char *s = VERSION;
#if 0
	while (*s == ' ') s++;
	while (*s != ' ' && *s != '\0') s++;
	while (*s == ' ') s++;
#endif
	return s;
}

const char *proginfo_name(void) {return PROGRAM_NAME;}


