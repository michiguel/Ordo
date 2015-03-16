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

#include <math.h>
#include <assert.h>

#include "boolean.h"
#include "xpect.h"


double inv_xpect	(double invbeta, double p) 
{
	return (-1.0*invbeta) * log(100.0/p-1.0);
}

double
xpect (double a, double b, double beta)
{
	return 1.0 / (1.0 + exp((b-a)*beta));
}

#if 0
void
get_pWDL(double delta_rating /*delta rating*/, double *pw, double *pd, double *pl, double drawrate0, double beta)
{
	double perf, pdra, pwin, plos;
	bool_t switched;
	
	switched = delta_rating < 0;
	if (switched) delta_rating = -delta_rating;

	perf = xpect (delta_rating,0,beta);
	pdra = draw_rate_fperf (perf, drawrate0);
	pwin = perf - pdra/2;
	plos = 1 - pwin - pdra;

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
#endif

static double Abs(double a) { return a > 0? a: -a;}
 
double 
draw_rate_fperf (double p, double d0)
{
	double	fi, a, b, c, x, y, dy, newx;
	double ret=0;

	if (d0 < 0.0001) {
		ret = 2*d0*sqrt(p-p*p)-d0*d0;
		return ret;
	}

		fi = (1-d0)/(2*d0);
		c = 4*(p*p-p);
		b = 2;
		a = 4*fi*fi-1;

	if (d0 < 1-0.0001 && (d0 > 0.5001 || d0 < 0.4999) ) {
		ret = ( sqrt(b*b-4*a*c) - b) /(2*a);
		return ret;
	}

	newx = 0;
	do 
	{
		x = newx;
		y  = c + b * x + a * x * x;
		dy = b + 2*a*x;
		newx = x - y/dy;

	} while (Abs(newx - x) > 0.0000001);
 
	return newx;
}


static double 
draw_rate_fperf_calc (double p, double d0)
{
//	double fi;
	double a, c;
	double ret;

	assert (d0 <= 1.0 && d0 >= 0);
	assert (p  <= 1.0 && p  >= 0);
	assert(!(!(d0 < 0.4999) && d0 < 0.5001));

//	fi = (1-d0)/(2*d0);
	c = 4*(p*p-p);
//	a = 4*fi*fi-1;

a = (1 - 2*d0)/(d0*d0);

	ret = ( sqrt(1-a*c)-1 ) / a; // because b = 2
	return ret;
}

static double 
draw_rate_fperf_iter (double p, double d0)
{
	double	fi, a, b, c, x, y, dy, newx;

	assert (d0 <= 1.0 && d0 >= 0);
	assert (p  <= 1.0 && p  >= 0);
	assert(!(d0 < 0.4999) && d0 < 0.5001);

	fi = (1-d0)/(2*d0);
	c = 4*(p*p-p);
	b = 2;
	a = 4*fi*fi-1;

	newx = 0;
	do {
		x = newx;
		y  = c + b * x + a * x * x;
		dy = b + 2*a*x;
		newx = x - y/dy;

	} while (Abs(newx - x) > 0.0000001);
 
	return newx;
}

void
get_pWDL (double delta_rating /*delta rating*/, double *pw, double *pd, double *pl, double d0, double beta)
{
#define MINPROB 1E-32

	double perf, pdra, pwin, plos;
	bool_t switched;
	
	switched = delta_rating < 0;
	if (switched) delta_rating = -delta_rating;

	perf = xpect (delta_rating,0,beta);

	if (d0 < 0.00001) {
		pdra = 2*d0*sqrt(perf-perf*perf)-d0*d0;
		plos = 1 - perf - pdra/2;
		pwin = 1 - plos - pdra;	
	} else
	if (d0 < 0.49999) {
		pdra = draw_rate_fperf_calc (perf, d0);
		plos = 1 - perf - pdra/2;
		pwin = 1 - plos - pdra;	
	} else
	if (d0 < 0.50001) {
		pdra = draw_rate_fperf_iter (perf, d0);
		plos = 1 - perf - pdra/2;
		pwin = 1 - plos - pdra;	
	} else 
	if (d0 < 0.99000) {
		pdra = draw_rate_fperf_calc (perf, d0);
		plos = 1 - perf - pdra/2;
		pwin = 1 - plos - pdra;	
	} else {
		double x = perf - 0.5;
		double a = (1 - 2*d0)/(d0*d0);
		plos =	0.5 - x + 0.5 * (	(1-sqrt(1+a-4*a*x*x))/a 	);
		if (plos < MINPROB) plos = MINPROB;
		pdra = 2 * (1 - perf - plos);
		pwin = 1 - plos - pdra;	
	}

	if (plos < MINPROB) plos = MINPROB;
	if (pdra < MINPROB) pdra = MINPROB;
	if (pdra < MINPROB) pdra = MINPROB;

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
