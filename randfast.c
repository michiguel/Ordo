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


#include "randfast.h"
#include "datatype.h"

/*
|
|	Random number generator taken from
|	http://www.burtleburtle.net/bob/rand/talksmall.html
|
*/

typedef struct ranctx { 
	uint32_t a; 
	uint32_t b; 
	uint32_t c; 
	uint32_t d; 
} ranctx;

static ranctx Rndseries;

#define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))

static uint32_t 
ranval( ranctx *x ) 
{
    uint32_t e = x->a - rot(x->b, 27);
    x->a = x->b ^ rot(x->c, 17);
    x->b = x->c + x->d;
    x->c = x->d + e;
    x->d = e + x->a;
    return x->d;
}

static void raninit ( ranctx *x, uint32_t seed ) 
{
    uint32_t i;
    x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
    for (i = 0; i < 20; ++i) {
        (void)ranval(x);
    }
}

void randfast_init (uint32_t seed)
{
	raninit (&Rndseries, seed); 
}

uint32_t randfast32 (void)
{
	return ranval (&Rndseries); 
}


//==========================================
#include "gauss.h"

static double
rand_area (void)
{
	uint32_t r;
	double rr;
	do {
		r = randfast32();
	} while (r == 0);
	r &= 8191;
	rr = (double) r;
	rr = rr/8192;
	return rr;
}


static double
rand_gauss_normalized(void)
{
	double xi, yi, area, slope;
	double limit = 0.00001;
	int n;

	area = rand_area();
	n = 0;
	xi = 0;
	do {
		yi = gauss_integral (xi) - area;
		slope = gauss_density (xi);
		xi = xi - yi / slope;
		n++;
	} while (n < 100 && (yi > limit || yi < -limit));

	return xi;
}

double
rand_gauss(double x, double s)
{
	double z = rand_gauss_normalized();
	return x + z * s;
}
