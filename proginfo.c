/*
	Ordo is program for calculating ratings of engine or chess players
    Copyright 2013 Miguel A. Ballicora

    This file is part of Ordo.

    Ordo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ordo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ordo.  If not, see <http://www.gnu.org/licenses/>.
*/


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


