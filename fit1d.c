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
#include "fit1d.h"
#include "boolean.h"

#if !defined(NDEBUG)
#include <stdio.h>
static bool_t is_nan (double x) {if (x != x) return TRUE; else return FALSE;}
#endif

static double absol(double x) {return x >= 0? x: -x;}


static bool_t
find_parabolic_min_x_withreference (const double *x, const double *y, double reference, double *result)
{
	double y12, x12, y13, x13, s12, s13, d1, d2, den, res;
	double x1, x2, x3;

	assert (x[3] > x[2]);
 	assert (x[2] > x[1]);

	x1 = x[1] - reference;
	x2 = x[2] - reference;
	x3 = x[3] - reference;

	y12 = y[1] - y[2];
	x12 = x1 - x2;
	y13 = y[1] - y[3];
	x13 = x1 - x3;
	s12 = x1*x1 - x2*x2;
	s13 = x1*x1 - x3*x3;

	if (x12*y13 <= y12*x13) // not a minimum
		return FALSE;

	d1 = y13*x12;
	d2 = y12*x13;
	den = d1 - d2;

	if (den < 1E-64)
		return FALSE;

	res = ((y13*s12 - y12*s13) / den)/2;
	res += reference;
	assert(!is_nan(res));

	*result = res;
	return TRUE;
}

static bool_t
find_parabolic_min_x (const double *x, const double *y, double *result)
{
	double reference;
	bool_t ok;
	double res;

	assert (x[3] > x[2]);
 	assert (x[2] > x[1]);

	reference = x[2];
	ok = find_parabolic_min_x_withreference (x, y, reference, &res);
	if (!ok) return FALSE;

	reference = res;
	ok = find_parabolic_min_x_withreference (x, y, reference, &res);
	if (!ok) return FALSE;

	*result = res;
	return ok;
}


#define Epsilon 1E-7

static double
optimum_center (double *x, double *y)
{
	double result;
	assert (x[3] > x[2]);
 	assert (x[2] > x[1]);

	// is this too strict?
	assert (y[3] >= y[2]);
 	assert (y[1] >= y[2]);

	if (
		(x[3]-x[1]) > Epsilon &&
		(x[2]-x[1]) > Epsilon &&
		(x[3]-x[2]) > Epsilon &&
		find_parabolic_min_x (x, y, &result)
		) {
		assert (result > x[1] 
				|| !fprintf(stderr, "\n\nx1=%.12le x2=%.12le x3=%.12le result=%.12le\n",x[1],x[2],x[3],result)
				|| !fprintf(stderr, "y1=%.12le y2=%.12le y3=%.12le \n",y[1],y[2],y[3])
				|| !fprintf(stderr, "dy1=%.12le dy2=%.12le dy3=%.12le \n",y[1]-y[1],y[2]-y[1],y[3]-y[1])
				|| !fprintf(stderr, "x3-x1=%.12le x2-x1=%.12le result-x1=%.12le \n\n",x[3]-x[2],x[2]-x[1],result-x[1])
		);
		assert (result < x[3]);
		return result;
	}
	return (x[3]+x[1])/2;
}


double
quadfit1d_2 (double limit, 
			double x1, double x2, double x3,
			double ya, double yb, double yc, 
			double (*unfitnessf)(double, const void *), const void *p)
{	
	long int rightchop=0, leftchop=0;
	bool_t equality = FALSE;
	double x[4];
	double y[4];

	assert(!is_nan(limit));
	assert(!is_nan(x1) && !is_nan(x2) && !is_nan(x3) && !is_nan(ya) && !is_nan(yb) && !is_nan(yc));
	assert (yc >= yb && ya >= yb);
	assert ((x3 > x2 && x2 > x1) || !fprintf(stderr, "x1=%lf, x2=%lf, x3=%lf\n",x1,x2,x3));

	x[1] = x1;
	x[2] = x2;
	x[3] = x3;
	y[1] = ya;
	y[2] = yb;
	y[3] = yc;

	x[0] = optimum_center (x, y); 
	y[0] = unfitnessf( x[0], p);

	do {
		assert (!(x[1] > x[2] || x[2] > x[3] || x[1] > x[3]));

		equality = FALSE;

		if (x[0] < x[2] && y[0] <= y[2]) { rightchop++; leftchop=0;
			x[3] = x[2];
			y[3] = y[2];	
			x[2] = x[0];
			y[2] = y[0];
		} else
		if (x[0] > x[2] && y[0] >  y[2]) { rightchop++; leftchop=0;
			x[3] = x[0];
			y[3] = y[0];
		} else
		if (x[0] < x[2] && y[0] >  y[2]) { rightchop=0; leftchop++;
			x[1] = x[0];
			y[1] = y[0];
		} else
		if (x[0] > x[2] && y[0] <= y[2]) { rightchop=0; leftchop++;
			x[1] = x[2];
			y[1] = y[2];
			x[2] = x[0];
			y[2] = y[0];
		} else {						  equality = TRUE;;

			if (x[3]-x[2] > x[2]-x[1]) {
				x[0] = x[2] + 0.01 * (x[3]-x[2]);
			} else {
				x[0] = x[2] - 0.01 * (x[2]-x[1]);
			}
		}

		if (equality) {

			y[0] = unfitnessf( x[0], p);

		} else if (rightchop < 3 && leftchop < 3) {

			x[0] = optimum_center (x, y);
			y[0] = unfitnessf( x[0], p);

		} else {
			double thirdlo = (1*x[3]+2*x[1])/3;
			double thirdhi = (2*x[3]+1*x[1])/3;

			x[0] = x[2];

			if (x[3]-x[2] > 2*(x[2]-x[1]) ) { // lower third

				do {
					x[0] = x[0] + (x[0] - x[1]);
					y[0] = unfitnessf( x[0], p);

				} while (x[0] < thirdlo && y[0] <= y[2]);

			} else 
			if (x[3]-x[2] < (x[2]-x[1])/2 ) { // upper third

				do {
					x[0] = x[0] - (x[3] - x[0]);
					y[0] = unfitnessf( x[0], p);
				} while (x[0] > thirdhi && y[0] <= y[2]);

			} else {

				x[0] = (x[2] + (leftchop==0?x[1]:x[3]) ) / 2;
				y[0] = unfitnessf( x[0], p);
			}

			assert (x[0] > x[1]);
			assert (x[0] < x[3]);
		}

	} while (absol(x[3]-x[1]) > limit);	

assert (absol(x[3]-x[1]) <= limit);
assert (x[2] <= x[3]);
assert (x[1] <= x[2]);

	return x[2];
}


double
quadfit1d	(double limit, double a, double b, double (*unfitnessf)(double, const void *), const void *p)
{
	double cente = (a+b)/2;
	double delta_neg, delta_pos;
	double ei, ej, ek;
	double xi, xj, xk;
	double r1, r2;

	assert(!is_nan(limit));
	assert(!is_nan(a));
	assert(!is_nan(b));

	delta_pos = delta_neg = (b>a?b-a:a-b)/2;

	assert(!is_nan(cente));
	assert(!is_nan(delta_neg));
	assert(!is_nan(delta_pos));

	xi = cente - delta_neg;
	xj = cente;
	xk = cente + delta_neg;

	ei = unfitnessf	(xi, p);
	ej = unfitnessf	(xj, p);
	ek = unfitnessf	(xk, p);
	// find if the input is quasi-flat, the return center

	if (!(ej > 0)) 
		return xj;
	r1 = (ei <= ej)? ej/ei: ei/ej;
	r2 = (ej <= ek)? ek/ej: ej/ek;
	if (r1 < 1.0000001 && r2 < 1.0000001) {
		return xj;
	}

	for (;;) {	

		assert(!is_nan(ei));
		assert(!is_nan(ej));
		assert(!is_nan(ek));
		if (ei >= ej && ej <= ek) {
			double r;
			assert(!is_nan(xi));
			assert(!is_nan(xj));
			r = quadfit1d_2 (limit, xi, xj, xk, ei, ej, ek, unfitnessf, p);
			assert(!is_nan(r));
			return r;

		} else
		if (ej >= ei && ei <= ek) {

			delta_neg *= 2;

			ek = ej;
			ej = ei; 

			xk = xj;
			xj = xi;
			xi = cente - delta_neg;

			assert (!is_nan(xi));
			ei = unfitnessf	(xi, p);

		} else
		if (ei >= ek && ek <= ej) {

			delta_pos *= 2;

			ei = ej;
			ej = ek;

			xi = xj;
			xj = xk;
			xk = cente + delta_pos;

			assert(!is_nan(xk));
			ek = unfitnessf	(xk, p);

		} else {
			assert(0);
		}
	} 
}

//============ center adjustment end
