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

struct watch_type {
  NetServer *s;
  String nick;
  Log &log;
  watch_type (NetServer *server, c_char n, Log &l) : s (server),
    nick (n, NICK_SIZE), log (l) {}
};
List *watch_list;

///////////////
// prototypes
///////////////

static watch_type *server_nick2watch (NetServer *, c_char);
static void watch_reply (NetServer *);
static void watch_conf (NetServer *, c_char);
static void watch_stop (void);
static void watch_start (void);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "watch",
  watch_start,
  watch_stop,
  watch_conf,
  NULL
};

}

////////////////////
// module managing
////////////////////

// return the watch for a given server/nick, NULL if nonexistant
static
watch_type *server_nick2watch (NetServer *s, c_char nick)
{
  watch_type *watch;
  watch_list->rewind ();
  while ((watch = (watch_type *)watch_list->next ()) != NULL)
    if (watch->s == s && (watch->nick |= nick))
      return watch;
  return NULL;
}

static void
watch_reply (NetServer *s)
{
  watch_type *watch;

  // init
  if (REPLY == RPL_ISUPPORT)		// just assume watch is available
    {
      watch_list->rewind ();
      while ((watch = (watch_type *)watch_list->next ()) != NULL)
        if (watch->s == s)
          s->irc_watch (watch->nick, 1);
    }

  watch = server_nick2watch (s, CMD[3]);
  if (watch == NULL)
    return;

  switch (REPLY)
    {
      // watch
      case RPL_WATCHONLINE:
      case RPL_WATCHSTARTONLINE:
        watch->log << "Signon " << CMD[3] << '!' << CMD[4] << '@' << CMD[5] << EOL;
        s->irc_whois (CMD[3]);
        break;
      case RPL_WATCHOFFLINE:
        watch->log << "Signoff " << CMD[3] << '!' << CMD[4] << '@' << CMD[5] << EOL;
        break;
      case RPL_WATCHSTOP:
        s->irc_watch (CMD[3], 1);
        break;

      // whois
      case RPL_AWAY:
        watch->log << CMD[3] << " :" << CMD[4] << EOL;
        break;
      case RPL_WHOISUSER:
        watch->log << CMD[3] << ' ' << CMD[4] << ' ' << CMD[5] << ' ' << CMD[6]
          << " :" << CMD[7] << EOL;
        break;
      case RPL_WHOISSERVER:
        watch->log << CMD[3] << ' ' << CMD[4] << " :" << CMD[5] << EOL;
        break;
      case RPL_WHOISOPERATOR:
        watch->log << CMD[3] << " :" << CMD[4] << EOL;
        break;
      case RPL_WHOISIDLE:
        watch->log << CMD[3] << ' ' << CMD[4] << ' '<< CMD[5] << " :" << CMD[6] << EOL;
        break;
      case RPL_WHOISCHANNELS:
        watch->log << CMD[3] << " :" << CMD[4] << EOL;
        break;
      default:
        break;
    }
}

// configuration file's local parser
static void
watch_conf (NetServer *s, c_char bufread)
{
  char buf[7][MSG_SIZE+1];

  strsplit (bufread, buf, 3);

  if (strcasecmp (buf[0], "watch") != 0)
    return;

  if (buf[2] == 0)
    s->b->conf_error ("sintax error: use \"watch loghandle nick\"");
  Log *log = s->b->log_get (buf[1]);
  if (log == NULL)
    {
      snprintf (buf[0], MSG_SIZE, "inexistant loghandle: %s", buf[1]);
      s->b->conf_error (buf[0]);
    }
  bool first = 1;	// if it's the first log for this server
  watch_type *watch;
  watch_list->rewind ();
  while ((watch = (watch_type *)watch_list->next ()) != NULL)
    {
      if (watch->nick |= buf[2] && watch->s == s)
        {
          snprintf (buf[0], MSG_SIZE, "watch already defined for nick '%s' in this server", buf[2]);
          s->b->conf_error (buf[0]);
        }
      if (watch->s == s)
        first = 0;
    }
  watch = new watch_type (s, buf[2], *log);
  if (watch == NULL)
    s->b->conf_error ("error initializing watch");
  watch_list->add ((void *)watch);
  if (first)
    s->script.replies.add ((void *)watch_reply);
}

// module termination
static void
watch_stop (void)
{
  watch_type *watch;
  watch_list->rewind ();
  while ((watch = (watch_type *)watch_list->next ()) != NULL)
    {
      watch->s->script.replies.del ((void *)watch_reply);
      delete watch;
    }
  delete watch_list;
}

// module initialization
static void
watch_start (void)
{
  watch_list = new List ();
}

