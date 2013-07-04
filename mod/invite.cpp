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

#include "mbot.h"

#define LEVEL_INVITE_PVT 0

#define HELP_INVITE_PVT "Makes the bot invite you to the configured channel"

struct invite_type {
  NetServer *s;
  String name;
  invite_type (NetServer *server) : s (server), name (CHANNEL_SIZE) {}
};
List *invite_list;

///////////////
// prototypes
///////////////

static void invite_cmd_invite (NetServer *);

static invite_type *server2invite (NetServer *);
static void invite_var (NetServer *, c_char, char *, size_t);
static void invite_conf (NetServer *, c_char);
static void invite_stop (void);
static void invite_start (void);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "invite",
  invite_start,
  invite_stop,
  invite_conf,
  NULL
};

}

/////////////
// commands
/////////////

// invite
static void
invite_cmd_invite (NetServer *s)
{
  invite_type *invite = server2invite (s);
  if (invite == NULL)
    return;
  // doesn't work inside #'s
  if ((bool)invite->name && CMD[3][6] == 0 && strcasecmp (CMD[2], s->nick) == 0)
    {
      int i = CHANNEL_INDEX (invite->name);	// if the bot is there..
      if (i != -1)
        CHANNELS[i]->irc_invite (s->script.source);	// ..invite
      return;
    }
}

////////////////////
// module managing
////////////////////

// returns the invite channel for a given server, NULL if nonexistant
static invite_type *
server2invite (NetServer *s)
{
  invite_type *a;
  invite_list->rewind ();
  while ((a = (invite_type *)invite_list->next ()) != NULL)
    if (a->s == s)
      return a;
  return NULL;
}

static void
invite_var (NetServer *s, c_char name, char *data, size_t n)
{
  invite_type *buf = server2invite (s);
  if (buf == NULL)
    return;
  if (strcasecmp (name, "invite_channel") == 0)
    {
      if (n != 0)
        my_null_strncpy (data, buf->name, n);
      else
        buf->name = data;
    }
}

// configuration file's local parser
static void
invite_conf (NetServer *s, c_char bufread)
{
  char buf[2][MSG_SIZE+1];

  strsplit (bufread, buf, 1);

  if (strcasecmp (buf[0], "bot") != 0)
    return;

  if (server2invite (s) != NULL)
    {
      s->b->conf_error ("invite already defined in this server");
      return;
    }

  invite_type *invite = new invite_type (s);
  if (invite == NULL)
    s->b->conf_error ("error initializing invite");
  invite_list->add ((void *)invite);
  s->script.cmd_bind (invite_cmd_invite, LEVEL_INVITE_PVT, "invite", module.mod, HELP_INVITE_PVT);
  s->vars.var_add ("invite_channel", invite_var);
}

// module termination
static void
invite_stop (void)
{
  invite_type *invite;
  invite_list->rewind ();
  while ((invite = (invite_type *)invite_list->next ()) != NULL)
    {
      invite->s->vars.var_del ("invite_channel");
      invite->s->script.cmd_unbind ("invite");
      delete invite;
    }
  delete invite_list;
}

// module initialization
static void
invite_start (void)
{
  invite_list = new List ();
}

