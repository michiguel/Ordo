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
