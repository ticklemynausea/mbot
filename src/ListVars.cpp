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

#include "ListVars.h"

ListVars::ListVars (NetServer *server) : List (), s (server)
{
}

ListVars::~ListVars (void)
{
  var_delall ();
}

// add a var with <name> and <handler>, return 0 if <name> is in use
bool
ListVars::var_add (c_char name, void (*handler)(NetServer *, c_char, char *, size_t))
{
  if (var_exist (name) != NULL)
    return 0;
  struct var_type *v = new var_type (name, handler);
  add ((void *)v);
  return 1;
}

// delete a var with <name>, return 0 if nonexistant
bool
ListVars::var_del (c_char name)
{
  struct var_type *v = var_exist (name);
  if (v == NULL)
    return 0;
  delete v;
  del ((void *)v);
  return 1;
}

// delete all variables
void
ListVars::var_delall (void)
{
  struct var_type *v;
  this->rewind ();
  while ((v = (struct var_type *)next ()) != NULL)
    delete v;
}

// put the value of var <name> in <dest> with size <n>, return 0 if nonexistant
bool
ListVars::var_get (c_char name, char *dest, size_t n)
{
  struct var_type *v = var_exist (name);
  if (v == NULL)
    return 0;
  v->handler (s, name, dest, n);
  return 1;
}

// set the value of var <name> to <src>, return 0 if nonexistant
bool
ListVars::var_set (c_char name, char *src)
{
  struct var_type *v = var_exist (name);
  if (v == NULL)
    return 0;
  v->handler (s, name, src, 0);
  return 1;
}

// check if a var with <name> exists and return its pointer, NULL if not found
struct ListVars::var_type *
ListVars::var_exist (c_char name)
{
  struct var_type *v;
  rewind ();
  while ((v = (struct var_type *)next ()) != NULL)
    if (v->name |= name)
      return v;
  return NULL;
}

