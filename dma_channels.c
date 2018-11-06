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
 * Differential mobility analyzer related channels
 *
 * Changelog: (date,who,description)
 */

#include "RMCIOS-functions.h"
#include "dma_formula.h"
#include <string.h>

struct dma_data
{
   // Dma dimensions:
   float r1;
   float r2;
   float length;

   // Diameter calculation values
   float dp;  // diameter 
   float V;   // voltage
   float qs;  // l/min
   float T;   // 'C
   float p;   // Pa

   int qs_channel;
   int p_channel;
   int T_channel;
   int r1_channel;
   int r2_channel;
   int length_channel;
};

void dma_p_subfunc (struct dma_data *this,
                    const struct context_rmcios *context, int id,
                    enum function_rmcios function,
                    enum type_rmcios paramtype,
                    union param_rmcios returnv,
                    int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case read_rmcios:
      if (this == NULL)
         break;
      return_float (context, paramtype, returnv, this->p);
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      this->p = param_to_float (context, paramtype, param, 0);
      this->V =
         calculate_voltage (this->dp, this->T + 273.15, this->p,
                            this->r1, this->r2, this->length, 
                            this->qs / 60000);
      write_f (context, linked_channels (context, id), this->V);
      break;
   default:
      break;
   }
}

void dma_T_subfunc (struct dma_data *this,
                    const struct context_rmcios *context, int id,
                    enum function_rmcios function,
                    enum type_rmcios paramtype,
                    union param_rmcios returnv,
                    int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case read_rmcios:
      if (this == NULL)
         break;
      return_float (context, paramtype, returnv, this->T);
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      this->T = param_to_float (context, paramtype, param, 0);
      this->V =
         calculate_voltage (this->dp, this->T + 273.15, this->p,
                            this->r1, this->r2, this->length, 
                            this->qs / 60000);

      write_f (context, linked_channels (context, id), this->V);
      break;
   default:
      break;
   }
}

void dma_qs_subfunc (struct dma_data *this,
                     const struct context_rmcios *context, int id,
                     enum function_rmcios function,
                     enum type_rmcios paramtype,
                     union param_rmcios returnv,
                     int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case read_rmcios:
      if (this == NULL)
         break;
      return_float (context, paramtype, returnv, this->qs);
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      this->qs = param_to_float (context, paramtype, param, 0);
      this->V =
         calculate_voltage (this->dp, this->T + 273.15, this->p,
                            this->r1, this->r2, this->length, 
                            this->qs / 60000);
      write_f (context, linked_channels (context, id), this->V);
      break;
   default:
      break;
   }
}

void dma_v_subfunc (struct dma_data *this,
                    const struct context_rmcios *context, int id,
                    enum function_rmcios function,
                    enum type_rmcios paramtype,
                    union param_rmcios returnv,
                    int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case read_rmcios:
      if (this == NULL)
         break;
      return_float (context, paramtype, returnv, this->V);
      break;
   default:
      break;
   }
}

void dma_dp_subfunc (struct dma_data *this,
                     const struct context_rmcios *context, int id,
                     enum function_rmcios function,
                     enum type_rmcios paramtype,
                     union param_rmcios returnv,
                     int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case read_rmcios:
      if (this == NULL)
         break;
      return_float (context, paramtype, returnv, this->dp);
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      this->dp = param_to_float (context, paramtype, param, 0);
      this->V =
         calculate_voltage (this->dp, this->T + 273.15, this->p,
                            this->r1, this->r2, this->length, 
                            this->qs / 60000);
      write_f (context, linked_channels (context, id), this->V);
      break;
   default:
      break;
   }
}

void dma_class_func (struct dma_data *this,
                     const struct context_rmcios *context, int id,
                     enum function_rmcios function,
                     enum type_rmcios paramtype,
                     union param_rmcios returnv,
                     int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "Differential mobility analyzer channel help: \r\n"
                     "Calculates DMA voltage using sheath flow, pressure"
                     ", and temperature.\r\n"
                     "Gets values from configured constants/channels\r\n"
                     " create dma newname\r\n"
                     " setup newname qs[l/min] | p[Pa] | T['C] |"
                     " r1[m] r2[m] length[m] | qs_channel | p_channel |"
                     " T_channel | r1_channel r2_channel length_channel  \r\n"
                     " subchannels: dma_t dma_p dma_qs dma_v dma_dp\r\n"
                     "\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      else
      {
         int id;

         // allocate new data
         this = (struct dma_data *) 
                allocate_storage (context, sizeof (struct dma_data), 0);
         if (this == NULL)
            info (context, context->errors, "Could not create dma!\r\n");

         // create channel
         id = create_channel_param (context, paramtype, param, 0, 
                                    (class_rmcios) dma_class_func, this); 
         create_subchannel_str (context, id, "_t",
                                (class_rmcios) dma_T_subfunc, this);
         create_subchannel_str (context, id, "_p",
                                (class_rmcios) dma_p_subfunc, this);
         create_subchannel_str (context, id, "_qs",
                                (class_rmcios) dma_qs_subfunc, this);
         create_subchannel_str (context, id, "_v",
                                (class_rmcios) dma_v_subfunc, this);
         create_subchannel_str (context, id, "_dp",
                                (class_rmcios) dma_dp_subfunc, this);

         // default values:
         this->p = 101325; // Pa
         this->T = 20;     // 'C
         this->qs = 5.0;   // l/min   

         // Dma dimensions:
         this->r1 = 0.025;      // m
         this->r2 = 0.033;      // m
         this->length = 0.28;   // m

         this->qs_channel = 0;
         this->p_channel = 0;
         this->T_channel = 0;
         this->r1_channel = 0;
         this->r2_channel = 0;
         this->length_channel = 0;

         this->V = calculate_voltage (this->dp, this->T + 273.15, this->p,
                               this->r1, this->r2, this->length,
                               this->qs / 60000);
      }
      break;

   case setup_rmcios:  
      if (num_params < 1)
         break;
      this->qs = param_to_float (context, paramtype, param, 0);

      if (num_params < 2)
         break;
      this->p = param_to_float (context, paramtype, param, 1);

      if (num_params < 3)
         break;
      this->T = param_to_float (context, paramtype, param, 2);

      if (num_params < 6)
         break;
      this->r1 = param_to_float (context, paramtype, param, 3);
      this->r2 = param_to_float (context, paramtype, param, 4);
      this->length = param_to_float (context, paramtype, param, 5);

      if (num_params < 7)
         break;
      this->qs_channel = param_to_int (context, paramtype, param, 6);
      if (num_params < 8)
         break;
      this->p_channel = param_to_int (context, paramtype, param, 7);
      if (num_params < 9)
         break;
      this->T_channel = param_to_int (context, paramtype, param, 8);

      if (num_params < 12)
         break;
      this->r1_channel = param_to_int (context, paramtype, param, 9);
      this->r2_channel = param_to_int (context, paramtype, param, 10);
      this->length_channel = param_to_int (context, paramtype, param, 11);
      break;

   case write_rmcios:
      if (this == NULL)
         break;

      // Update values from specified channels (if specified)
      if (this->qs_channel != 0)
         this->qs = read_f (context, this->qs_channel);
      if (this->p_channel != 0)
         this->p = read_f (context, this->p_channel);
      if (this->T_channel != 0)
         this->T = read_f (context, this->T_channel);
      if (this->r1_channel != 0)
         this->r1 = read_f (context, this->r1_channel);
      if (this->r2_channel != 0)
         this->r2 = read_f (context, this->r2_channel);
      if (this->length_channel != 0)
         this->length = read_f (context, this->length_channel);

      // set diameter value
      if (num_params > 0)
         this->dp = param_to_float (context, paramtype, param, 0);

      // calculate voltage
      this->V = calculate_voltage (this->dp, this->T + 273.15, this->p,
                            this->r1, this->r2, this->length, this->qs / 60000);

      write_f (context, linked_channels (context, id), this->V);
      break;
   case read_rmcios:
      if (this == NULL)
         break;
      {
         return_float (context, paramtype, returnv, this->V);
      }
      break;
   }
}

struct dma_error_data
{
   // Dma dimensions:
   float r1;
   float r2;
   float length;

   // Monitoring channels
   int dp_channel;
   int qs_channel;
   int v_channel;
   int p_channel;
   int T_channel;
};

void dma_error_class_func (struct dma_error_data *this,
                           const struct context_rmcios *context, int id,
                           enum function_rmcios function,
                           enum type_rmcios paramtype,
                           union param_rmcios returnv,
                           int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "Estimates dma error in % with measured values "
                     " from specified channels/constants\r\n"
                     " create dma_error newname \r\n"
                     " setup newname 0=r1 1=r2 2=length 3=dp_channel"
                     " 4=qs_channel 5=v_channel | 6=p_channel 7=T_channel \r\n"
                     " read newname # read calculated dma error.\r\n");
      break;
   case create_rmcios:
      if (num_params < 1)
         break;
      else
      {
         // allocate new data
         this = (struct dma_error_data *) 
                 allocate_storage (context, sizeof (struct dma_error_data), 0);
 
         if (this == NULL)
            info (context, context->errors,
                  "Could not allocate memory for channel: dma error!\r\n");
         // create channel
         create_channel_param (context, paramtype, param, 0, 
                               (class_rmcios) dma_error_class_func, this);
         this->dp_channel = 0;
         this->qs_channel = 0;
         this->v_channel = 0;
         this->p_channel = 0;
         this->T_channel = 0;
      }
      break;
   case setup_rmcios: 
      if (this == NULL)
         break;
      if (num_params < 6)
         break;
      this->r1 = param_to_float (context, paramtype, param, 0);
      this->r2 = param_to_float (context, paramtype, param, 1);
      this->length = param_to_float (context, paramtype, param, 2);
      this->dp_channel = param_to_int (context, paramtype, param, 3);
      this->qs_channel = param_to_int (context, paramtype, param, 4);
      this->v_channel = param_to_int (context, paramtype, param, 5);
      if (num_params < 8)
         break;
      this->p_channel = param_to_int (context, paramtype, param, 6);
      this->T_channel = param_to_int (context, paramtype, param, 7);

      break;
   case read_rmcios:
      if (this == NULL)
         break;
      {
         float dp = 0;
         float qs = 0;
         float v = 0;
         float p = 101325;
         float T = 21;

         if (this->dp_channel == 0
             || this->qs_channel == 0 || this->v_channel == 0)
         {
            // return NAN
            return_float (context, paramtype, returnv, 1.0 / 0.0); 
            break;
         }

         dp = read_f (context, this->dp_channel);
         qs = read_f (context, this->qs_channel);
         v = read_f (context, this->v_channel);

         if (this->p_channel != 0)
         {
            // optionally pressure
            p = read_f (context, this->p_channel);     
         }
         if (this->T_channel != 0)
         {
            // optionally temperature
            T = read_f (context, this->T_channel); 
         }

         float dp_calc;
         dp_calc =
            calculate_dp (v, dp, T + 273.15, p, this->r1, this->r2,
                          this->length, qs / 60000);
         dp_calc = (1.0 - dp / dp_calc) * 100.0;
         return_float (context, paramtype, returnv, dp_calc);
      }
      break;

   default:
      break;
   }
}

void init_dma_channels (const struct context_rmcios *context)
{
   // create dma creator channel
   create_channel_str (context, "dma", (class_rmcios) dma_class_func, NULL); 
   
   // create dma creator channel
   create_channel_str (context, "dma_error", 
                       (class_rmcios) dma_error_class_func, NULL);        
   
}

