/*

This file is part of mbot.

mbot is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

mbot is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with mbot; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "Text.h"

Text::Text (void) : line_num (0), filename (PATH_MAX), lines (NULL),
  lines_cur (NULL), line_cur (0)
{
}

Text::~Text (void)
{
  delete_lines ();
}

// read a text file into memory
bool
Text::read_file (c_char name)
{
  char bufread[LINE_SIZE+1];
  filename = name;
  ifstream f ((c_char)filename, ios::in | IOS_NOCREATE);
  if (!f)
    return 0;
  while (f.getline (bufread, LINE_SIZE))
    {
      strip_crlf (bufread);
      add_line (bufread);
    }
  rewind_text ();
  return 1;
}

// move the current line pointer to the beginning
void
Text::rewind_text (void)
{
  lines_cur = lines;
  line_cur = 0;
}

// get the current line and move to the next
c_char 
Text::get_line (void)
{
  char *tmp = NULL;
  if (lines_cur != NULL)
    {
      tmp = lines_cur->text;
      lines_cur = lines_cur->next;
      line_cur++;
    }
  return tmp;
}

// delete all lines
void
Text::delete_lines (void)
{
  line_type *l = lines, *l2;
  while (l != NULL)
    {
      l2 = l->next;
      if (l->text != NULL)
        free (l->text);
      delete l;
      l = l2;
    }
  lines = NULL;
  line_num = line_cur = 0;
}

// add a text line to the end, unless it's an empty line
void
Text::add_line (c_char t)
{
  if (t == NULL)
    return;
  if (lines == NULL)				// if it's empty
    {
      lines = new line_type;
      lines->text = NULL;
      strset (&lines->text, t, MSG_SIZE);
      lines->next = NULL;
    }
  else
    {
      line_type *l_buf = lines;
      while (l_buf->next != NULL)		// else find the last
        l_buf = l_buf->next;
      l_buf->next = new line_type;
      l_buf->next->text = NULL;
      strset (&l_buf->next->text, t, MSG_SIZE);
      l_buf->next->next = NULL;
    }
  line_num++;
}

