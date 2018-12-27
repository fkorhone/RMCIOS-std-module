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

/* 2017 Frans Korhonen. University of Helsinki. 
 * Standard channels module implementation. 
 *
 * Changelog: (date,who,description) */

#include "RMCIOS-functions.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/////////////////////////////////////////////////
// TSI 4000 series flowmeter channel
/////////////////////////////////////////////////
struct tsi_flow_data
{
   int flow_channel;
   int t_channel;
   int p_channel;
   float flow;
   float temp;
   float pressure;

   float timeout_time;
   int serial_channel;
   int timeout_channel;
   char receive_buffer[20];
   int rIndex;
   int id;

   int ext_p_channel;
};

void tsi_temperature_subchan_func (struct tsi_flow_data *this,
                                   const struct context_rmcios *context,
                                   int id, enum function_rmcios function,
                                   enum type_rmcios paramtype,
                                   union param_rmcios returnv, int num_params,
                                   const union param_rmcios param)
{
   switch (function)
   {
   case read_rmcios:
      return_float (context, paramtype, returnv, this->temp);
      break;
   default:
      break;
   }
}

void tsi_pressure_subchan_func (struct tsi_flow_data *this,
                                const struct context_rmcios *context, int id,
                                enum function_rmcios function,
                                enum type_rmcios paramtype,
                                union param_rmcios returnv,
                                int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case read_rmcios:
      return_float (context, paramtype, returnv, this->pressure);
      break;
   default:
      break;
   }
}

void tsi_flow_subchan_func (struct tsi_flow_data *this,
                            const struct context_rmcios *context, int id,
                            enum function_rmcios function,
                            enum type_rmcios paramtype,
                            union param_rmcios returnv,
                            int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case read_rmcios:
      return_float (context, paramtype, returnv, this->flow);
      break;
   default:
      break;
   }
}

void tsi_flow_class_func (struct tsi_flow_data *this,
                          const struct context_rmcios *context, int id,
                          enum function_rmcios function,
                          enum type_rmcios paramtype,
                          union param_rmcios returnv,
                          int num_params, const union param_rmcios param)
{
   char c, str[2];
   c = 0;
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "TSI 4000 series flowmeter channel help: \r\n"
                     " create tsi_flow newname \r\n"
                     " setup serial_channel | timeout_timer timeout_time "
                     " | ext_p_channel\r\n"
                     " link flow_out_channel # link to channel that will"
                     " output flow\r\n"
                     " write data # (serial data input, timer reset input) \r\n"
                     " read # read latest flow rate\r\n"
                     " creates subchannels :\r\n"
                     "  newname_t # Temperature channel. \r\n"
                     "  newname_p # Pressure channel. \r\n"
                     "  newname_flow # Flow channel.\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      this = (struct tsi_flow_data *) 
              allocate_storage (context, sizeof (struct tsi_flow_data), 0);

      this->id =
         create_channel_param (context, paramtype, param, 0,
                               (class_rmcios) tsi_flow_class_func, this);
      this->t_channel =
         create_subchannel_str (context, this->id, "_t",
                                (class_rmcios)
                                tsi_temperature_subchan_func, this);
      this->p_channel =
         create_subchannel_str (context, this->id, "_p",
                                (class_rmcios) tsi_pressure_subchan_func, this);
      this->flow_channel =
         create_subchannel_str (context, this->id, "_flow",
                                (class_rmcios) tsi_flow_subchan_func, this);

      // default values:
      this->serial_channel = 0;
      this->flow = NAN;
      this->temp = NAN;
      this->pressure = NAN;
      this->rIndex = 0;
      this->ext_p_channel = 0;
      break;

   case setup_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      this->serial_channel = param_to_int (context, paramtype, param, 0);
      link_channel (context, this->serial_channel, this->id);
      if (num_params < 3)
         break;
      this->timeout_channel = param_to_int (context, paramtype, param, 1);
      this->timeout_time = param_to_float (context, paramtype, param, 2);
      link_channel (context, this->timeout_channel, this->id);
      // Start timeout timer
      write_f (context, this->timeout_channel, this->timeout_time); 
      if (num_params < 4)
         break;
      // external pressure measurement channel
      this->ext_p_channel = param_to_int (context, paramtype, param, 3); 
      break;

   case read_rmcios:
      return_float (context, paramtype, returnv, this->flow);
      break;

   case write_rmcios:  
      // Serial receive and receive timeout
      if (this == NULL)
      {
         printf ("Timer data NULL\r\n");
         break;
      }

      if (this->timeout_channel != 0)
         // restart timeout timer
         write_f (context, this->timeout_channel, this->timeout_time);  
      if (num_params < 1)
      // Timeout reset
      { 
         // request new value
         write_str (context, this->serial_channel, "DAFTP0001\r\n", 0); 
         break;
      }

      param_to_string (context, paramtype, param, 0, 2, str);
      c = str[0];

      if (c == 0)
      {
         // request new value
         write_str (context, this->serial_channel, "DAFTP0001\r\n", 0); 
      }
      if (c == '\n')
      {
         this->receive_buffer[this->rIndex] = 0;
         this->rIndex = 0;
         if (this->receive_buffer[0] != 'O')
         {
            char *temp_str;
            char *pressure_str;
            this->flow = atof (this->receive_buffer); // l/min
            temp_str = strstr (this->receive_buffer, ",") + 1;
            if (temp_str != (void *) 1)
            {
               this->temp = atof (temp_str);    // 'C
               pressure_str = strstr (temp_str, ",") + 1;
               this->pressure = atof (pressure_str) * 1000.0;   // to Pa
            }
            write_f (context, linked_channels (context, id), this->flow);
            write_f (context,
                     linked_channels (context, this->flow_channel), this->flow);
            write_f (context,
                     linked_channels (context, this->t_channel), this->temp);
            write_f (context,
                     linked_channels (context, this->p_channel),
                     this->pressure);

            if (this->ext_p_channel != 0)
            // update flowmeter pressure. 
            // for getting volume flow from external pressure gauge
            {   
               char ps[15];
               sprintf (ps, "SP%06.2f\r\n",
                        read_f (context, this->ext_p_channel) / 1000.0);
               if (this->serial_channel != 0)
                  write_str (context, this->serial_channel, ps, 0);
            }
            if (this->serial_channel != 0)
               // request new value
               write_str (context, this->serial_channel, "DAFTP0001\r\n", 0);
         }
      }
      else
      {
         this->receive_buffer[this->rIndex++] = c;
         if (this->rIndex > 18)
            this->rIndex = 18;
      }
      break;

   default:
      break;
   }
}

////////////////////////////////////////////
// ATM High voltage supply driver channel //
////////////////////////////////////////////
struct atm_hv_data
{
   char id[3];
   int serial_channel;
   int wait_channel;
};

void atm_hv_class_func (struct atm_hv_data *this,
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
                     "hy atm high-voltage supply help:\r\n"
                     "create atm_hv newname\r\n"
                     " setup newname wait_channel serial_channel address\r\n"
                     " write newname voltage #Set voltage\r\n"
                     " read newname voltage #read voltage monitor\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;

      // allocate new data
      this = (struct atm_hv_data *) 
              allocate_storage (context, sizeof (struct atm_hv_data), 0); 

      //default values :
      this->serial_channel = 0;
      this->id[0] = '*';
      this->id[1] = '0';
      this->id[2] = 0;

      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) atm_hv_class_func, this); 
      break;

   case setup_rmcios:
      if (this == NULL)
      {
         break;
      }
      if (num_params < 2)
         break;
      this->wait_channel = param_to_int (context, paramtype, param, 0);
      this->serial_channel = param_to_int (context, paramtype, param, 1); 
    
      if (num_params < 3)
         break;
      param_to_string (context, paramtype, param, 2, 2, this->id);
      break;

   case write_rmcios:

      if (this == NULL)
      {
         break;
      }
      if (num_params < 1)
         break;
      else
      {
         char command[20];
         float voltage = param_to_float (context, paramtype, param, 0);
         sprintf (command, "%ss%.1f\r\n", this->id, voltage);
         write_str (context, this->serial_channel, command, 0);
      }
      break;
   case read_rmcios:
      if (this == NULL)
      {
         break;
      }
      char command[20];
      sprintf (command, "%sm\r\n", this->id);
      write_str (context, this->serial_channel, command, 0);
      // wait for response
      write_f (context, this->wait_channel, 0.3);       
      float test;
      test = read_f (context, this->serial_channel);
      return_float (context, paramtype, returnv, test);
      break;
   }
}

///////////////////////////////////////////////////////////
//! Platinum thermistor conversion channel
///////////////////////////////////////////////////////////
struct pt_temperature_data
{
   float R0;
   float a;
   float b;
   float c;
   float T;
   float R;
   int linked_channels;
};

void pt_temperature_class_func (struct pt_temperature_data *this,
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
                     "pt channel - "
                     "channel for platinum temperature sensor conversion\r\n"
                     "create pt newname\r\n"
                     "setup newname R0 | a b | c \r\n"
                     "  -Set the sensor coefficients \r\n"
                     "write newname R \r\n"
                     "  -write the sensor resistance for calculations\r\n"
                     "read newname \r\n"
                     "  -read the latest calculated temperature \r\n"
                     "link newname linked\r\n"
                     "  -set channels that the calculated temperatures"
                     " are sent on update.\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      else
      {
         this = (struct pt_temperature_data *) 
                 allocate_storage (context, 
                                   sizeof (struct pt_temperature_data), 0);
         if (this == NULL)
         {
            printf ("Unable to allocate memory for ramp data!");
            break;
         }
         this->R0 = 1000;       // PT1000
         this->a = 3.9083E-03;  // ITS-90
         this->b = -5.7750E-07; // ITS-90
         this->c = -4.1830E-12; // ITS-90
         this->T = 0;
         this->R = 0;

         // create the channel
         create_channel_param (context, paramtype, param, 0, 
                               (class_rmcios) pt_temperature_class_func, this);
         break;
      }
   case setup_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      this->R0 = param_to_float (context, paramtype, param, 0);
      if (num_params < 3)
         break;
      this->a = param_to_float (context, paramtype, param, 1);
      this->b = param_to_float (context, paramtype, param, 2);
      if (num_params < 4)
         break;
      this->c = param_to_float (context, paramtype, param, 3);
      break;

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
      {
         float R = param_to_float (context, paramtype, param, 0);
         float a = this->a;
         float b = this->b;
         float c = this->c;
         float R0 = this->R0;
         this->T =
            (-1.0 * R0 * a +
             sqrt (R0 * R0 * a * a - 4 * R0 * b * (R0 - R))) / (2 * R0 * b);
         if (!isnormal (this->T))
            this->T = 9999;
         write_f (context, linked_channels (context, id), this->T);
      }
      break;
   }
}

//////////////////////////////////////////////////////////////
//! Concentration scaling channel
//////////////////////////////////////////////////////////////
struct conc_data
{
   int timer_channel;
   int counter_channel;
   int sample_flow_channel;

   // Concentration calculation values
   float sample_flow; //unit: cm^3/s
};

void conc_class_func (struct conc_data *this,
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
                     "Calculates concentration (1/cm^3) \r\n"
                     "setup conc 0=sample_flow(l/min)"
                     " 1=counter_channel "
                     " 2=timer_channel "
                     " | 3=sample_flow_channel\r\n"
                     "read conc #calculate and read the concentration\r\n"
                     "write conc #empty write clear counter"
                     " and timer for concentration\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      else
      {
         // allocate new data
         this = (struct conc_data *) 
                allocate_storage (context, sizeof (struct conc_data), 0);  
         if (this == NULL)
            info (context, context->errors, "Could not create conc!\r\n");
         
         // create channel
         create_channel_param (context, paramtype, param, 0, 
                               (class_rmcios) conc_class_func, this); 
      }

      this->sample_flow = 0;
      this->counter_channel = 0;
      this->timer_channel = 0;
      this->sample_flow_channel = 0;
      break;

   case setup_rmcios:  
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      this->sample_flow =
         param_to_float (context, paramtype, param, 0) * 16.6666667;
      this->sample_flow_channel = 0;
      if (num_params < 3)
         break;
      this->counter_channel = param_to_int (context, paramtype, param, 1);
      this->timer_channel = param_to_int (context, paramtype, param, 2);

      if (num_params < 4)
         break;
      this->sample_flow_channel = param_to_int (context, paramtype, param, 3);
      break;
   case read_rmcios:
      if (this == NULL)
         break;
      if (this->sample_flow_channel != 0)
         this->sample_flow =
            read_f (context, this->sample_flow_channel) * 16.6666667;
      return_float (context, paramtype, returnv,
                    read_f (context,
                            this->counter_channel) / read_f (context,
                                                             this->
                                                             timer_channel)
                    / this->sample_flow);
      break;
   case write_rmcios:
      if (this == NULL)
         break;
      if (this->sample_flow_channel != 0)
         this->sample_flow =
            read_f (context, this->sample_flow_channel) * 16.6666667;
      {
         float conc;
         conc =
            write_fv (context, this->counter_channel, 0,
                      NULL) / write_fv (context, this->timer_channel, 0,
                                        NULL) / this->sample_flow;
         write_f (context, linked_channels (context, id), conc);
         return_float (context, paramtype, returnv, conc);
      }
      break;
   }
}

///////////////////////////////////////////////////////////
// Modbus RTU
///////////////////////////////////////////////////////////
struct modbus_rtu_data
{
   int communication_channel;
   char address;
   char function;
   int crc;
   int datalen;
   char *data;
   int linked_channels;
};

void modbus_rtu_class_func (struct conc_data *this,
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
                     "Channel for communicating with Modbus RTU -devices \r\n"
                     "create modbus_rtu newname\r\n"
                     "setup newname communication_channel | "
                     " address | function\r\n"
                     "write newname data | function | address\r\n"
                     "read newname | function | address\r\n"
                     "link newname linked_channel\r\n");
      break;
   }
}

void init_std_device_channels (const struct context_rmcios *context)
{
   // Device channels
   create_channel_str (context, "tsi4000",
                       (class_rmcios) tsi_flow_class_func, NULL);
   create_channel_str (context, "atm_hv", (class_rmcios) atm_hv_class_func,
                       NULL);
   create_channel_str (context, "pt",
                       (class_rmcios) pt_temperature_class_func, NULL);
   create_channel_str (context, "conc", (class_rmcios) conc_class_func, NULL);
}

