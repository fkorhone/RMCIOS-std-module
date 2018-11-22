/* 
RMCIOS - Reactive Multipurpose Control Input Output System
Copyright (c) 2018 Frans Korhonen

RMCIOS was originally developed at Institute for Atmospheric 
and Earth System Research / Physics, Faculty of Science, 
University of Helsinki, Finland

Assistance, experience and feedback from following persons have been 
critical for development of RMCIOS: Erkki Siivola, Juha Kangasluoma, 
Lauri Ahonen, Ella Häkkinen, Pasi Aalto, Joonas Enroth, Runlong Cai, 
Markku Kulmala and Tuukka Petäjä.

This file is part of RMCIOS. This notice was encoded using utf-8.

RMCIOS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RMCIOS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public Licenses
along with RMCIOS.  If not, see <http://www.gnu.org/licenses/>.
*/

/* 2015 Frans Korhonen. University of Helsinki.
 * Forumula for calulating DMA electrode voltage for specified dp
 * (particle diameter).
 */

#ifndef dma_formula_h
#define dma_formula_h

#include <math.h>

// Default T=293K
// Default P=101325Pa

// e=1.602E-19 C
#define e 1.602176462E-19
#define pi 3.14159265359
// T_0=296.16 K
#define T_0 296.15
// P0=101325 Pa
#define p_0 101325
// n_0=1.83245E-5 Kg/(ms)
#define n_0 1.83245E-5
//qs = m^3/s

float calculate_cunningham(float dp, float T, float p)
{
	float lambda=67.3E-9*pow(T/T_0,2.0)*(p_0/p)*( (T_0+110.4L)/(T+110.4L) ) ;
	float cunn = 1.0L+(2.0*lambda/dp*(1.165+0.483*exp(-0.997*dp/(2.0L*lambda))));
	return cunn ;
}

float calculate_viscocity(float T)
{
	float viscocity=n_0*pow(T/T_0,3.0/2.0)*((T_0+110.4)/(T+110.4)) ; 
	return viscocity ;
}

float calculate_mobility( float dp, float T,float p)
{
	float viscocity=calculate_viscocity(T) ;
	float cunn=calculate_cunningham(dp,T,p) ;
	float mobility ;
	mobility = e * cunn / (3.0 * pi * viscocity * dp) ; 
	return mobility ;
}

float calculate_voltage(float dp,float T,float p,
                        float r1, float r2,float length, float qs)
{
	float voltage ;
	float mobility=calculate_mobility(dp,T,p) ;
	voltage = qs * log(r2 / r1) / (2.0 * pi * mobility * length) ; 
	return voltage ;
}

// Calculate dp in logaritmic intervals
void calculate_dp_table(float dp_min, float dp_max, int n, float dp_table[] )
{
	int i ;
	for(i=0 ; i<n ; i++ )
	{
		dp_table[i]= dp_min*exp( ((float)i)/(n-1)*log(dp_max/dp_min) ) ; 
	}
}

// Calculates dp. Uses dp_cunn for calculating cunninghamm correction.
float calculate_dp(float voltage, float dp_cunn, float T, float p,
                   float r1, float r2,float length, float qs)
{
	float dp ;
	float viscocity=calculate_viscocity(T) ;
	float cunn=calculate_cunningham(dp_cunn,T,p) ;
	dp = voltage* 2.0 * pi * e * cunn * length / 
        (qs * log(r2 / r1) * 3.0 * pi * viscocity);
	return dp ;
}

#endif


