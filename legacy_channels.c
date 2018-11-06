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
#include <string.h>
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////
// Commander for commandig linked channel with predefined string patterns //
////////////////////////////////////////////////////////////////////////////
struct commander_data
{
   int commanded_channel;
   char write_command[40];
   char read_command[40];
};

void commander_class_func (struct commander_data *this,
                           const struct context_rmcios *context, int id,
                           enum function_rmcios function,
                           enum type_rmcios paramtype,
                           union param_rmcios returnv, int num_params,
                           const union param_rmcios param)
{
   int slen;
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "Channel for interfacing with instruments command"
                     " interface\r\n"
                     " create commander newname\r\n"
                     " setup newname command_channel | write_command |"
                     " read_command \r\n"
                     " read newname #Write read_command to commanded_channel."
                     " Read commanded channel data.\r\n"
                     " write newname #Send commanded channel read data to "
                     " linked channel.\r\n "
                     "	#Write write_command to commanded_channel. \r\n"
                     " link newname channel #Set linked channel.\r\n");
      break;
   case create_rmcios:
      if (num_params < 1)
         break;

      // allocate new data
      this = (struct commander_data *) 
             allocate_storage (context, sizeof (struct commander_data), 0); 
      if (this == NULL)
         printf ("Could not allocate memory for a commander!\n");
      
      //default values :
      this->write_command[0] = 0;
      this->read_command[0] = 0;
      this->commanded_channel = 0;
      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) commander_class_func, this);   
      break;

   case setup_rmcios:
      // 0=commanded_channel | 1=write_command | 2=read_command 
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      this->commanded_channel = param_to_int (context, paramtype, param, 0);
      if (num_params < 2)
         break;
      param_to_string (context, paramtype, param, 1,
                       sizeof (this->write_command), this->write_command);
      if (num_params < 3)
         break;
      param_to_string (context, paramtype, param, 2,
                       sizeof (this->read_command), this->read_command);
      break;

   case read_rmcios:
      if (this == NULL)
         break;
      if (this->read_command[0] != 0)
      {
         write_str (context, this->commanded_channel, this->read_command, 0);
      }
      context->run_channel (context, this->commanded_channel, function,
                            paramtype, returnv, 0, param);

      break;
   case write_rmcios:
      if (this == NULL)
         break;
      // Read data to linked channel
      // Send commanded channel read data to linked channel:
      context->run_channel (context, this->commanded_channel,
                            read_rmcios, channel_rmcios,
                            (union param_rmcios) linked_channels (context,
                                                                  id), 0,
                            (const union param_rmcios) 0);

      // Write the parameter printed write_command to commanded channel:
      if (this->write_command[0] != 0)
      {
         if (paramtype == buffer_rmcios || paramtype == channel_rmcios)
         {
            print_param (context, this->write_command,
                         channel_rmcios,
                         (void *) this->commanded_channel, num_params, param);
         }
         else
         {
            print_param (context, this->write_command,
                         channel_rmcios,
                         (void *) this->commanded_channel, 0, NULL);
         }
      }
      break;
   }
}

///////////////////////////////////////////////
// branch input class
///////////////////////////////////////////////
struct branch_data
{
   int num_linked;
   int *linked_channels;
};

void branch_class_func (struct branch_data *this,
                        const struct context_rmcios *context, int id,
                        enum function_rmcios function,
                        enum type_rmcios paramtype,
                        union param_rmcios returnv, int num_params,
                        const union param_rmcios param)
{
   int i;
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "branch channel help:\r\n"
                     " create branch newname\r\n"
                     " setup newname ch1 ch2 ch3... \r\n"
                     "  #link branch to multiple channels\r\n"
                     " write newname data \r\n"
                     "  #write to all configured channels\r\n"
                     " read newname channel \r\n"
                     "  #read all configured channels\n"
                     " link newname channel \r\n"
                     "  #link all configured channels to channel\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
      {
         break;
      }
      // allocate new data
      this = (struct branch_data *) 
             allocate_storage (context, sizeof (struct branch_data), 0); 

      //default values :
      this->num_linked = 0;

      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) branch_class_func, this); 
      break;

   case setup_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
      { // no linked channels
         if (this->num_linked != 0)
            free (this->linked_channels);
         this->num_linked = 0;
         break;
      }
      if (this->num_linked != 0)
         free (this->linked_channels);
      this->num_linked = num_params;
      this->linked_channels = (int *) malloc (sizeof (int) * this->num_linked);

      for (i = 0; i < this->num_linked; i++)
      { 
         // attach linked channels
         this->linked_channels[i] = param_to_int (context, paramtype, param, i);

      }
      break;
   case write_rmcios:
   case read_rmcios:
      if (this == NULL)
         break;
      for (i = 0; i < this->num_linked; i++)
      { // execute linked channels

         context->run_channel (context, this->linked_channels[i],
                               function, paramtype, returnv, num_params, param);
         if (returnv.p != NULL)
            return_string (context, paramtype, returnv, " ");
      }
      break;
   default:
      break;
   }
}

//////////////////////////////////////
//! Channel for parsing serial data //
//////////////////////////////////////
struct parser_data
{
   // array representing Start Of Message symbol // som=NULL -> dont use
   char *som;                   
   // length of som array
   unsigned int som_len;        
   // array representing End Of Line delimiter symbol eol=NULL -> dont use
   char *eol;                   
   // length of array
   unsigned int eol_len;        
   // array representing Column Separator cs=NULL -> dont use
   char *cs;                    
   // length of array
   unsigned int cs_len;         
   // array representing data Keyword kw=NULL -> dont use
   char *kw;                    
   // length of array
   unsigned int kw_len;         
   // string representing Stop Of Data ed=NULL -> dont use
   char *sod;                   
   // length of array
   unsigned int sod_len;        
   // column number to be picked column<0 -> dont use
   int column;                  
   
   // line number to be picked line<0 -> dont use
   int line;                    
   // buffer to store latest value
   char *buffer;                
   // buffer to store latest value
   unsigned int buffer_len;     

   int state;
   int i_col;
   int i_line;
   int i_buf;
   //  wait_MESSAGE=0,
   //  COUNT_LINES=1,
   //  COUNT_COUMNS=2,
   //  WAIT_KEYWORD=3,
   //  RECEPTION=4
};


// int=min 32bit little-endian
#define intstr(a,b,c) (a | b<<8 | c<<16)        

void parser_class_func (struct parser_data *this,
                        const struct context_rmcios *context, int id,
                        enum function_rmcios function,
                        enum type_rmcios paramtype,
                        union param_rmcios returnv, int num_params,
                        const union param_rmcios param)
{
   int plen;
   switch (function)
   {
   case help_rmcios:
      // Parser filters table/keyword based message data.
      // Saves filtered data to local buffer. Sends it to linked channel.
      // Filters can be used or left unsed.(filter condition skipped)
      // Buffer is cleared on the first match.  

      // Filtering procedure stages 
      // (stage can be skipped by not defining identifier, 
      // or specifying 0 to line/column):
      // 0. wait for SOM. Table indexing starts.
      // 1. count EOL to find right line.
      // 2. count CS to find right column.
      // 3. wait for KW.
      // 4. enable reception (stop when SOD found)
      // 5. start from beginning.
      return_string (context, paramtype, returnv,
                     "parser - "
                     "tool for picking single value from stream message\r\n"
                     "create parser name "
                     "   # -create new parser instance\r\n"
                     "setup parser parameter value \r\n"
                     "   # -parameter is the parameter to be configuread:\r\n"
                     "setup parser cs identifier "
                     "   # -Column separator\r\n"
                     "setup parser sod identifier "
                     "   # -stop of data trigger \r\n"
                     "setup parser col number "
                     "   # -set the column number 0=first\r\n"
                     "setup parser buf buffer_length\r\n"
                     "write parsr data #feed input data to parser\r\n"
                     "write parser"
                     "   # -empty write resets parser state and buffer\r\n"
                     "read parser "
                     "   # -read latest parser buffered data.\r\n"
                     "link parser channel "
                     "   # -link parser output to another channel\r\n");
      break;
   case create_rmcios:
      if (num_params < 1)
         break;
      
      // allocate new data
      this = (struct parser_data *) 
             allocate_storage (context, sizeof (struct parser_data), 0); 

      //default values :
      this->som = NULL; // Start Of Message symbol
      this->som_len = 0;        // length of array
      this->eol = "\n"; // End Of Line delimiter symbol
      this->eol_len = 1;        // length of array
      this->cs = " ";   // Column Separator symbol
      this->cs_len = 1; // length of array
      this->kw = NULL;  // Keyword
      this->kw_len = 0; // length of array
      this->sod = "\n"; // Stop Of Data symbol
      this->sod_len = 1;
      this->line = 0;
      this->column = 0;
      // buffer to store latest value
      this->buffer = (char *) malloc (20);      
      // buffer to store latest value
      this->buffer_len = 20;    
      this->buffer[0] = 0;
      this->state = 0;
      this->i_line = 0;
      this->i_col = 0;
      this->i_buf = 0;

      // create channel
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) parser_class_func, this);      
      break;
   case setup_rmcios:
      if (num_params < 2)
         break;
      if (this == NULL)
         break;
      else
      {
         int i = 0;
         unsigned int pstr = 0;
         param_to_string (context, paramtype, param, 0, sizeof (pstr),
                          (char *) (&pstr));
         int symlen;
         symlen = param_string_length (context, paramtype, param, 1);
         switch (pstr)
         {
         case intstr ('s', 'o', 'm'):
            if (this->som != NULL)
               free ((void *) this->som);
            this->som = (char *) malloc (symlen + 1);
            // copy string to buffer
            param_to_string (context, paramtype, param, 1, 
                             symlen + 1, this->som);      
            this->som_len = symlen;
            break;
         case intstr ('e', 'o', 'l'):
            if (this->eol != NULL
                && !(this->eol_len == 1 && this->eol[0] == '\n'))
               free ((void *) this->som);
            this->eol = (char *) malloc (symlen + 1);
            param_to_string (context, paramtype, param, 1,
                             symlen + 1, this->eol);
            this->eol_len = symlen;
            break;
         case intstr ('c', 's', 0):
            if (this->cs != NULL && !(this->cs_len == 1 && this->cs[0] == ' '))
               free ((void *) this->cs);
            this->cs = (char *) malloc (symlen + 1);
            param_to_string (context, paramtype, param, 1,
                             symlen + 1, this->cs);
            this->cs_len = symlen;
            break;
         case intstr ('k', 'w', 0):
            if (this->kw != NULL)
               free ((void *) this->kw);
            this->kw = (char *) malloc (symlen + 1);
            param_to_string (context, paramtype, param, 1,
                             symlen + 1, this->kw);
            this->kw_len = symlen;
            break;
         case intstr ('s', 'o', 'd'):
            if (this->sod != NULL
                && !(this->sod_len == 1 && this->sod[0] == '\n'))
               free ((void *) this->sod);
            this->sod = (char *) malloc (symlen + 1);
            param_to_string (context, paramtype, param, 1,
                             symlen + 1, this->sod);
            this->sod_len = symlen;
            break;
         case intstr ('c', 'o', 'l'):
            this->column = param_to_int (context, paramtype, param, 1);
            break;

         case intstr ('l', 'i', 'n'):
            this->line = param_to_int (context, paramtype, param, 1);
            break;
         }
         this->buffer[0] = 0;
         this->state = 0;
         this->i_line = 0;
         this->i_col = 0;
         this->i_buf = 0;
      }
      break;
   case write_rmcios:
      if (this == NULL)
         break;
      
      // Empty write resets the state and buffers
      if (num_params < 1)       
      {
         this->buffer[0] = 0;
         this->state = 0;
         this->i_line = 0;
         this->i_col = 0;
         this->i_buf = 0;
         break;
      }

      plen = param_string_alloc_size (context, paramtype, param, 0);
      {
         char nbuffer[plen];
         const char *s;
         int n;
         int i;

         s = param_to_string (context, paramtype, param, 0, plen, nbuffer);
         n = strlen (s);

         for (i = 0; i < n; i++)
         {

            switch (this->state)
            {
            case 0:    
               // 0. wait for SOM. Table indexing starts.
               //if(this->som_len!=0) break ;
            case 1:    
               // 1. count EOL to find right line.
               //if(this->eol_len!=0 && this->line!=0 ) break ;
            case 2:    
               // 2. count CS to find right column
               if (this->cs_len == 1)
               {
                  if (s[i] == this->cs[0])
                  {
                     this->i_col++;     // found column
                  }
                  if (this->i_col >= this->column)
                  {
                     this->state = 3;
                  }
                  if (this->column != 0)
                     break;
               }
            case 3:    // 3. wait for KW. (keyword)
               //if(this->kw_len!=0) break ;
            case 4:    
               // 4. enable reception 
               //(when SOD found: transmit buffered data to linked channel+stop
               {
                  char ch[2] = { 0, 0 };
                  ch[0] = s[i];

                  this->buffer[this->i_buf++] = s[i];
                  if (this->i_buf >= this->buffer_len)
                     this->i_buf = this->buffer_len - 1;

                  this->buffer[this->i_buf] = 0;
                  this->state = 4;
                  if (this->sod_len == 1 && s[i] == this->sod[0])
                  {
                     write_str (context,
                                linked_channels (context, id), this->buffer, 0);
                     // reset
                     this->buffer[0] = 0;
                     this->state = 0;
                     this->i_line = 0;
                     this->i_col = 0;
                     this->i_buf = 0;
                  }
                  break;
               }
            }
         }
      }
      break;

   case read_rmcios:
      if (this == NULL)
         break;
      return_string (context, paramtype, returnv, this->buffer);
      break;
   }
}
void init_legacy_channels (const struct context_rmcios *context)
{
   create_channel_str (context, "commander",
                       (class_rmcios) commander_class_func, NULL);
   create_channel_str (context, "branch", (class_rmcios) branch_class_func,
                       NULL);
   create_channel_str (context, "group", (class_rmcios) branch_class_func,
                       NULL);
   create_channel_str (context, "parser", (class_rmcios) parser_class_func,
                       NULL);
}
