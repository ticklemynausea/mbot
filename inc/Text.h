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

#ifndef _H_TEXT
#define _H_TEXT

#include "Strings.h"

class Text {
public:

  EXPORT Text (void);
  EXPORT ~Text (void);

  EXPORT bool read_file (c_char);
  EXPORT void rewind_text (void);
  EXPORT c_char get_line (void);
  EXPORT void delete_lines (void);

  int line_num;				// total lines

private:

  void add_line (c_char);
  void merge(Text* t);
  
  String filename;

  struct line_type {
    char *text;
    line_type *next;
  } *lines, *lines_cur;

  int line_cur;				// current line number

};

#endif	// _H_TEXT
