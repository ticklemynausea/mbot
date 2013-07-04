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

#define SOURCE s->script.source
#define DEST s->script.dest
#define BUF s->script.buf
#define SEND_TEXT s->script.send_text

struct getnick_type {
  NetServer *server;
  String nick;
  String cmd;
  getnick_type *next;
  getnick_type (NetServer *s, getnick_type *n) : server (s),
    nick (NICK_SIZE), cmd (MSG_SIZE), next (n) {}
};
getnick_type *getnick_list;

///////////////
// prototypes
///////////////

static void getnick_add (NetServer *);
static getnick_type *server2getnick (NetServer *);
static void getnick_event (NetServer *);
static void getnick_conf (NetServer *, c_char);
static void getnick_stop (void);
static void getnick_start (void);
static void getnick_var (NetServer *, c_char, char *, size_t);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "getnick",
  getnick_start,
  getnick_stop,
  getnick_conf,
  NULL
};

}

////////////////////
// module managing
////////////////////

// add a getnick to the list
static void
getnick_add (NetServer *s)
{
  getnick_list = new getnick_type (s, getnick_list);
}

// returns the getnick for a given server, NULL if nonexistant
static getnick_type *
server2getnick (NetServer *s)
{
  getnick_type *buf = getnick_list;
  while (buf != NULL)
    {
      if (buf->server == s)
        return buf;
      buf = buf->next;
    }
  return NULL;
}

// watch nick changes
static void
getnick_event (NetServer *s)
{
  if (EVENT != EVENT_NICK)
    return;
  getnick_type *getnick = server2getnick (s);
  if (getnick == NULL || !(bool)getnick->nick)
    return;
  mask2nick (CMD[0], BUF[0]);
  if (!(getnick->nick |= BUF[0]))
    return;
  s->irc_nick (BUF[0]);
  s->writeln ("%s", (c_char)getnick->cmd);
}

// configuration file's local parser
static void
getnick_conf (NetServer *s, c_char bufread)
{
  char buf[2][MSG_SIZE+1];

  strsplit (bufread, buf, 1);

  if (strcasecmp (buf[0], "bot") == 0)
    {
      if (server2getnick (s) == NULL)
        {
          getnick_add (s);
          s->script.events.add ((void *)getnick_event);
          s->vars.var_add ("getnick_nick", getnick_var);
          s->vars.var_add ("getnick_cmd", getnick_var);
        }
    }
}

// module termination
static void
getnick_stop (void)
{
  getnick_type *buf = getnick_list, *buf2;
  while (buf != NULL)
    {
      buf->server->script.events.del ((void *)getnick_event);
      buf->server->vars.var_del ("getnick_nick");
      buf->server->vars.var_del ("getnick_cmd");
      buf2 = buf->next;
      delete buf;
      buf = buf2;
    }
}

// module initialization
void
getnick_start (void)
{
  getnick_list = NULL;
}

static void
getnick_var (NetServer *s, c_char name, char *data, size_t n)
{
  getnick_type *buf = server2getnick (s);
  if (buf == NULL)
    return;
  if (strcasecmp (name, "getnick_nick") == 0)
    {
      if (n != 0)
        my_null_strncpy (data, buf->nick, n);
      else
        buf->nick = data;
    }
  else if (strcasecmp (name, "getnick_cmd") == 0)
    {
      if (n != 0)
        my_null_strncpy (data, buf->cmd, n);
      else
        buf->cmd = data;
    }
}

