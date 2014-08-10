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


#if !defined(H_XPECT)
#define H_XPECT
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

//#include "boolean.h"
//#include "mytypes.h"


static double
fxpect (double a, double b, double beta)
{
	return 1.0 / (1.0 + exp((b-a)*beta));
}

#if 0
#define DRAWRATE_AT_EQUAL_STRENGTH 0.33
#define DRAWFACTOR (1/(2*(DRAWRATE_AT_EQUAL_STRENGTH))-0.5)
#endif

static void
fget_pWDL(double dr /*delta rating*/, double *pw, double *pd, double *pl, double beta)
{
	// Performance comprises wins and draws.
	// if f is expected performance from 0 to 1.0, then
	// f = pwin + pdraw/2
	// from that, dc is the fraction of points that come from draws, not wins, so
	// pdraw (probability of draw) = 2 * f * dc
	// calculation of dc is an empirical formula to fit average data from CCRL:
	// Draw rate of equal engines is near 0.33, and decays on uneven matches.

	double f;
	double pdra, pwin, plos;
// 	double dc; 
	bool_t switched;
	
	switched = dr < 0;

	if (switched) dr = -dr;

	f = fxpect (dr,0,beta);

#if 0
	dc = 0.5 / (0.5 + DRAWFACTOR * exp(dr*beta));
	pdra = 2 * f * dc;
	pwin = f - pdra/2;
	plos = 1 - pwin - pdra; 
#else
	pwin = f * f;
	plos = (1-f) * (1-f);
	pdra = 1 - pwin - plos;
#endif

	if (switched) {
		*pw = plos;
		*pd = pdra;
		*pl = pwin;
	} else {
		*pw = pwin;
		*pd = pdra;
		*pl = plos;
	}
	return;
}

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
