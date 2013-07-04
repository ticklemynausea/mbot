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

#include "List.h"

List::List (void)
{
  rewinded_current = current = head = new element_type;
  head->next = NULL;
}

List::~List (void)
{
  delall ();
  delete head;
}

bool
List::add (void *e)
{
  struct element_type *etmp = new element_type;
  if (etmp == NULL)
    return 0;
  etmp->e = e;
  etmp->next = head->next;
  head->next = etmp;
  return 1;
}

bool
List::del (void *e)
{
  struct element_type *etmp = head, *enext;
  while (etmp->next != NULL)
    {
      if (etmp->next->e == e)
        {
          enext = etmp->next->next;
          delete (etmp->next);
          if (current == etmp->next)
            current = etmp;
          etmp->next = enext;
          return 1;
        }
      etmp = etmp->next;
    }
  return 0;
}

void
List::delall (void)
{
  struct element_type *etmp = head->next, *enext;
  while (etmp != NULL)
    {
      enext = etmp->next;
      delete (etmp);
      etmp = enext;
    }
  head->next = NULL;
}

void
List::rewind (void)
{
  rewinded_current = current;
  current = head;
}

void *
List::next (void)
{
  if (current->next == NULL)
    return NULL;
  current = current->next;
  return current->e;
}

size_t
List::count (void)
{
  size_t num = 0;
  struct element_type *etmp = head->next;
  while (etmp != NULL)
    {
      num++;
      etmp = etmp->next;
    }
  return num;
}

void
List::restore_current (void)
{
  current = rewinded_current;
}

bool
List::exist (void *e)
{
  struct element_type *etmp = head->next;
  while (etmp != NULL)
    {
      if (etmp->e == e)
        return 1;
      etmp = etmp->next;
    }
  return 0;
}
