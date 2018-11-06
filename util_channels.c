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

/////////////////////////////////////////////////////
// Channel for formatting channel data like printf //
/////////////////////////////////////////////////////
// Linked list of buffered data
struct buffer_queue
{
   char *data;                  // Pointer to buffer that contains data
   int length;                  // Length of data in the buffer
   int size;                    // Size of the buffer
   struct buffer_queue *next;   // Pointer to next item in the list
};

// Recursive printing function. 
// It collects data to list. 
// Copies list data to single buffer. 
// Sends the buffer contents to linked_channels.
// Total bytes contain the size of the payload in the list.
void print_with_list (const struct context_rmcios *context,
                      struct buffer_queue *latest,
                      char *s,
                      enum type_rmcios paramtype,
                      union param_rmcios returnv,
                      int num_params, const union param_rmcios param,
                      int pindex, int total_size, struct buffer_queue *first)
{
   if (num_params > 0 && *s != 0)       
   // print single parameter to the list
   {
      // Store start of non formatted string
      char *p = s;
      char temp;

      struct buffer_queue current;
      current.data = p;
      current.next = NULL;
      latest->next = &current;

      // Look for non-formatted characters
      while (*s != '%')
      {
         if (*s == 0)   // End of string
         {
            current.length = s - p;
            current.size = current.length;
            print_with_list (context, &current, s, paramtype,
                             returnv, num_params, param, pindex,
                             total_size, first);
            return;
         }
         s++;
         total_size++;

      }

      // Add the non formatted data to the string
      current.length = s - p;   // Data without the '%' symbol.
      current.size = current.length;
      if (current.length > 0)
         total_size--;

      p = s;    // Store start of format string
      s++;

      // Collect format specifier
      int collecting = 1;
      int slen;
      while (collecting == 1)
      {
         collecting = 0;
         if (*s == 0)
         {
            print_with_list (context, &current, s, paramtype,
                             returnv, num_params, param, pindex,
                             total_size, first);
         }
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
            int ip = param_to_int (context, paramtype, param, pindex);
            slen = snprintf (NULL, 0, p, ip);   
            // Determine needed buffer size
            {
               // Allocate buffer
               char sbuf[slen + 1];     
               
               // Fill buffer
               snprintf (sbuf, slen + 1, p, ip);  

               struct buffer_queue formatted;
               formatted.next = NULL;
               
               // link last->this
               current.next = &formatted;       
               formatted.data = sbuf;
               formatted.length = strlen (sbuf);
               formatted.size = formatted.length;

               // Add parameter to the list and increase recursion depth.
               total_size += formatted.length;
               // Restore string
               *s = temp;       
               print_with_list (context, &formatted, s, paramtype,
                                returnv, num_params - 1, param,
                                ++pindex, total_size, first);
               return;
            }
            *s = temp;  // Restore string
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
            //
            s++;
            temp = *s;
            *s = 0;     // Mark end of string
            float fp = param_to_float (context, paramtype, param, pindex);
            // Determine needed buffer size
            slen = snprintf (NULL, 0, p, fp);   
            {
               // Allocate buffer
               char sbuf[slen + 1];     
               // Fill buffer
               snprintf (sbuf, slen + 1, p, fp);        

               struct buffer_queue formatted;
               formatted.next = NULL;
               current.next = &formatted;      
               formatted.data = sbuf;
               formatted.length = strlen (sbuf);
               formatted.size = formatted.length;

               total_size += formatted.length;
               // Add parameter to the list and increase recursion depth.
               // Restore string
               *s = temp;       
               print_with_list (context, &formatted, s, paramtype,
                                returnv, num_params - 1, param,
                                ++pindex, total_size, first);
               return;
            }
            *s = temp;  // Restore string
            s--;

            break;
            // String
         case 's':
            // Print formatted string
            s++;
            temp = *s;
            // Mark end of string
            *s = 0;     
            // Determine needed buffer size
            slen = param_string_alloc_size (context, paramtype, param, pindex);
            {
               char sbuf[slen];
               const char *pstr;
               pstr = param_to_string (context, paramtype, param, pindex, 
                                       slen, sbuf);  
               
               struct buffer_queue formatted;
               formatted.next = NULL;
               current.next = &formatted;       
               formatted.data = (char *) pstr;
               formatted.length = strlen (pstr);
               formatted.size = formatted.length + 1;

               total_size += formatted.length;

               // Add parameter to the list and increase recursion depth.
               // Restore string
               *s = temp;       
               print_with_list (context, &formatted, s, paramtype,
                                returnv, num_params - 1, param,
                                ++pindex, total_size, first);
               return;
            }
            *s = temp;  // Restore string
            s--;

            break;
            // Character
         case 'c':
            {
               char sbuf[2];
               // get 1 char string
               param_to_string (context, paramtype, param, pindex, 2, sbuf); 

               struct buffer_queue formatted;
               formatted.next = NULL;
               current.next = &formatted;       
               formatted.data = sbuf;
               formatted.length = 1;
               formatted.size = 2;

               total_size += formatted.length;
               
               // Add parameter to the list and increase recursion depth.
               print_with_list (context, &formatted, ++s,
                                paramtype, returnv, num_params - 1,
                                param, ++pindex, total_size, first);
               return;
            }
            break;
         
         case 'n':
            break;
         case '%':
            {
               struct buffer_queue formatted;
               formatted.next = NULL;
               current.next = &formatted;       
               formatted.data = "%";
               formatted.length = 1;
               formatted.size = formatted.length;
               total_size += formatted.length;

               // Add parameter to the list and increase recursion depth.
               print_with_list (context, &formatted, ++s,
                                paramtype, returnv, num_params,
                                param, pindex, total_size, first);
               return;
            }
         default:
            // Continue collecting
            collecting = 1;     
            break;
         }
         s++;
      }
   }
   else // Build the final string, and send to channel
   {
      char buffer[total_size + strlen (s) + 1];
      int i = 0;
      while (first != NULL)
      {
         // append list data to string buffer
         memcpy (buffer + i, first->data, first->length);       
         i += first->length;
         // Move to next list item
         first = first->next;   
      }
      // Add null terminator
      buffer[i] = 0;
      strcat (buffer, s);

      // Send the string to the channel
      return_string (context, paramtype, returnv, buffer);
   }
   return;      // Finnish the recursive collecting
}

// prints given channel function parameters to an channel. 
// Formats string as in printf using a single write command
void print_param (const struct context_rmcios *context,
                  char *format,
                  enum type_rmcios paramtype,
                  union param_rmcios returnv,
                  int num_params, const union param_rmcios param)
{
   // insert initial list record
   struct buffer_queue initial_queue;   
   initial_queue.next = NULL;
   initial_queue.data = NULL;
   initial_queue.length = 0;
   initial_queue.size = 0;

   print_with_list (context, &initial_queue, format, paramtype, returnv,
                    num_params, param, 0, 0, &initial_queue);
}


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
   create_channel_str (context, "format", (class_rmcios) format_class_func,
                       NULL);
}

