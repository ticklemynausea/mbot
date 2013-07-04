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

#ifndef _H_LIST
#define _H_LIST

#include "system.h"

class List {
public:
  EXPORT List (void);
  EXPORT ~List (void);

  EXPORT bool add (void *);
  EXPORT bool del (void *);
  EXPORT void delall (void);
  EXPORT void rewind (void);
  EXPORT void *next (void);
  EXPORT size_t count (void);
  EXPORT void restore_current (void);
  EXPORT bool exist (void *);

private:

  struct element_type {
    void *e;
    struct element_type *next;
  } *head, *current, *rewinded_current;

};

#endif	// _H_LIST

