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
	typedef unsigned short int	uint16_t;
	typedef unsigned int			uint32_t;
	typedef unsigned __int64		uint64_t;

	typedef signed char			int8_t;
	typedef short int				int16_t;
	typedef int					int32_t;
	typedef __int64					int64_t;

#else
	#error OS not defined properly for 64 bit integers
#endif


//===============================================

#include "boolean.h"

enum Player_Performance_Type {
	PERF_NORMAL = 0,
	PERF_SUPERWINNER = 1,
	PERF_SUPERLOSER = 2,
	PERF_NOGAMES = 3
};

typedef int64_t gamesnum_t;

typedef int64_t player_t;



struct gamei {
	player_t 	whiteplayer;
	player_t 	blackplayer;
	int32_t 	score;	
};

struct GAMES {
	gamesnum_t 	n; 
	gamesnum_t	size;
	struct gamei *ga;
};

struct ENC {
	double 	wscore;
	player_t 	wh;
	player_t 	bl;
	gamesnum_t	played;
	gamesnum_t	W;
	gamesnum_t	D;
	gamesnum_t	L;
};

struct GAMESTATS {
	gamesnum_t
		white_wins,
		draws,
		black_wins,
		noresult;
};

struct prior {
	double value;
	double sigma;	
	bool_t isset;
};

struct relprior {
	player_t player_a;
	player_t player_b;
	double delta;
	double sigma;	
};

struct DEVIATION_ACC {
	double sum1;
	double sum2;
	double sdev;
};


struct summations {
	struct DEVIATION_ACC *relative; // to be dynamically assigned
	double	*sum1; // to be dynamically assigned
	double	*sum2; // to be dynamically assigned
	double	*sdev; // to be dynamically assigned 
	double wa_sum1;
	double wa_sum2;				
	double dr_sum1;
	double dr_sum2; 
	double wa_sdev;				
	double dr_sdev;
};

//

struct PLAYERS {
	player_t	n; 
	player_t	size;
	player_t	anchored_n;
	const char  **name;
	bool_t		perf_set;
	bool_t		*flagged;
	bool_t		*prefed;
	bool_t		*priored;
	int			*performance_type; 
};

struct RATINGS {
	player_t	size;
	player_t	*sorted; 	/* sorted index by rating */
	double		*obtained;
	gamesnum_t	*playedby; 	/* N games played by player "i" */
 	double		*ratingof; 	/* rating current */
 	double		*ratingbk; 	/* rating backup  */
 	double		*changing; 	/* direction change vector */
	double		*ratingof_results;
	double		*obtained_results;
	gamesnum_t	*playedby_results;
};

struct ENCOUNTERS {
	gamesnum_t	n;		
	gamesnum_t	size;
	struct ENC *enc;
};

//

struct rel_prior_set {
	player_t n;
	struct relprior *x; // this points to an array
};

struct output_qualifiers {
	bool_t  	mingames_set;
	gamesnum_t	mingames; 
};

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
