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


#if !defined(H_MYTYP)
#define H_MYTYP
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#if defined(MINGW)
	#include <windows.h>
	#if !defined(MVSC)
		#define MVSC
	#endif
#endif

#ifdef _MSC_VER
	#include <windows.h>
	#if !defined(MVSC)
		#define MVSC
	#endif
#else
	#include <unistd.h>
#endif

#if defined(__linux__) || defined(__GNUC__)
	#if !defined(GCCLINUX)
		#define GCCLINUX
	#endif
#endif

#if defined(MINGW)
	#undef GCCLINUX
#endif

#if defined(GCCLINUX) || defined(MINGW)

	#include <stdint.h>

#elif defined(MVSC)

	typedef unsigned char			uint8_t;
	typedef unsigned short int		uint16_t;
	typedef unsigned int			uint32_t;
	typedef unsigned __int64		uint64_t;

	typedef signed char				int8_t;
	typedef short int				int16_t;
	typedef int						int32_t;
	typedef __int64					int64_t;

#else
	#error OS not defined properly for 64 bit integers
#endif


//===============================================

#include "boolean.h"

typedef int32_t player_t;

struct gamei {
	int32_t 	whiteplayer;
	int32_t 	blackplayer;
	int32_t 	score;	
};

struct GAMES {
	size_t	 	n; 
	size_t		size;
	struct gamei *ga;
};

struct ENC {
	// FIXME covert to long ints or int32_t
	double 	wscore;
	int		played;
	int 	wh;
	int 	bl;
	int		W;
	int		D;
	int		L;
};

struct GAMESTATS {
	long int
		white_wins,
		draws,
		black_wins,
		noresult;
};

enum 			Player_Performance_Type {
				PERF_NORMAL = 0,
				PERF_SUPERWINNER = 1,
				PERF_SUPERLOSER = 2,
				PERF_NOGAMES = 3
};

struct prior {
	double value;
	double sigma;	
	bool_t isset;
};

struct relprior {
	int player_a;
	int player_b;
	double delta;
	double sigma;	
};

struct DEVIATION_ACC {
	double sum1;
	double sum2;
	double sdev;
};

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
