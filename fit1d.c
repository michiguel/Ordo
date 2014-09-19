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
			y[0] = unfitnessf( x[0], p);
		} else if (rightchop < 3 && leftchop < 3) {

			x[0] = parabolic_center_x (x, y);
			if (x[0] <= x[1] || x[0] >= x[3]) {
				x[0] = (x[1] + x[3]) / 2; 
			}
			y[0] = unfitnessf( x[0], p);

		} else {
			double half = (x[3]-x[1])/2;
			x[0] = x[2];

			if (x[3]-x[2] > 2*(x[2]-x[1]) ) { // lower third

				do {
					x[0] = x[0] + (x[0] - x[1]);
					y[0] = unfitnessf( x[0], p);
				} while (x[0] < half && y[0] <= y[2]);

			} else 
			if (x[3]-x[2] < (x[2]-x[1])/2 ) { // upper third

				do {
					x[0] = x[0] - (x[3] - x[0]);
					y[0] = unfitnessf( x[0], p);
				} while (x[0] > half && y[0] <= y[2]);

			} else {
				x[0] = (x[2] + (leftchop==0?x[1]:x[3]) ) / 2;
				y[0] = unfitnessf( x[0], p);
			}
		}

	} while (absol(x[3]-x[1]) > limit);	

	return x[2];
}


double
quadfit1d	(double limit, double a, double b, double (*unfitnessf)(double, const void *), const void *p)
{
	double cente = (a+b)/2;
	double delta_neg, delta_pos;
	double ei, ej, ek;
	
	delta_pos = delta_neg = (b>a?b-a:a-b)/2;

	ei = unfitnessf	(cente - delta_neg, p);
	ej = unfitnessf	(cente            , p);
	ek = unfitnessf	(cente + delta_pos, p);

	for (;;) {	

		if (ei >= ej && ej <= ek) {

			return quadfit1d_2 (limit, cente - delta_neg, cente + delta_pos, unfitnessf, p);

		} else
		if (ej >= ei && ei <= ek) {

			delta_neg *= 2;

			ek = ej;
			ej = ei; 
			ei = unfitnessf	( cente - delta_neg, p);

		} else
		if (ei >= ek && ek <= ej) {

			delta_pos *= 2;

			ei = ej;
			ej = ek;
			ek = unfitnessf	( cente + delta_pos, p);

		}

	} 

}

//============ center adjustment end
