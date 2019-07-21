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

/* 
 * Standard channels module implementation. 
 *
 * Changelog: (date,who,description)
 * */

#include "RMCIOS-functions.h"
#include <math.h>
#include <stddef.h>

////////////////////////////////////////////////
// scaling channels:
////////////////////////////////////////////////
float sqrt_func (float a, float b)
{
   return sqrt (a);
}

struct oper
{
   float valueA;
   float valueB;
   int valueA_channel;
   int valueB_channel;
};

void generic_std_operator_class_func (struct oper *this,
                                  const struct context_rmcios *context,
                                  int id, enum function_rmcios function,
                                  enum type_rmcios paramtype,
                                  struct combo_rmcios *returnv,
                                  int num_params,
                                  const union param_rmcios param,
                                  float (*opfunc) (float, float))
{
   switch (function)
   {
   case setup_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      this->valueB = param_to_float (context, paramtype, param, 0);
      if (num_params < 2)
         break;
      this->valueA_channel = param_to_int (context, paramtype, param, 1);
      if (num_params < 3)
         break;
      this->valueB_channel = param_to_int (context, paramtype, param, 2);
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      if (this->valueA_channel != 0)
         // update A from channel
         this->valueA = read_f (context, this->valueA_channel); 
      if (this->valueB_channel != 0)
         // update B from channel
         this->valueB = read_f (context, this->valueB_channel); 
      if (num_params >= 1)
         this->valueA = param_to_float (context, paramtype, param, 0);
      write_f (context, linked_channels (context, id),
               opfunc (this->valueA, this->valueB));
      break;
   case read_rmcios:
      if (this == NULL)
         break;
      if (this->valueA_channel != 0)
         // update A from channel
         this->valueA = read_f (context, this->valueA_channel); 
      if (this->valueB_channel != 0)
         // update B from channel
         this->valueB = read_f (context, this->valueB_channel); 
      return_float (context, returnv,
                    opfunc (this->valueA, this->valueB));
      break;
   default:
      break;
   }
}

///////////////////////////////////////////////
//! Channel for square root operation
///////////////////////////////////////////////
void sqrt_class_func (struct oper *this, struct context_rmcios *context,
                      int id, enum function_rmcios function,
                      enum type_rmcios paramtype, 
                      struct combo_rmcios *returnv,
                      int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, returnv,
                     "help for sqrt square root channel:\r\n"
                     " create sqrt newname\r\n"
                     " write newname X # calculate result=sqrt(X) \r\n"
                     " read newname #read result\r\n"
                     " link newname channel #link result to channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      
      // allocate new data
      this = (struct oper *) 
             allocate_storage (context, sizeof (struct oper), 0);       

      //default values :
      this->valueA = 1;
      this->valueB = 1;
      this->valueA_channel = 0;
      this->valueB_channel = 0;
      
      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) sqrt_class_func, this); 
      break;
   }
   // use generic implementation
   generic_std_operator_class_func (this, context, id, function, paramtype, 
                                returnv, num_params, param, sqrt_func); 
}

void init_std_math_channels (const struct context_rmcios *context)
{
   create_channel_str (context, "sqrt", (class_rmcios) sqrt_class_func, NULL);
}
