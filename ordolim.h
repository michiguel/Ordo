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


#if !defined(H_OLIM)
#define H_OLIM
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#define PRIOR_SMALLEST_SIGMA 0.0000001

#ifdef NDEBUG
	#define LABELBUFFERSIZE (1 << 16)
	#define MAXBLOCKS ((size_t)2048*(size_t)1024)
	#define MAXGAMESxBLOCK ((size_t)1024*(size_t)1024)
	#define MAXNAMESxBLOCK ((size_t)1024*(size_t)64)
	#define MAX_RPBLOCK 1024
#else
	#define LABELBUFFERSIZE 100
	#define MAXBLOCKS ((size_t)2048*(size_t)1024)
	#define MAXGAMESxBLOCK ((size_t)16)
	#define MAXNAMESxBLOCK ((size_t)16)
	#define MAX_RPBLOCK 100
#endif

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
