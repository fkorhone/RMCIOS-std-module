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
 * Changelog: (date,who,description)
 */
#include "RMCIOS-functions.h"
#include <stdlib.h>
#include <math.h>
#include "pid.h"

/////////////////////////////////////////////////////////////
//! Ramp channel
//////////////////////////////////////////////////////////////
struct ramp_data
{
   int size;
   float *values;
   int index;
};

void ramp_class_func (struct ramp_data *this,
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
                     "Ramp channel - channel that outputs a ramp.\r\n"
                     "create ramp newname\r\n"
                     "setup newname continuous \r\n"
                     "setup newname trigged \r\n"
                     "setup newname | type start/single/values... |"
                     " end | size \r\n"
                     "  -type=single \r\n"
                     "  -type=lin \r\n"
                     "  -type=log : values are spaced logarithmically "
                     " between start and end.\r\n"
                     "  -type=list : output from selected numeric list\r\n"
                     "  -start/single: Start value or the single value"
                     "(type=single). \r\n"
                     "  -end: Last value in the ramp\r\n"
                     "  -size: number of values in the ramp\r\n"
                     "  -values...: list of numbers that define the ramp"
                     " output(type=list)\r\n"
                     "setup newname\r\n"
                     "  -restart ramp\r\n"
                     "write newname\r\n"
                     "  run one step of the ramp\r\n"
                     "link newname channel\r\n"
                     "  link ramp output to a channel\r\n"
                     "link newname channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      else
         this = (struct ramp_data *) 
                 allocate_storage (context, sizeof (struct ramp_data), 0);
      if (this == NULL)
      {
         info (context, context->errors,
               "Unable to allocate memory for ramp data!");
         break;
      }
      this->size = 0;
      this->values = NULL;
      this->index = 0;

      // create the channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) ramp_class_func, this); 
      break;

   case setup_rmcios:
      if (this == NULL)
         break;
      else
      {
         char type[8];
         if (num_params < 1)
            this->index = 0;
         if (num_params < 2)
            break;

         param_to_string (context, paramtype, param, 0, sizeof (type), type);

         if (this->values != NULL)
         {
            free (this->values);
            this->values = NULL;
         }

         if (strcmp (type, "single") == 0)      
          // Fill the single value to array
         {
            this->size = 1;
            this->values = malloc (sizeof (float));
            this->values[0] = param_to_float (context, paramtype, param, 1);
            // Ramp is not initially running
            this->index = this->size;   
         }

         if (strcmp (type, "lin") == 0) 
         // Calculate linear ramp to array
         {
            if (num_params < 4)
               break;
            int i;
            float start = param_to_float (context, paramtype, param, 1);
            float end = param_to_float (context, paramtype, param, 2);
            this->size = param_to_integer (context, paramtype, param, 3);
            this->values = malloc (this->size * sizeof (float));
            for (i = 0; i < this->size; i++)
            {
               this->values[i] = (end - start) / (this->size - 1) * i + start;
            }
            // Ramp is not initially running
            this->index = this->size;   
         }

         if (strcmp (type, "log") == 0) 
         // Calculate log ramp to array
         {
            if (num_params < 4)
               break;
            int i;
            float start = log (param_to_float (context, paramtype, param, 1));
            float end = log (param_to_float (context, paramtype, param, 2));
            this->size = param_to_integer (context, paramtype, param, 3);
            this->values = malloc (this->size * sizeof (float));
            for (i = 0; i < this->size; i++)
            {
               this->values[i] =
                  exp ((end - start) / (this->size - 1) * i + start);
            }
            this->index = this->size;   // Ramp is not initially running
         }

         if (strcmp (type, "list") == 0)        // Fill given values to array
         {
            int i;
            this->size = num_params - 1;
            this->values = malloc (this->size * sizeof (float));
            for (i = 0; i < this->size; i++)
            {
               this->values[i] =
                  param_to_float (context, paramtype, param, i + 1);
            }
            this->index = this->size;   // Ramp is not initially running
         }
      }
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      if (this->index < this->size)
      {
         // Write next step of the ramp
         write_f (context, linked_channels (context, id),
                  this->values[this->index++]);
         // Empty write signaled after a scan ramp.
         if (this->index == this->size)
            write_fv (context, linked_channels (context, id), 0, NULL);
      }
      break;
   }
}

/////////////////////////////////////////////////////////////////////////
//! Channel for creating pwm signal from external timer timings        //
/////////////////////////////////////////////////////////////////////////
struct timerpwm_data
{
   int self;
   int timer_channel;
   char state;
   float period;                // Hz
   float on_time;               // s
   float off_time;              // s
   float duty;
};

void timerpwm_class_func (struct timerpwm_data *this,
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
                     "help for timerpwm channel\r\n"
                     " create timerpwm newname\r\n"
                     " setup newname frequency | timer_channel\r\n"
                     " write newname # trigger pwm state change"
                     " (called from the timer)\r\n"
                     " write newname duty_cycle #(0..1)\r\n"
                     " link newname channel #link pwm output to channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct timerpwm_data *) 
              allocate_storage (context, sizeof (struct timerpwm_data), 0);     

      // Default values:
      this->timer_channel = 0;
      this->state = 0;
      this->period = 1;
      this->on_time = 0.5;
      this->off_time = 0.5;

      // create channel
      this->self = create_channel_param (context, paramtype, param, 0, 
                                         (class_rmcios) timerpwm_class_func, 
                                         this);       
      break;

   case setup_rmcios:
      if (num_params < 1)
         break;
      this->period = 1.0 / param_to_float (context, paramtype, param, 0);
      this->on_time = this->period * this->duty;
      this->off_time = this->period * (1 - this->duty);

      if (num_params < 2)
         break;
      this->timer_channel = param_to_int (context, paramtype, param, 1);
      link_channel (context, this->timer_channel, this->self);
      this->state = 0;
      write_f (context, this->timer_channel, this->off_time);
      break;
   case write_rmcios:
      if (num_params < 1)      
      // state change
      {
         if (this->timer_channel == 0)
            break;

         if (this->state == 0)
         {
            write_i (context, linked_channels (context, id), 1);
            write_f (context, this->timer_channel, this->on_time);
            this->state = 1;
         }
         else
         {
            write_i (context, linked_channels (context, id), 0);
            write_f (context, this->timer_channel, this->off_time);
            this->state = 0;
         }

      }
      else
      {
         this->duty = param_to_float (context, paramtype, param, 0);
         this->on_time = this->period * this->duty;
         this->off_time = this->period * (1 - this->duty);
      }
      break;
   }
}

/////////////////////////////////////////////////
// PID Controller Channel
/////////////////////////////////////////////////
void pid_setp_subchan_func (struct pid_type *this,
                            const struct context_rmcios *context, int id,
                            enum function_rmcios function,
                            enum type_rmcios paramtype,
                            union param_rmcios returnv,
                            int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case read_rmcios:
      return_float (context, paramtype, returnv, this->setpoint);
      break;
   case write_rmcios:
      if (num_params < 1)
         break;
      this->setpoint = param_to_float (context, paramtype, param, 0);
      break;
   default:
      break;
   }
}

void pid_class_func (struct pid_type *this,
                     const struct context_rmcios *context, int id,
                     enum function_rmcios function,
                     enum type_rmcios paramtype,
                     union param_rmcios returnv,
                     int num_params, const union param_rmcios param)
{
   float control_value;
   float time;
   int namelen;
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "pid controller channel help\r\n"
                     "pid controller require clock channel to know time"
                     " between update events\r\n"
                     " create pid newname \r\n"
                     " setup newname setpoint clk_channel Kp Ki Kd"
                     " inp_min input_max output_min output_max |\r\n"
                     " write newname inputvalue\r\n"
                     " read newname # read latest contol value\r\n"
                     " write newname_setpoint value # setpoint new setpoint\r\n"
                     " link newname channel # link control to channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      else
      {
         int id;
         // allocate new data
         this = (struct pid_type *) 
                allocate_storage (context, sizeof (struct pid_type), 0);

         // default values :
         this->Kp = 1.0;
         this->Ki = 0;
         this->Kd = 0;
         this->input_min = 0;
         this->input_max = 1;
         this->output_min = 0;
         this->output_max = 1;
         this->clock_channel = 0;

         this->previous_error = 0;
         this->integral = 0;
         this->output = 0;
         this->safe_output = 0;

         // create channel
         id = create_channel_param (context, paramtype, param, 0, 
                                    (class_rmcios) pid_class_func, this); 
         
         // create subchannel for setpoint 
         create_subchannel_str (context, id, "_setpoint", 
               (class_rmcios) pid_setp_subchan_func, this);  
      }
      break;

   case setup_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      this->setpoint = param_to_float (context, paramtype, param, 0);
      if (num_params < 9)
         break;
      this->clock_channel = param_to_int (context, paramtype, param, 1);
      this->Kp = param_to_float (context, paramtype, param, 2);
      this->Ki = param_to_float (context, paramtype, param, 3);
      this->Kd = param_to_float (context, paramtype, param, 4);
      this->input_min = param_to_float (context, paramtype, param, 5);
      this->input_max = param_to_float (context, paramtype, param, 6);
      this->output_min = param_to_float (context, paramtype, param, 7);
      this->output_max = param_to_float (context, paramtype, param, 8);
      if (num_params < 10)
         break;
      this->safe_output = param_to_float (context, paramtype, param, 9);
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;

      // reset clock for elapsed time (empty write)
      time = write_fv (context, this->clock_channel, 0, NULL);  
      control_value =
         pid_control (this,
                      param_to_float (context, paramtype, param, 0), time);
      write_f (context, linked_channels (context, id), control_value);
      break;

   case read_rmcios:
      if (this == NULL)
         break;
      return_float (context, paramtype, returnv, this->output);
      break;
   default:
      break;
   }
}

/////////////////////////////////////////////////////////
//! wait time reserved bus
/////////////////////////////////////////////////////////
struct delayed_bus_data
{
   float reserve_time;
   int wait_channel;
   int share_register;
};

// timeslot sharing of linked channels
void delayed_bus_class_func (struct delayed_bus_data *this,
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
                     "Bus that passes write calls to linked channels"
                     " and waits for a fixed time.\r\n"
                     "Only single entry to the bus can be done at a time.\r\n"
                     " create bus newname\r\n"
                     " setup newname reserve_time | wait_channel(wait)\r\n"
                     " write newname data\r\n"
                     " link newname channel"
                     " #link bus to data transport channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      else
         this = (struct delayed_bus_data *) 
                 allocate_storage (context, sizeof (struct delayed_bus_data), 0);
      if (this == NULL)
         info (context, context->errors, "Could not create delayed bus!\r\n");

      this->reserve_time = 1;
      this->wait_channel = channel_enum (context, "wait");
      this->share_register = 0;
      
      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) delayed_bus_class_func, this); 
      break;

   case setup_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      this->reserve_time = param_to_float (context, paramtype, param, 0);
      if (num_params < 2)
         break;
      this->wait_channel = param_to_int (context, paramtype, param, 1);
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      
      // Pass write command to linked channels
      context->run_channel (context, linked_channels (context, id),
                            write_rmcios, paramtype, returnv, num_params,
                            param);
      
      // Wait for specified time
      write_f (context, this->wait_channel, this->reserve_time);
      break;
   }
}
void init_control_channels (const struct context_rmcios *context)
{
   // Control channels

   create_channel_str (context, "ramp", (class_rmcios) ramp_class_func, NULL);
   create_channel_str (context, "timerpwm",
                       (class_rmcios) timerpwm_class_func, NULL);
   create_channel_str (context, "pid", (class_rmcios) pid_class_func, NULL);
   create_channel_str (context, "bus", (class_rmcios) delayed_bus_class_func,
                       NULL);
}
