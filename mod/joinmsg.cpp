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

#define DEST s->script.dest
#define BUF s->script.buf

struct joinmsg_type {
  NetServer *s;
  String channel;
  String filename;
  Text text;
  joinmsg_type (NetServer *server, c_char c, c_char f) : s (server),
    channel (c, CHANNEL_SIZE), filename (f, PATH_MAX) {}
};
List *joinmsg_list;

///////////////
// prototypes
///////////////

static void joinmsg_add (NetServer *, c_char, c_char);
static joinmsg_type *server2joinmsg (NetServer *, c_char);
static void joinmsg_event (NetServer *);
static void joinmsg_conf (NetServer *, c_char);
static void joinmsg_stop (void);
static void joinmsg_start (void);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "joinmsg",
  joinmsg_start,
  joinmsg_stop,
  joinmsg_conf,
  NULL
};

}

////////////////////
// module managing
////////////////////

// add a joinmsg to the list
static void
joinmsg_add (NetServer *s, c_char name, c_char file)
{
  joinmsg_type *joinmsg;
  joinmsg = new joinmsg_type (s, name, file);
  if (s == NULL)
    s->b->conf_error ("error initializing joinmsg");
  joinmsg_list->add ((void *)joinmsg);
  if (!joinmsg->text.read_file (file))
    {
      s->write_botlog ("error opening %s: %s", file, strerror (errno));
      mbot_exit ();
    }
}

// return the joinmsg for a given server/channel, NULL if nonexistant
static joinmsg_type *
server2joinmsg (NetServer *s, c_char chan)
{
  joinmsg_type *joinmsg;
  joinmsg_list->rewind ();
  while ((joinmsg = (joinmsg_type *)joinmsg_list->next ()) != NULL)
    if (joinmsg->s == s && (joinmsg->channel |= chan))
      return joinmsg;
  return NULL;
}

// check for joins, to send the file
static void
joinmsg_event (NetServer *s)
{
  if (EVENT != EVENT_JOIN)
    return;

  joinmsg_type *c = server2joinmsg (s, CMD[2]);
  if (c == NULL)
    return;
  mask2nick (CMD[0], BUF[0]);
  if (strcasecmp (CMD[2], c->channel) == 0
      && strcasecmp (BUF[0], s->nick) != 0 // don't send to the bot
      && USERS.match_mask_reallevel (CMD[0]) >= 0)
    {
      c->text.rewind_text ();
      for (int i = c->text.line_num; i > 0; i--)
        s->irc_notice (BUF[0], c->text.get_line ());
    }
}

// configuration file's local parser
static void
joinmsg_conf (NetServer *s, c_char bufread)
{
  char buf[4][MSG_SIZE+1];

  strsplit (bufread, buf, 3);

  if (strcasecmp (buf[0], "bot") == 0)
    s->script.events.add ((void *)joinmsg_event);

  else if (strcasecmp (buf[0], "joinmsg") == 0)
    {
      if (server2joinmsg (s, buf[1]) == NULL)
        joinmsg_add (s, buf[1], buf[2]);
      else
        s->b->conf_error ("joinmsg already defined for that channel.");
    }
}

// module termination
static void
joinmsg_stop (void)
{
  joinmsg_type *joinmsg;
  joinmsg_list->rewind ();
  while ((joinmsg = (joinmsg_type *)joinmsg_list->next ()) != NULL)
    {
      joinmsg->s->script.events.del ((void *)joinmsg_event);
      delete joinmsg;
    }
  delete joinmsg_list;
}

// module initialization
static void
joinmsg_start (void)
{
  joinmsg_list = new List;
}

