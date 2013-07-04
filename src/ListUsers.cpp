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

#include "ListUsers.h"

ListUsers::ListUsers (NetServer *server) : List (),
  filename (PATH_MAX), s (server)
{
}

ListUsers::~ListUsers (void)
{
  user_type *u;
  rewind ();
  while ((u = (user_type *)next ()) != NULL)
    delete u;
}

// load users from <file>, return 0 on failure
bool
ListUsers::load_users (c_char file)
{
  filename = file;
  return load_users ();
}

// load users from file, return 0 on failure
bool
ListUsers::load_users (void)
{
  ifstream f ((c_char)filename, ios::in | IOS_NOCREATE);
  if (!f)
    {
      s->write_botlog ("error opening user file %s: %s", (c_char)filename, strerror (errno));
      return 0;
    }
  char *b, *u_mask = NULL, *u_pass = NULL, *bufsep;
  int u_level = 0, line = 0;
  char buf[LINE_SIZE+1];
  while (f.getline (buf, LINE_SIZE))
    {
      bufsep = buf;
      line++;
      if (buf[0] == '#')
        continue;
      b = strsep (&bufsep, ":");
      if (b != NULL)
        {
          u_mask = b;
          b = strsep (&bufsep, ":");
          if (b != NULL)
            {
              u_pass = b;
              b = strsep (&bufsep, ":");
              if (b != NULL && bufsep != NULL)
                {
                  u_level = atoi (b);
                  addlist_user (u_mask, u_pass, u_level, bufsep);
                  continue;
                }
            }
        }
      bot->write_botlog ("error reading %s: invalid field (line %d)",
                            (c_char)filename, line);
      return 0;
    }
  return 1;
}

// save users to user file, return 0 on failure
bool
ListUsers::save_users (void)
{
  ofstream f ((c_char)filename);
  if (!f)
    {
      s->write_botlog ("error opening user file %s: %s", (c_char)filename, strerror (errno));
      return 0;
    }
  f << "# DON'T EDIT THIS FILE MANUALLY - USE MBOT COMMANDS INSTEAD\n";
  user_type *u;
  rewind ();
  while ((u = (user_type *)next ()) != NULL)
    f << u->mask << ":" << u->pass << ":" << u->level << ":"
      << (u->msg ? (c_char)u->msg : "") << endl;
  return 1;
}

// <msg> can be NULL to disable it
void
ListUsers::addlist_user (c_char mask, c_char crypt_pass, int level, c_char msg)
{
  user_type *u = new user_type (mask, crypt_pass, level, msg);
  add (u);
}

// return 0 if not found
bool
ListUsers::dellist_user (c_char mask)
{
  user_type *u = abs_mask2user (mask);
  if (u == NULL)
    return 0;
  delete u;
  del (u);
  return 1;
}

// add a user, return 0 on failure
// if <pass> is NULL, create passwordless user
bool
ListUsers::add_user (c_char mask, c_char pass, int level, c_char msg)
{
  if (pass == NULL)
    addlist_user (mask, "", level, msg);
  else
    addlist_user (mask, crypt (pass, random_salt ()), level, msg);
  return save_users ();
}

// delete a user, return 0 on failure
bool
ListUsers::del_user (c_char mask)
{
  if (!dellist_user (mask))
    return 0;
  return save_users ();
}

// return 1 if <pass> is user <mask>'s password
bool
ListUsers::match_check_pass (c_char mask, c_char pass)
{
  user_type *u = match_mask2user (mask);
  if (u == NULL)
    return 0;
  if ((c_char)u->pass == NULL || u->pass[0] == 0)
    return 1;
  if (u->pass != crypt (pass, u->pass))
    return 0;
  return 1;
}

bool
ListUsers::abs_check_pass (c_char abs_mask, c_char pass)
{
  user_type *u = abs_mask2user (abs_mask);
  if (u == NULL)
    return 0;
  if ((c_char)u->pass == NULL || u->pass[0] == 0)
    return 1;
  if (u->pass != crypt (pass, u->pass))
    return 0;
  return 1;
}

// set user <mask>'s password to <pass>
bool
ListUsers::match_set_pass (c_char mask, c_char pass)
{
  user_type *u = match_mask2user (mask);
  if (u == NULL)
    return 0;
  u->pass = crypt (pass, random_salt ());
  return save_users ();
}

bool
ListUsers::abs_set_pass (c_char abs_mask, c_char pass)
{
  user_type *u = abs_mask2user (abs_mask);
  if (u == NULL)
    return 0;
  u->pass = crypt (pass, random_salt ());
  return save_users ();
}

bool
ListUsers::match_set_msg (c_char mask, c_char msg)
{
  user_type *u = match_mask2user (mask);
  if (u == NULL)
    return 0;
  u->msg = msg;
  return save_users ();
}

bool
ListUsers::abs_set_msg (c_char abs_mask, c_char msg)
{
  user_type *u = abs_mask2user (abs_mask);
  if (u == NULL)
    return 0;
  u->msg = msg;
  return save_users ();
}

bool
ListUsers::abs_set_mask (c_char abs_mask, c_char new_abs_mask)
{
  user_type *u = abs_mask2user (abs_mask);
  if (u == NULL)
    return 0;
  u->mask = new_abs_mask;
  return save_users ();
}

bool
ListUsers::match_set_level (c_char mask, int level)
{
  user_type *u = match_mask2user (mask);
  if (u == NULL)
    return 0;
  u->level = level;
  return save_users ();
}

bool
ListUsers::abs_set_level (c_char abs_mask, int level)
{
  user_type *u = abs_mask2user (abs_mask);
  if (u == NULL)
    return 0;
  u->level = level;
  return save_users ();
}

bool
ListUsers::match_set_id (c_char mask, bool id)
{
  user_type *u = match_mask2user (mask);
  if (u == NULL)
    return 0;
  u->id = id;
  if (id)
    u->cur_mask = mask;
  else
    u->cur_mask = NULL;
  return 1;
}

ListUsers::user_type *
ListUsers::match_mask2user (c_char mask)
{
  user_type *u;
  rewind ();
  while ((u = (user_type *)next ()) != NULL)
    if (match_mask (mask, u->mask))
      return u;
  return NULL;
}

ListUsers::user_type *
ListUsers::abs_mask2user (c_char abs_mask)
{
  user_type *u;
  rewind ();
  while ((u = (user_type *)next ()) != NULL)
    if (u->mask |= abs_mask)
      return u;
  return NULL;
}

int
ListUsers::match_mask_level (c_char mask)
{
  user_type *u = match_mask2user (mask);
  if (u != NULL)
    {
      if (u->level < 0)
        return u->level;
      else
        {
          if (u->id && u->cur_mask == mask)
            return u->level;
          else
            return 1;
        }
    }
  return 0;
}

int
ListUsers::match_mask_reallevel (c_char mask)
{
  user_type *u = match_mask2user (mask);
  if (u != NULL)
    return u->level;
  return 0;
}

