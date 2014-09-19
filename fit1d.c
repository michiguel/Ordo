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

//#include <stdio.h>
//#include <stdlib.h>
#include <assert.h>
#include "fit1d.h"
#include "boolean.h"

static double absol(double x) {return x >= 0? x: -x;}

static double
parabolic_center_x (double *x, double *y)
{
	double y12, x12, y13, x13, s12, s13;
	y12 = y[1] - y[2];
	x12 = x[1] - x[2];
	y13 = y[1] - y[3];
	x13 = x[1] - x[3];
	s12 = x[1]*x[1] - x[2]*x[2];
	s13 = x[1]*x[1] - x[3]*x[3];

	return ((y13*s12 - y12*s13) / (y13*x12 - y12*x13))/2;
}

//========

double
quadfit1d_2 (double limit, double a, double b, double (*unfitnessf)(double, const void *), const void *p)
{	
	long int rightchop=0, leftchop=0;
	bool_t equality = FALSE;
	int i;
	double x[4];
	double y[4];

	x[1] = a > b? b: a;
	x[2] = (a+b)/2;
	x[3] = b > a? b: a;

	for (i = 1; i < 4; i++) {
		y[i] = unfitnessf	(x[i], p);
	}

	x[0] = parabolic_center_x (x, y);
	if (x[0] < x[1] || x[0] > x[3]) {
		x[0] = (x[1] + x[3]) / 2; 
	}
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
			x[0] = (x[1] + x[3])/2;
		}

		if (equality) {
			// do nothing
		} else if (rightchop < 2 && leftchop < 2) {

			x[0] = parabolic_center_x (x, y);
			if (x[0] <= x[1] || x[0] >= x[3]) {
				x[0] = (x[1] + x[3]) / 2; 
			}

		} else {
			x[0] = (x[2] + (leftchop==0?x[1]:x[3]) ) / 2;
		}

		y[0] = unfitnessf( x[0], p);

	} while (absol(x[3]-x[1]) > limit);	

	return x[2];
}



//=======


double
quadfit1d_  (double limit, double a, double b, double (*unfitnessf)(double, const void *), const void *p)
{	
	bool_t rightchop_old = FALSE, rightchop_cur = FALSE, equality = FALSE;
	int i;
	double x[4];
	double y[4];

	x[1] = a > b? b: a;
	x[2] = (a+b)/2;
	x[3] = b > a? b: a;

	for (i = 1; i < 4; i++) {
		y[i] = unfitnessf	(x[i], p);
	}

	x[0] = parabolic_center_x (x, y);
	if (x[0] < x[1] || x[0] > x[3]) {
		x[0] = (x[1] + x[3]) / 2; 
	}
	y[0] = unfitnessf( x[0], p);

	do {
		assert (!(x[1] > x[2] || x[2] > x[3] || x[1] > x[3]));

		equality = FALSE;
		rightchop_old = rightchop_cur;

		if (x[0] < x[2] && y[0] <= y[2]) { rightchop_cur = TRUE;
			x[3] = x[2];
			y[3] = y[2];	
			x[2] = x[0];
			y[2] = y[0];
		} else
		if (x[0] > x[2] && y[0] > y[2]) { rightchop_cur = TRUE;
			x[3] = x[0];
			y[3] = y[0];
		} else
		if (x[0] < x[2] && y[0] > y[2]) { rightchop_cur = FALSE;
			x[1] = x[0];
			y[1] = y[0];
		} else
		if (x[0] > x[2] && y[0] <= y[2]) { rightchop_cur = FALSE;
			x[1] = x[2];
			y[1] = y[2];
			x[2] = x[0];
			y[2] = y[0];
		} else {						  equality = TRUE;;
			x[0] = (x[1] + x[3])/2;
		}

		if (equality) {
			// do nothing
		} else if (rightchop_old != rightchop_cur) {

			x[0] = parabolic_center_x (x, y);
			if (x[0] <= x[1] || x[0] >= x[3]) {
				x[0] = (x[1] + x[3]) / 2; 
			}

		} else {
			x[0] = (x[2] + (rightchop_cur?x[1]:x[3]) ) / 2;
		}

		y[0] = unfitnessf( x[0], p);

	} while (absol(x[3]-x[1]) > limit);	

	return x[2];
}

//============ center adjustment begin

double
quadfit1d	(double limit, double a, double b, double (*unfitnessf)(double, const void *), const void *p)
{
	double cente = (a+b)/2;
	double delta = (b>a?b-a:a-b)/2;
	double ei, ej, ek;

	cente = 0;

	ei = unfitnessf	(cente - delta, p);
	ej = unfitnessf	(cente        , p);
	ek = unfitnessf	(cente + delta, p);

	do {	

		if (ei >= ej && ej <= ek) {

			return quadfit1d_2 (limit, cente - delta, cente + delta, unfitnessf, p);

//			delta = delta / 4;
//			ei = unfitnessf	( cente - delta, p);
//			ek = unfitnessf	( cente + delta, p);

		} else
		if (ej >= ei && ei <= ek) {

			cente -= delta;

			ek = ej;
			ej = ei; 
			ei = unfitnessf	( cente - delta, p);

		} else
		if (ei >= ek && ek <= ej) {

			cente += delta;

			ei = ej;
			ej = ek;
			ek = unfitnessf	( cente + delta, p);

		}

	} while (
		delta > limit 
	);

	return cente;
}

//============ center adjustment end
