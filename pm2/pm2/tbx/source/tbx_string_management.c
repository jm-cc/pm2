/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/*
 * tbx_string_management.c
 * :::::::::::::::::::::::////////////////////////////////////////////////////
 */

#include "tbx.h"
#include <string.h>

/*
 *  Macros
 * ___________________________________________________________________________
 */
#define MAX_NUMBER_LENGTH 256

/*
 *  Functions
 * ___________________________________________________________________________
 */


/*
 * Initialization
 * --------------
 */
void
tbx_string_manager_init(void)
{
  LOG_IN();
  LOG_OUT();
}

void
tbx_string_manager_exit(void)
{
  LOG_IN();
  LOG_OUT();
}

char *
tbx_strdup(const char *src)
{
  char   *dst    = NULL;
  size_t  length =    0;
  
  LOG_IN();
  length = strlen(src);
  dst    = TBX_MALLOC(length + 1);
  CTRL_ALLOC(dst);

  strcpy(dst, src);
  LOG_OUT();  

  return dst;
}


p_tbx_string_t
tbx_string_init(void)
{
  p_tbx_string_t string = NULL;

  LOG_IN();
  string = TBX_CALLOC(1, sizeof(tbx_string_t));
  CTRL_ALLOC(string);

  string->length           =    0;
  string->allocated_length =    0;
  string->data             = NULL;
  LOG_OUT();

  return string;
}

void
tbx_string_free(p_tbx_string_t string)
{
  LOG_IN();
  tbx_string_reset(string);
  TBX_FREE(string);
  LOG_OUT();
}

size_t
tbx_string_length(p_tbx_string_t string)
{
  size_t length = 0;
  
  LOG_IN();
  length = string->length;
  LOG_OUT();

  return length;
}

void
tbx_string_reset(p_tbx_string_t string)
{
  LOG_IN();
  if (string->allocated_length)
    {
      TBX_FREE(string->data);
    }
  
  string->length           =    0;
  string->allocated_length =    0;
  string->data             = NULL;
  LOG_OUT();  
}

char *
tbx_string_to_c_string(p_tbx_string_t string)
{
  char   *c_string = NULL;
  size_t  length   =    0;
  
  LOG_IN();
  length   = string->length;
  
  c_string = TBX_MALLOC(length + 1);
  CTRL_ALLOC(c_string);
  
  if (length)
    {
      memcpy(c_string, string->data, length);
    }
  
  c_string[length] = '\0';
  LOG_OUT();  

  return c_string;
}

char *
tbx_string_to_c_string_and_free(p_tbx_string_t string)
{
  char *c_string = NULL;
  
  LOG_IN();
  c_string = tbx_string_to_c_string(string);
  tbx_string_free(string);
  LOG_OUT();  

  return c_string;
}

void
tbx_string_set_to_c_string(p_tbx_string_t  string,
			   const char     *c_string)
{
  size_t length = 0;

  LOG_IN();
  tbx_string_reset(string);
  if ((length = strlen(c_string)))
    {
      string->data = TBX_MALLOC(length);
      CTRL_ALLOC(string->data);
      
      memcpy(string->data, c_string, length);
      string->length           = length;
      string->allocated_length = length;
    }  
  LOG_OUT();
}

void
tbx_string_set_to_c_string_and_free(p_tbx_string_t  string,
				    char           *c_string)
{
  LOG_IN();
  tbx_string_set_to_c_string(string, c_string);
  TBX_FREE(c_string);
  LOG_OUT();
}

void
tbx_string_set_to_char(p_tbx_string_t string,
		       int            data)
{
  LOG_IN();
  tbx_string_reset(string);
  string->data = TBX_MALLOC(1);
  CTRL_ALLOC(string->data);
      
  string->data[0]          = data;
  string->length           =    1;
  string->allocated_length =    1;
  LOG_OUT();
}

p_tbx_string_t
tbx_string_init_to_c_string(const char *c_string)
{
  p_tbx_string_t string = NULL;

  LOG_IN();
  string = tbx_string_init();
  tbx_string_set_to_c_string(string, c_string);
  LOG_OUT();

  return string;
}

p_tbx_string_t
tbx_string_init_to_c_string_and_free(char *c_string)
{
  p_tbx_string_t string = NULL;

  LOG_IN();
  string = tbx_string_init();
  tbx_string_set_to_c_string_and_free(string, c_string);
  LOG_OUT();

  return string;
}

p_tbx_string_t
tbx_string_init_to_char(const int data)
{
  p_tbx_string_t string = NULL;

  LOG_IN();
  string = tbx_string_init();
  tbx_string_set_to_char(string, data);
  LOG_OUT();

  return string;
}

void
tbx_string_append_c_string(p_tbx_string_t  string,
			   const char     *c_string)
{
  LOG_IN();
  if (string->allocated_length)
    {
      size_t length = 0;
      
      if ((length = strlen(c_string)))
	{
	  if (string->length + length > string->allocated_length)
	    {
	      string->allocated_length =
		max(2 * (string->allocated_length + 1),
		    string->allocated_length + length);
	      string->data =
		TBX_REALLOC(string->data, string->allocated_length);
	      CTRL_ALLOC(string->data);
	    }

	  memcpy(string->data + string->length, c_string, length);
	  string->length += length;
	}
    }
  else
    {
      tbx_string_set_to_c_string(string, c_string);
    }
  LOG_OUT();
}

void
tbx_string_append_c_string_and_free(p_tbx_string_t  string,
				    char           *c_string)
{
  LOG_IN();
  tbx_string_append_c_string(string, c_string);
  TBX_FREE(c_string);
  LOG_OUT();
}

void
tbx_string_append_char(p_tbx_string_t string,
		       int            data)
{
  LOG_IN();
  if (string->allocated_length)
    {
      string->length++;
      if (string->length > string->allocated_length)
	{
	  string->allocated_length = 2 * (string->allocated_length + 1);
	  string->data = TBX_REALLOC(string->data, string->allocated_length);
	  CTRL_ALLOC(string->data);
	}
      
      string->data[string->length - 1] = (char)data;
    }
  else
    {
      tbx_string_set_to_char(string, data);
    }
  LOG_OUT();
}

void
tbx_string_append_int(p_tbx_string_t string,
		      int            data)
{
  char       temp[MAX_NUMBER_LENGTH + 1];
  size_t     offset = 0;
  tbx_bool_t neg    = (data < 0);

  LOG_IN();  
  if (neg)
    {
      data = -data;
    }
      
  do
    {
      temp[offset++] = '0' + data % 10;
      data /= 10;
    }
  while (data);
  
  if (neg)
    {
      temp[offset++] = '-';
    }
  
  do
    {
      tbx_string_append_char(string, temp[--offset]);
    }
  while (offset);
  LOG_OUT();
}

void
tbx_string_set_to_int(p_tbx_string_t string,
		      int            data)
{
  LOG_IN(); 
  tbx_string_reset(string);
  tbx_string_append_int(string, data);
  LOG_OUT();
}

p_tbx_string_t
tbx_string_init_to_int(int data)
{
  p_tbx_string_t string = NULL;

  LOG_IN();
  string = tbx_string_init();
  tbx_string_set_to_int(string, data);
  LOG_OUT();

  return string;
}

void
tbx_string_reverse(p_tbx_string_t string)
{
  size_t offset_left  = 0;
  size_t offset_right = string->length - 1;
  
  LOG_IN();
  while (offset_left < offset_right)
    {
      string->data[offset_left]  ^= string->data[offset_right];
      string->data[offset_right] ^= string->data[offset_left];
      string->data[offset_left]  ^= string->data[offset_right];
      offset_left  ++;
      offset_right --;
    }
  LOG_OUT();
}

void
tbx_string_set_to_string(p_tbx_string_t dst_string,
			 p_tbx_string_t src_string)
{
  LOG_IN(); 
  tbx_string_reset(dst_string);
  if (src_string->length)
    {
      dst_string->data = TBX_MALLOC(src_string->length);
      CTRL_ALLOC(dst_string->data);
      
      memcpy(dst_string->data, src_string->data, src_string->length);
      dst_string->length           = src_string->length;
      dst_string->allocated_length = src_string->length;
    }
  LOG_OUT();
}

void
tbx_string_set_to_string_and_free(p_tbx_string_t dst_string,
				  p_tbx_string_t src_string)
{
  LOG_IN();
  tbx_string_set_to_string(dst_string, src_string);
  tbx_string_free(src_string);
  LOG_OUT();
}

void
tbx_string_append_string(p_tbx_string_t dst_string,
			 p_tbx_string_t src_string)
{
  LOG_IN();
  if (src_string->length)
    {
      if (dst_string->allocated_length)
	{
	  if (dst_string->length + src_string->length >
	      dst_string->allocated_length)
	    {
	      dst_string->allocated_length =
		max(2 * (dst_string->allocated_length + 1),
		    dst_string->allocated_length + src_string->length);
	      dst_string->data =
		TBX_REALLOC(dst_string->data, dst_string->allocated_length);
	      CTRL_ALLOC(dst_string->data);
	    }

	  memcpy(dst_string->data + dst_string->length, 
		 src_string->data, src_string->length);
	  dst_string->length += src_string->length;
	}
      else
	{
	  tbx_string_set_to_string(dst_string, src_string);
	}
    }
  LOG_OUT();
}

void
tbx_string_append_string_and_free(p_tbx_string_t dst_string,
				  p_tbx_string_t src_string)
{
  LOG_IN();
  tbx_string_append_string(dst_string, src_string);
  tbx_string_free(src_string);
  LOG_OUT();
}

p_tbx_string_t
tbx_string_init_to_string(p_tbx_string_t src_string)
{
  p_tbx_string_t dst_string = NULL;

  LOG_IN();
  dst_string = tbx_string_init();
  tbx_string_set_to_string(dst_string, src_string);
  LOG_OUT();

  return dst_string;
}

p_tbx_string_t
tbx_string_init_to_string_and_free(p_tbx_string_t src_string)
{
  p_tbx_string_t dst_string = NULL;

  LOG_IN();
  dst_string = tbx_string_init_to_string(src_string);
  tbx_string_free(src_string);
  LOG_OUT();

  return dst_string;
}

p_tbx_string_t
tbx_string_double_quote(p_tbx_string_t src_string)
{
  char           *src_data   = src_string->data;
  size_t          src_length = src_string->length;
  p_tbx_string_t  dst_string = NULL;
  size_t          offset     =    0;

  LOG_IN();
  // Leading double quote
  dst_string = tbx_string_init_to_char('"');
  
  while (offset < src_length)
    {
      switch (src_data[offset])
	{
	case '\\':
	case '"':
	  {
	    tbx_string_append_char(dst_string, '\\');
	  }
	  break;
 	default:
	  {
	    tbx_string_append_char(dst_string, src_data[offset++]);
	  }
	  break;
	}
    }
  

  // Trailing double quote
  tbx_string_append_char(dst_string, '"');
  LOG_OUT();

  return dst_string;
}

p_tbx_string_t
tbx_string_double_quote_and_free(p_tbx_string_t src_string)
{
  p_tbx_string_t dst_string = NULL;

  LOG_IN();
  dst_string = tbx_string_double_quote(src_string);
  tbx_string_free(src_string);
  LOG_OUT();

  return dst_string;
}

p_tbx_string_t
tbx_string_single_quote(p_tbx_string_t src_string)
{
  p_tbx_string_t dst_string = NULL;

  LOG_IN();
  dst_string = tbx_string_init_to_char('\'');
  tbx_string_append_string(dst_string, src_string);
  tbx_string_append_char(dst_string, '\'');
  LOG_OUT();

  return dst_string;
}

p_tbx_string_t
tbx_string_single_quote_free(p_tbx_string_t src_string)
{
  p_tbx_string_t dst_string = NULL;

  LOG_IN();
  dst_string = tbx_string_single_quote(src_string);
  tbx_string_free(src_string);
  LOG_OUT();

  return dst_string;
}

p_tbx_slist_t
tbx_string_split(p_tbx_string_t  src_string,
		 const char     *IFS)
{
  p_tbx_slist_t slist = NULL;
  
  slist = tbx_slist_nil();

  LOG_IN();
  if (src_string->length)
    {
      p_tbx_string_t  dst_string   = NULL;
      char           *data         = src_string->data;
      size_t          length       = src_string->length;
      size_t          offset       = 0;
      tbx_bool_t      single_mode  = tbx_false;
      tbx_bool_t      double_mode  = tbx_false;
      tbx_bool_t      copy_mode    = tbx_false;
      tbx_bool_t      quoting_mode = tbx_false;

      IFS = (IFS)?:" \t\n";

      do
	{	  
	  if (copy_mode)
	    {
	      if (!single_mode && !double_mode && !quoting_mode)
		{
		  const char *ptr = IFS;
		  
		  while (*ptr)
		    {
		      if (*ptr == data[offset])
			{
			  offset++;
			  copy_mode = tbx_false;
			  
			  tbx_slist_append(slist, dst_string);
			  dst_string = NULL;
			  
			  goto next_1;
			}
		      
		      ptr++;
		    }
		}
	      
	      switch (data[offset])
		{
		case '\\':
		  {
		    if (!single_mode)
		      {
			quoting_mode = !quoting_mode;
		      }
		  }
		  break;
		case '\'':
		  {
		    if (!quoting_mode && !double_mode)
		      {
			single_mode = !single_mode;
		      }
		  }
		  break;
		case '"':
		  {
		    if (!quoting_mode && !single_mode)
		      {
			double_mode = !double_mode;
		      }
		  }
		  break;
		default:
		  {
		    /* Nothing */
		  }
		  break;
		}

	      tbx_string_append_char(dst_string, data[offset++]);
	    }
	  else
	    {
	      const char *ptr = IFS;

	      while (*ptr)
		{
		  if (*ptr == data[offset])
		    {
		      offset++;
		      goto next_1;
		    }

		  ptr++;
		}

	      dst_string = tbx_string_init();
	      copy_mode = tbx_true;
	    }
	next_1: 
	}
      while (offset < length);
  
      if (dst_string)
	{
	  tbx_slist_append(slist, dst_string);
	  dst_string = NULL;      
	}
    }
  LOG_OUT();

  return slist;
}

p_tbx_slist_t
tbx_string_split_and_free(p_tbx_string_t  src_string,
			  const char     *IFS)
{
  p_tbx_slist_t slist = NULL;

  LOG_IN();
  slist = tbx_string_split(src_string, IFS);
  tbx_string_free(src_string);
  LOG_OUT();

  return slist;
}

p_tbx_string_t
tbx_string_extract_name_from_pathname(p_tbx_string_t path_name)
{
  p_tbx_string_t name  = NULL;
  size_t         idx = 0;

  LOG_IN();
  name = tbx_string_init();
  
  idx = path_name->length;
  if (idx)
    {
      while ((idx > 0) && (path_name->data[idx - 1] != '\\'))
	{
	  idx--;
	}

      name->length = path_name->length - idx;
      if (name->length)
	{
	  name->allocated_length = name->length;
	  name->data = TBX_MALLOC(name->allocated_length);
	  CTRL_ALLOC(name->data);
	  
	  memcpy(name->data, path_name->data + idx, name->length);

	  path_name->length = (idx > 0)?idx - 1:idx;

	  if (!path_name->length)
	    {
	      TBX_FREE(path_name->data);
	      path_name->data             = NULL;
	      path_name->allocated_length = 0;
	    }
	}
    }
  
  LOG_OUT();

  return name;
}
