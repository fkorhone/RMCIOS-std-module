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

#ifndef STD_CHANNELS_H
#define STD_CHANNELS_H

#ifdef __cplusplus
extern "C" {
#endif

extern void init_std_channels(const struct context_rmcios *context) ;
extern void init_control_channels(const struct context_rmcios *context) ;
extern void init_device_channels(const struct context_rmcios *context) ;
extern void init_legacy_channels(const struct context_rmcios *context) ;
extern void init_math_channels(const struct context_rmcios *context) ;
extern void init_parse_channels(const struct context_rmcios *context) ;
extern void init_system_channels(const struct context_rmcios *context) ;
extern void init_util_channels(const struct context_rmcios *context) ;
extern void init_dma_channels(const struct context_rmcios *context) ;
extern void init_encoding_channels(const struct context_rmcios *context) ;

#ifdef __cplusplus
}
#endif

#endif
