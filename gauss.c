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
#include "gauss.h"

#define M_PI 3.14159265358979323846

//static Gauss_A = 1.0 / sqrt(2 * M_PI);

double gauss_density(double x) {return (1.0 / sqrt(2 * M_PI)) * exp(-0.5*x*x);}

double gauss_integral(double z)
{
//http://finance.bi.no/~bernt/gcc_prog/recipes/recipes/node23.html
	double b1;
	double b2;
	double b3;	
	double b4;
	double b5;
	double p ;
	double c2;
	
	double a; 
	double t; 
	double b; 
	double n; 	

	if (z >  6.0) return 1.0;
	if (z < -6.0) return 0.0;

	b1 = 0.31938153;
	b2 = -0.356563782;
	b3 = 1.781477937;	
	b4 = -1.821255978;
	b5 = 1.330274429;
	p  = 0.2316419;
	c2 = 0.3989423;
	
	a  = fabs(z);
	t = 1.0/(1.0+a*p);
	b = c2*exp((-z)*(z/2.0));
	n = ((((b5*t+b4)*t+b3)*t+b2)*t+b1)*t;	
	n = 1.0-b*n;
	if (z < 0.0) n = 1.0-n;
	return n;
}

static double symrange(double z) {return 2.0 * (gauss_integral(z) - 0.5);}

double confidence2x (double confidence)
{
	double i,j,k,rj;
	int x;

	i = 0;
	j = 3;
	k = 6;
	
	for (x = 0; x < 32; x++) {
		rj = symrange(j);
		if (confidence > rj) {
			i = j; 
		} else {
			k = j; 
		}
		j = (i+k)/2;
	}
	return j;
}


