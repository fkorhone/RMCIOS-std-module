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
 */

#include "RMCIOS-functions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

///////////////////////////////////
// Channel for comparing values
///////////////////////////////////

void compare_class_func (void *this,
                        const struct context_rmcios *context, int id,
                        enum function_rmcios function,
                        enum type_rmcios paramtype,
                        union param_rmcios returnv,
                        int num_params, const union param_rmcios param){
   int pstrlen;
   int write=0;
   switch (function)
   {
      case help_rmcios:
         return_string (context, paramtype, returnv,
              "help for compare channel\r\n"
              "create compare newname\r\n"
              "read compare type value1 value2\r\n"
              "write newname type value1 value2\r\n"
              "link newname channel\r\n"
               ) ;
         break ;

      case create_rmcios :
         if(num_params<1) break;
         create_channel_param (context, paramtype, param, 0, 
                               (class_rmcios) compare_class_func, 0); 

         break ;

      case write_rmcios :
         write=1 ;
      case read_rmcios:
         if(num_params<3) break ;
         
         char buf[2] ;
         param_to_string(context,paramtype,param,0,2,buf) ;

         int channel1=param_to_channel(context,paramtype,param,1) ;
         int channel2=param_to_channel(context,paramtype,param,2) ;
         int result ; 
         switch(buf[0])
         {
            case 'i' : 
               // int
               {
                  int value1 ;
                  int value2 ;
                  
                  if(channel1==0) 
                  {
                     value1=param_to_integer(context,paramtype,param,1) ;
                  }
                  else value1= read_i(context, channel1) ;
                  if(channel2==0) 
                  {
                     value2=param_to_integer(context,paramtype,param,2) ;
                  }
                  else value2= read_i(context, channel2) ;
                  result= value1 == value2  ;
               }
               break;

            case 'f' :
               // float
               {
                  float value1 ;
                  float value2 ;
                  if(channel1==0) 
                  {
                     value1=param_to_float(context,paramtype,param,1) ;
                  }
                  else value1= read_f(context, channel1) ;

                  if(channel2==0)
                  {
                     value2=param_to_float(context,paramtype,param,2) ;
                  }
                  else value2= read_f(context, channel2) ;
                  result= value1 == value2 ;
               }
               break;

            case 's' :
               
               if(channel1!=0 && channel2!=0)
               // Compare strings read from 2 channels
               {  
                  int slen1=read_str(context,channel1,NULL,0)+1 ;
                  int slen2=read_str(context,channel2,NULL,0)+1 ;
                  {
                     char s1[slen1] ;
                     char s2[slen2] ;
                     read_str(context, channel1, s1, slen1) ;
                     read_str(context, channel2, s2, slen2) ;
                     if(strcmp(s1,s2)==0 ) result=1  ;
                     else result=0 ;
                  }
               }
               else if(channel1!=0) 
               // Compare another string from channel
               {
                  int slen1=read_str(context,channel1,NULL,0)+1 ;
                  int slen2=param_string_alloc_size(context,paramtype,param,2) ;
                  {   
                     char s1[slen1] ;
                     char buffer2[slen2] ;
                     read_str(context, channel1, s1, slen1) ;
                     const char *s2=param_to_string(context,paramtype,param,2,slen2,buffer2);
                     if(strcmp(s1,s2)==0 ) result=1 ;
                     else result=0 ;
                  }
               }
               else if(channel2!=0)
               // Compare another string from channel
               {
                  int slen1=param_string_alloc_size(context,paramtype,param,1) ;
                  int slen2=read_str(context,channel2,NULL,0)+1 ;
                  {
                     char buffer1[slen1] ;
                     char s2[slen2] ;
                     const char *s1=param_to_string(context,paramtype,param,1,slen1,buffer1);
                     read_str(context, channel2, s2, slen1) ;
                     if(strcmp(s1,s2)==0 ) result=1 ;
                     else result=0 ;
                  }
               } 
               else
               // compare 2 strings 
               {
                  int slen1=param_string_alloc_size(context,paramtype,param,1) ;
                  int slen2=param_string_alloc_size(context,paramtype,param,2) ;
                  {
                     char buffer1[slen1] ;
                     char buffer2[slen2] ;
                     const char *s1,*s2 ;
                     s1=param_to_string(context,paramtype,param,1,slen1,buffer1);
                     s2=param_to_string(context,paramtype,param,2,slen2,buffer2);
                     if(strcmp(s1,s2)==0 ) result=1 ;
                     else result=0 ;
                  }
               }
               break ;
         }
         return_int(context,paramtype,returnv,result) ;

         // Send result to linked on write
         if(write==1) write_i(context, linked_channels(context,id), result) ;
         break;
   }
}

/////////////////////////////////////////////////////
// Channel for formatting channel data like printf //
/////////////////////////////////////////////////////

struct format_data
{
   char *buffer;
   char *format;
   int format_size;
   int buffer_size;
   int *param_channels;
   int num_params;
};

void format_class_func (struct format_data *this,
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
                     "format channel "
                     " - Channel for printing channel read data. \r\n"
                     "   Uses same using format as printf.\r\n"
                     " create format newname\r\n"
                     " setup newname format_string | parameter_channels...\r\n"
                     "  -Set format string \r\n"
                     "  -Set the channels to use to fill format parameters.\r\n"
                     "  -channel set to 0 means write command instead.\r\n"
                     " write newname"
                     "  -Send result string to linked channels\r\n"
                     " read newname\r\n"
                     "  -Read the latest formatted string"
                     " link newname channel \r\n"
                     " IMPLEMENTATION LIMITATIONS: \r\n"
                     "channel is read to a fixed sized buffer (128bytes) \r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      
      // allocate new data
      this = (struct format_data *) 
              allocate_storage (context, sizeof (struct format_data), 0); 

      if (this == NULL)
         break;
      this->format_size = 16;
      this->format = allocate_storage (context, this->format_size, 0);
      this->buffer_size = 32;
      this->buffer = (char *) allocate_storage (context, this->buffer_size, 0);
      this->num_params = 0;
      this->param_channels = NULL;
      if (this->buffer == NULL)
         this->buffer_size = 0;
      else
         this->buffer[0] = 0;

      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) format_class_func, this); 
      break;

   case setup_rmcios:
      if (this == NULL)
      {
         if (num_params < 1)
            break;
         int pslen = param_string_length (context, paramtype, param, 0) + 1;
         {
            struct format_data fmt;
            char buffer[pslen];
            this = &fmt;
            this->buffer_size = pslen;
            this->buffer = buffer;
            param_to_string (context, paramtype, param, 0,
                             this->buffer_size, this->buffer);

            // Parameters specified
            if (num_params > 1) 
            {
               int i;
               int param_channels[num_params - 1];
               this->num_params = num_params - 1;
               this->param_channels = param_channels;
               for (i = 1; i < num_params; i++)
               {
                  this->param_channels[i - 1] =
                     param_to_int (context, paramtype, param, i);
               }
               // return generated format string
               format_class_func (this, context, id, read_rmcios,
                                  paramtype, returnv, num_params, param);
            }
         }
         break;
      }

      if (num_params < 1)
         break;
      {
         // Obtain the format string     
         int pslen = param_string_length (context, paramtype, param, 0) + 1;
         if (pslen > this->format_size) 
         // Needed buffer is bigger than old buffer.
         {
            this->format_size = pslen;
            free_storage (context, this->format, 0);
            this->format =
               (char *) allocate_storage (context, this->format_size, 0);
         }
         param_to_string (context, paramtype, param, 0,
                          this->format_size, this->format);
      }
      if (num_params > 1) 
      // Parameters specified
      {
         int i;
         free_storage (context, this->param_channels, 0);

         // Get parameter channels
         this->num_params = num_params - 1;
         this->param_channels =
            (int *) allocate_storage (context,
                                      this->num_params * sizeof (int), 0);
         for (i = 1; i < num_params; i++)
         {
            this->param_channels[i - 1] =
               param_to_int (context, paramtype, param, i);
         }
      }

      // return generated format string
      format_class_func (this, context, id, read_rmcios, paramtype,
                         returnv, num_params, param);

      break;

   case read_rmcios:
      if (this == NULL)
         break;
      return_string (context, paramtype, returnv, this->buffer);
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      {
         char *s = this->format;
         // Index of output writing
         int index = 0;  
         // Index of currently processed parameter
         int call_param_index = 0;      
         // Index of channel processed parameter
         int channel_param_index = 0;  
         char temp;
         int specifier = 0;
         int slen;
         int param_channel = 0;
         // pointer to start of format string.
         char *p;               

         while (*s != 0)
         {
            if (specifier == 0)
            {
               if (*s == '%')
               {
                  if (channel_param_index < this->num_params)
                     param_channel =
                        this->param_channels[channel_param_index++];

                  specifier = 1;
                  p = s;
               }
               else
               {
                  // Double buffer size untill parameter fits in.
                  while ((this->buffer + index + 2) >=
                         (this->buffer + this->buffer_size))
                  {
                     this->buffer =
                        realloc (this->buffer, this->buffer_size << 1);
                     if (this->buffer == NULL)
                        // error out of memory etc...
                        return; 
                     this->buffer_size = this->buffer_size << 1;
                  }

                  this->buffer[index++] = *s;
                  this->buffer[index] = 0;
               }
            }
            else if (specifier == 1)
            {
               switch (*s) 
               // Check for format end characters
               {
                  // int
               case 'd':
               case 'i':
               case 'o':
               case 'x':
               case 'X':
                  // Print the formatted integer :
                  s++;
                  temp = *s;
                  // Mark end of string
                  *s = 0;       

                  // Fetch the parameter
                  int ip;
                  if (param_channel == 0 && call_param_index < num_params)
                     ip = param_to_integer (context, paramtype,
                                            param, call_param_index++);
                  else
                     ip = read_i (context, param_channel);
                  // Determine needed buffer size
                  slen = snprintf (NULL, 0, p, ip);

                  // Double buffer size untill parameter fits in.
                  while ((this->buffer + index + slen + 1) >=
                         (this->buffer + this->buffer_size))
                  {
                     this->buffer =
                        realloc (this->buffer, this->buffer_size << 1);
                     if (this->buffer == NULL)
                        return; // error out of memory etc...
                     this->buffer_size = this->buffer_size << 1;
                  }

                  // Fill the parameter
                  slen = snprintf (this->buffer + index, 
                                   this->buffer_size - index, p, ip); 
                  index += slen;

                  // Specifier has been processed:
                  specifier = 0;

                  // Restore string
                  *s = temp;
                  s--;
                  break;

                  // float
               case 'f':
               case 'F':
               case 'e':
               case 'E':
               case 'g':
               case 'G':
               case 'a':
               case 'A':
                  // Print the formatted float :
                  s++;
                  temp = *s;
                  *s = 0;       // Mark end of string

                  // Fetch the parameter
                  float fp = 0;
                  if (param_channel == 0 && call_param_index < num_params)
                     fp = param_to_float (context, paramtype,
                                          param, call_param_index++);
                  else
                     fp = read_f (context, param_channel);

                  // Determine needed buffer size 
                  slen = snprintf (NULL, 0, p, fp);

                  // Double buffer size untill parameter fits in.
                  while ((this->buffer + index + slen + 1) >=
                         (this->buffer + this->buffer_size))
                  {
                     this->buffer =
                        realloc (this->buffer, this->buffer_size << 1);
                     if (this->buffer == NULL)
                        return; // error out of memory etc...
                     this->buffer_size = this->buffer_size << 1;
                  }
                  // Fill the parameter and increment write position
                  index += snprintf (this->buffer + index, 
                                     this->buffer_size - index, p, fp);
                  
                  // Specifier has been processed:
                  specifier = 0;

                  // Restore string
                  *s = temp;
                  s--;
                  break;
                  // String
               case 's':
                  // Print formatted string
                  s++;
                  temp = *s;
                  *s = 0;       // Mark end of string

                  // Determine needed buffer size
                  if (param_channel == 0 && call_param_index < num_params)
                     slen =
                        param_string_length (context,
                                             paramtype, param,
                                             call_param_index);
                  else
                  {
                     struct buffer_rmcios retv;
                     retv.data = NULL;
                     retv.length = 0;
                     retv.size = 0;
                     retv.required_size = 0;
                     context->run_channel (context,
                                           param_channel,
                                           read_rmcios,
                                           buffer_rmcios,
                                           (union
                                            param_rmcios)
                                           &retv, 0,
                                           (const union param_rmcios) 0);
                     slen = retv.required_size;
                  }

                  // Double buffer size until parameter fits in.
                  while ((this->buffer + index + slen + 1) >=
                         (this->buffer + this->buffer_size))
                  {
                     this->buffer =
                        realloc (this->buffer, this->buffer_size << 1);
                     if (this->buffer == NULL)
                        return; // error out of memory etc...
                     this->buffer_size = this->buffer_size << 1;
                  }

                  // Fetch parameter
                  if (param_channel == 0 && call_param_index < num_params)
                  {
                     param_to_string (context, paramtype,
                                      param,
                                      call_param_index++,
                                      this->buffer_size -
                                      index, this->buffer + index);
                  }
                  else
                  {
                     read_str (context, param_channel,
                               this->buffer + index, this->buffer_size - index);
                  }
                  // Increment write position
                  index += strlen (this->buffer + index);

                  // Specifier has been processed:
                  specifier = 0;

                  // Restore string
                  *s = temp;
                  s--;
                  break;
                  // Character
               case 'c':
                  {
                     // Double buffer size until parameter fits in.
                     while ((this->buffer + index + 3) >=
                            (this->buffer + this->buffer_size))
                     {
                        this->buffer =
                           realloc (this->buffer, this->buffer_size << 1);
                        if (this->buffer == NULL)
                           return;      // error out of memory etc...
                        this->buffer_size = this->buffer_size << 1;
                     }

                     // Fetch parameter
                     if (param_channel == 0 && call_param_index < num_params)
                     {
                        param_to_string (context, paramtype,
                                         param,
                                         call_param_index++,
                                         2, this->buffer + index);
                     }
                     else
                     {
                        read_str (context, param_channel,
                                  this->buffer + index, 2);
                     }

                     // increment write position
                     index++;

                     // Specifier has been processed:
                     specifier = 0;
                  }
                  break;
                  // Nothing printed
               case 'n':
                  specifier = 0;
                  break;

               case '%':       // % as it is
                  {
                     // Double buffer size until parameter fits in.
                     while ((this->buffer + index + 3) >=
                            (this->buffer + this->buffer_size))
                     {
                        this->buffer =
                           realloc (this->buffer, this->buffer_size << 1);
                        if (this->buffer == NULL)
                           return;      // error out of memory etc...
                        this->buffer_size = this->buffer_size << 1;
                     }

                     this->buffer[index + 1] = 0;
                     this->buffer[index] = '%';
                     index++;
                     specifier = 0;
                  }

               default:
                  break;
               }

            }
            s++;
         }
         // Write the result to linked
         write_str (context, linked_channels (context, id), this->buffer, 0);
         break;
      }
   }
}

void init_util_channels (const struct context_rmcios *context)
{
   // Utility channels
   create_channel_str (context, "compare", 
                       (class_rmcios) compare_class_func, NULL);
   create_channel_str (context, "format", 
                       (class_rmcios) format_class_func, NULL);
}

