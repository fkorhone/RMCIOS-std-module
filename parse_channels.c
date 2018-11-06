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
 * Changelog: (date,who,description)
 */

#include "RMCIOS-functions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

////////////////////////////////////////////
// Buffer Channel
//////////////////////////////////////////////
struct buffer_data
{
   int linked_channels;
   char *buffer;
   int buffer_size;
   int length;

   // Pattern for reception
   char *pattern;
   int pattern_length;
   char *search_buffer;
   int search_length;           // search buffer data length

   // Register safe multithreading:
   int share_register;
};

void buffer_class_func (struct buffer_data *this,
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
                     "Buffer channel \r\n"
                     "  - Channel for storing and collecting data. \r\n"
                     " create buffer newname \r\n"
                     " setup newname size | flush_pattern\r\n"
                     "  -Set buffer size\r\n"
                     "  -Optonal flush pattern triggers buffer flushing\r\n"
                     "   Buffers are flushed to linked channels\r\n"
                     "  -Setting only size removes existing flush pattern.\r\n"
                     " write newname data\r\n"
                     "  -Append data to buffer\r\n"
                     "  -Buffer will flush automatically to linked channels."
                     "   when it becomes full or flush pattern is found.\r\n"
                     "  -Buffer data is not returned.\r\n"
                     " write newname \r\n"
                     "  -Flush buffer:\r\n"
                     "  1.Send data to linked channels.\r\n"
                     "  2.Return buffer data\r\n"
                     "  3.Clear buffer contents.\r\n"
                     "  4.Reset pattern search\r\n"
                     " read newname\r\n"
                     "  -Return buffer data\r\n" " link newname channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;

      //new struct conc_data ; // allocate new data
      this = (struct buffer_data *) 
              allocate_storage (context, sizeof (struct buffer_data), 0); 
      if (this == NULL)
         break;

      this->buffer_size = 16;
      this->buffer = malloc (16);
      this->length = 0;
      this->share_register = 0;

      // Pattern for reception
      this->pattern = NULL;
      this->pattern_length = 0;
      this->search_length = 0;
      this->search_buffer = NULL;
      
      // create the channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) buffer_class_func, this); 
      break;

   case setup_rmcios:
      if (this == NULL)
         break;

      // 1. parameter :
      if (num_params < 1)
         break;


      // Lock the channel:
      while (request_write_resource (&this->share_register) == 0);

      int size;
      size = param_to_int (context, paramtype, param, 0);
      this->buffer_size = size;
      this->length = 0;
      if (this->buffer == NULL)
         free (this->buffer);
      this->buffer = (char *) malloc (this->buffer_size);
      if (this->buffer == NULL)
      {
         printf ("Buffer channel: Could not allocate memory for buffer.\r\n");
         this->buffer_size = 0;
      }

      this->pattern_length = 0;
      if (this->search_buffer != NULL)
      {
         free (this->search_buffer);
         this->search_buffer = NULL;
      }
      if (this->pattern != NULL)
      {
         free (this->pattern);
         this->pattern = NULL;
      }

      // 2. parameter.
      if (num_params >= 2)
      {
         this->pattern_length =
            param_buffer_length (context, paramtype, param, 1);
         this->pattern = malloc (this->pattern_length);
         this->search_buffer = malloc (this->pattern_length);
         this->search_length = 0;

         if (this->pattern == NULL || this->search_buffer == NULL)
         {
            this->pattern_length = 0;
            printf
               ("Buffer channel: Could not allocate memory for pattern.\r\n");
            break;
         }

         param_to_buffer (context, paramtype, param, 1,
                          this->pattern_length, this->pattern);
      }

      // Unlock the channel
      stop_write_resource (&this->share_register);

      break;
   case write_rmcios:
      if (this == NULL)
         break;
      // Lock the channel:
      while (request_write_resource (&this->share_register) == 0);

      if (num_params < 1)       // Flush data
      {
         write_buffer (context, linked_channels (context, id),
                       this->buffer, this->length, 0);

         // Return buffer data:
         return_buffer (context, paramtype, returnv, this->buffer,
                        this->length);

         // Clear buffer contents
         this->length = 0;

         // Reset pattern search:
         this->search_length = 0;

      }
      else      
      // Append data
      {
         // Determine possibly needed buffer size:
         int plen = param_buffer_alloc_size (context, paramtype, param, 0);
         {
            int i;
            char pbuf[plen];
            struct buffer_rmcios buf;
            buf = param_to_buffer (context, paramtype, param, 0, plen, pbuf);

            //Append data byte by byte
            for (i = 0; i < buf.length; i++)
            {
               int j;

               // Append byte to buffer:
               this->buffer[this->length++] = buf.data[i];

               // Shift to make space for new character:
               if (this->search_buffer != NULL)
               {
                  for (j = 0; j < (this->pattern_length - 1); j++)
                     this->search_buffer[j] = this->search_buffer[j + 1];
                  // insert new character
                  this->search_buffer[j] = buf.data[i]; 
                  if (this->search_length < this->pattern_length)
                     // count received characters
                     this->search_length++;     
               }
               // Test for flush conditions:
               if (this->length >= this->buffer_size 
                   || (this->pattern != NULL 
                       && (this->search_length == this->pattern_length 
                       && memcmp (this->pattern, this->search_buffer, 
                                  this->pattern_length)== 0)
                      ) // pattern found?
                  )
               { // Flush data:

                  // Send buffer data to linked channels:
                  write_buffer (context,
                                linked_channels (context, id),
                                this->buffer, this->length, 0);

                  // Clear buffer
                  this->length = 0;     
                  // clear pattern search
                  this->search_length = 0;     
               }
            }
         }
      }

      // Unlock the channel
      stop_write_resource (&this->share_register);

      break;
   case read_rmcios:
      if (this == NULL)
         break;
      return_buffer (context, paramtype, returnv, this->buffer, this->length);
      break;
   }
}

//////////////////////////////////////////////////////////////////////////
//! Channel for splitting character delimited data to separate channels //
//////////////////////////////////////////////////////////////////////////
struct splitter_data
{
   char delimiter_char;
   char reset_char;
   int *outputs;
   int num_outputs;
   int slice;
};

void splitter_class_func (struct splitter_data *this,
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
                     "Splitter - Channel for splitting \r\n"
                     " create splitter newname\r\n"
                     " setup newname delimiter_char reset_char "
                     "               split_output_channels...\r\n"
                     " write newname data\r\n"
                     " write newname \r\n"
                     "   #- makes empty write to split output channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      
      // allocate new data
      this = (struct splitter_data *) 
             allocate_storage (context, sizeof (struct splitter_data), 0);

      //default values :
      this->delimiter_char = ' ';
      this->reset_char = '\n';
      this->outputs = NULL;
      this->num_outputs = 0;
      this->slice = 0;

      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) splitter_class_func, this);    
      break;

   case setup_rmcios:
      if (this == NULL)
         break;
      if (num_params < 2)
         break;
      param_to_buffer (context, paramtype, param, 0, 1, &this->delimiter_char);
      param_to_buffer (context, paramtype, param, 1, 1, &this->reset_char);
      int i;
      this->num_outputs = 0;
      if (this->outputs == NULL)
         free (this->outputs);
      this->outputs = malloc (sizeof (int) * num_params - 2);
      this->num_outputs = num_params - 2;
      for (i = 2; i < num_params; i++)
      {
         this->outputs[i - 2] = param_to_int (context, paramtype, param, i);
      }
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
      {
         context->run_channel (context,
                               this->outputs[this->slice],
                               write_rmcios,
                               paramtype,
                               (const union param_rmcios) 0,
                               0, (const union param_rmcios) 0);
         this->slice = 0;
      }
      else
      {
         int plen = param_buffer_alloc_size (context, paramtype, param, 0);
         {
            int slice_start = 0;
            char buffer[plen];
            struct buffer_rmcios b;
            int i;
            b = param_to_buffer (context, paramtype, param, 0, plen, buffer);
            for (i = 0; i < b.length; i++)
            {
               if (b.data[i] == this->delimiter_char)
               {
                  write_buffer (context,
                                this->outputs[this->slice],
                                b.data + slice_start, i - slice_start, 0);

                  context->run_channel (context,
                                        this->outputs[this->
                                                      slice],
                                        write_rmcios, paramtype,
                                        (union param_rmcios) 0,
                                        0, (const union param_rmcios) 0);
                  slice_start = i + 1;
                  this->slice++;
               }
               else if (b.data[i] == this->reset_char)
               {
                  write_buffer (context,
                                this->outputs[this->slice],
                                b.data + slice_start, i - slice_start, 0);

                  context->run_channel (context,
                                        this->outputs[this->
                                                      slice],
                                        write_rmcios, paramtype,
                                        (union param_rmcios) 0,
                                        0, (const union param_rmcios) 0);
                  slice_start = i + 1;
                  this->slice = 0;
               }
            }
            if (i - slice_start >= 0)
            {
               write_buffer (context, this->outputs[this->slice],
                             b.data + slice_start, i - slice_start, 0);
            }

         }
      }
      break;
   }
}


struct pattern_data
{
   int linked_channel;

   // Start pattern for reception
   char *start_pattern;
   int start_length;
   char *start_buffer;

   // Reception stop pattern
   char *stop_pattern;
   int stop_length;
   char *stop_buffer;

   // Direct readout of latest reception
   char *readout;
   char *readout_read;
   int readout_size;
   int readout_bytes;
   int readout_read_bytes;

   int enabled;
   int share_register;
};

void pattern_class_func (struct pattern_data *this,
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
                     "pattern scanning channel "
                     " -Channel that records data between "
                     "  start_pattern and end_pattern.\r\n"
                     "  -Records data to buffer."
                     "  -Sends buffer contents to linked channels. "
                     "  -patterns are excluded from data\r\n"
                     "create pattern newname\r\n"
                     "setup newname start_pattern stop_patern |buffer_size\r\n"
                     "write newname data \r\n"
                     "read newname \r\n"
                     "  # -Return the latest complete data found. "
                     "   # -Data is between start_pattern and end_pattern\r\n"
                     "link newname output_channel\r\n");

   case create_rmcios:
      if (num_params < 1)
         break;
      
      // allocate new data
      this = (struct pattern_data *) 
             allocate_storage (context, sizeof (struct pattern_data), 0);

      if (this == NULL)
         break;

      this->start_pattern = NULL;
      this->start_length = 0;
      this->start_buffer = NULL;

      this->stop_pattern = NULL;
      this->stop_length = 0;
      this->stop_buffer = NULL;

      this->readout = malloc (20);
      this->readout_size = 20;
      this->readout_bytes = 0;

      this->readout_read_bytes = 0;
      this->readout_read = malloc (20);

      this->enabled = 0;
      this->share_register = 0;

      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) pattern_class_func, this);
      break;

   case setup_rmcios:
      if (this == NULL)
         break;
      if (num_params < 2)
         break;

      this->start_length = param_buffer_length (context, paramtype, param, 0);

      // Allocate space for start pattern and buffers
      if (this->start_pattern != NULL)
         free (this->start_pattern);
      if (this->start_buffer != NULL)
         free (this->start_buffer);
      this->start_pattern = (char *) malloc (this->start_length);
      this->start_buffer = (char *) malloc (this->start_length);
      param_to_buffer (context, paramtype, param, 0, this->start_length,
                       this->start_pattern);

      // Allocate space for end pattern and buffers
      this->stop_length = param_buffer_length (context, paramtype, param, 1);
      if (this->stop_pattern != NULL)
         free (this->stop_pattern);
      if (this->stop_buffer != NULL)
         free (this->stop_buffer);
      this->stop_pattern = (char *) malloc (this->stop_length);
      this->stop_buffer = (char *) malloc (this->stop_length);
      param_to_buffer (context, paramtype, param, 1, this->stop_length,
                       this->stop_pattern);

      if (num_params < 3)
         break;
      // Allocate space for readout buffer
      if (this->readout != NULL)
         free (this->readout);
      if (this->readout_read != NULL)
         free (this->readout_read);
      this->readout_size = param_to_int (context, paramtype, param, 2);
      this->readout = (char *) malloc (this->readout_size);
      this->readout_read = (char *) malloc (this->readout_size);
      this->readout_bytes = 0;
      this->readout_read_bytes = 0;
      this->share_register = 0;
      break;

   case read_rmcios:
      if (this == NULL)
         break;
      while (request_read_resource (&this->share_register) == 0);
      return_buffer (context, paramtype, returnv, this->readout_read,
                     this->readout_read_bytes);
      stop_read_resource (&this->share_register);
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      if (this->start_length < 1)
         break;
      if (this->stop_length < 1)
         break;
      if (num_params < 1)
         break;

      // Get possibly needed buffer size
      int bufflen = param_buffer_alloc_size (context, paramtype, param, 0);     
      {
         // Allocate buffer (if needed)
         char buffer[bufflen];  
         struct buffer_rmcios b;
         int i;
         int start = 0;
         int bytes;
         // Get handle to data.
         b = param_to_buffer (context, paramtype, param, 0, bufflen, buffer);   
         for (i = 0; i < b.length; i++) 
         // Go through buffer byte by byte:
         {
            int j;
            // Shift to make space for new character:
            for (j = 0; j < (this->start_length - 1); j++)
               this->start_buffer[j] = this->start_buffer[j + 1];
            // insert new character
            this->start_buffer[j] = b.data[i];  


            // search for START pattern
            if (memcmp
                (this->start_pattern, this->start_buffer,
                 this->start_length) == 0)
            {
               // FOUND START PATTERN!
               this->enabled = 1;
               start = i + 1;
               this->readout_bytes = 0;
            }

            // Shift to make space for new character:
            for (j = 0; j < (this->stop_length - 1); j++)
               this->stop_buffer[j] = this->stop_buffer[j + 1];
            this->stop_buffer[j] = b.data[i];   
            // insert new character

            // search for STOP pattern
            if (this->enabled == 1
                && memcmp (this->stop_pattern, this->stop_buffer,
                           this->stop_length) == 0)
            {
               // FOUND STOP PATTERN!
               bytes = i + 1;
               write_buffer (context,
                             linked_channels (context, id),
                             b.data + start, bytes, 0);
               if (bytes > (this->readout_size - this->readout_bytes))
               {
                  bytes = this->readout_size - this->readout_bytes;
               }
               if (bytes < 0)
                  bytes = 0;
               
               // Copy to readout buffer
               memcpy (this->readout + this->readout_bytes, 
                       b.data + start, bytes);     
               this->readout_bytes += bytes;
               this->enabled = 0;


               // Copy data to the readable data buffer:
               while (request_write_resource (&this->share_register) == 0);
               memcpy (this->readout_read, this->readout,
                       this->readout_bytes - this->stop_length);
               this->readout_read_bytes =
                  this->readout_bytes - this->stop_length;
               stop_write_resource (&this->share_register);

               // Send new data to linked channels:
               while (request_read_resource (&this->share_register) == 0);
               write_buffer (context,
                             linked_channels (context, id),
                             this->readout_read, this->readout_read_bytes, 0);
               stop_read_resource (&this->share_register);
            }

         }
         if (this->enabled == 1)
         {
            bytes = b.length - start;
            if (bytes > (this->readout_size - this->readout_bytes))
            {
               bytes = this->readout_size - this->readout_bytes;
            }
            if (bytes < 0)
               bytes = 0;
            // Copy to readout buffer
            memcpy (this->readout + this->readout_bytes, 
                    b.data + start, bytes); 
            this->readout_bytes += bytes;
         }
      }
      break;
   }
}

void init_parse_channels (const struct context_rmcios *context)
{
   // Parsing channels
   create_channel_str (context, "buffer", (class_rmcios) buffer_class_func,
                       NULL);
   create_channel_str (context, "pattern", (class_rmcios) pattern_class_func,
                       NULL);
   create_channel_str (context, "splitter",
                       (class_rmcios) splitter_class_func, NULL);

}

