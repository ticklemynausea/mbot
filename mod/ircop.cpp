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

#define LEVEL_KILL 9

#define HELP_KILL "!kill nick reason - Kills <nick> with <reason>."

#define IRCOP_PASS_SIZE MSG_SIZE

#define DEST s->script.dest
#define BUF s->script.buf
#define SEND_TEXT s->script.send_text

struct ircop_type {
  NetServer *s;
  String ircop_pass;			// ircop password
  bool ircop;				// if it is ircop
  ircop_type (NetServer *server, c_char p) : s (server),
    ircop_pass (IRCOP_PASS_SIZE), ircop (0) {}
};
List *ircop_list;

///////////////
// prototypes
///////////////

static void kill_cmd_kill (NetServer *);

static ircop_type *server2ircop (NetServer *);
static void ircop_reply (NetServer *);
static void ircop_event (NetServer *);
static void ircop_conf (NetServer *, c_char);
static void ircop_stop (void);
static void ircop_start (void);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "ircop",
  ircop_start,
  ircop_stop,
  ircop_conf,
  NULL
};

}

/////////////
// commands
/////////////

// !kill nick reason
static void
kill_cmd_kill (NetServer *s)
{
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_KILL);
      return;
    }
  if (s->nick |= BUF[1])		// doesn't kill itself
    SEND_TEXT (DEST, "%s", SCRIPT_HURT_BOT);
  else
    s->irc_kill (BUF[1], BUF[2]);
}

////////////////////
// module managing
////////////////////

// return the ircop for a given server, NULL if nonexistant
static ircop_type *
server2ircop (NetServer *s)
{
  ircop_type *ircop;
  ircop_list->rewind ();
  while ((ircop = (ircop_type *)ircop_list->next ()) != NULL)
    if (ircop->s == s)
      return ircop;
  return NULL;
}

// look for the first server init reply, to identify as ircop
static void
ircop_reply (NetServer *s)
{
  if (REPLY == INIT)
    {
      ircop_type *ircop = server2ircop (s);
      if (ircop != NULL)
        if (ircop->ircop_pass[0] != 0 && !ircop->ircop)
          {
            s->irc_oper (s->nick, ircop->ircop_pass);
            ircop->ircop = 1;
          }
    }
}

// look for the first server ping, to identify as ircop
static void
ircop_event (NetServer *s)
{
  if (EVENT == EVENT_PING)
    {
      ircop_type *ircop = server2ircop (s);
      if (ircop != NULL)
        if (ircop->ircop_pass[0] && !ircop->ircop)
          {
            s->irc_oper (s->nick, ircop->ircop_pass);
            ircop->ircop = 1;
          }
    }
}

// configuration file's local parser
static void
ircop_conf (NetServer *s, c_char bufread)
{
  char buf[3][MSG_SIZE+1];

  strsplit (bufread, buf, 2);

  if (strcasecmp (buf[0], "ircop") == 0)
    {
      if (server2ircop (s) != NULL)
        {
          s->b->conf_error ("ircop already defined for this server");
          return;
        }
      ircop_type *ircop = new ircop_type (s, buf[1]);
      if (ircop == NULL)
        s->b->conf_error ("error initializing ircop");
      ircop_list->add ((void *)ircop);
      s->script.events.add ((void *)ircop_event);
      s->script.replies.add ((void *)ircop_reply);
      s->script.cmd_bind (kill_cmd_kill, LEVEL_KILL, "!kill", module.mod, HELP_KILL);
    }
}

// module termination
static void
ircop_stop (void)
{
  ircop_type *ircop;
  ircop_list->rewind ();
  while ((ircop = (ircop_type *)ircop_list->next ()) != NULL)
    {
      ircop->s->script.events.del ((void *)ircop_event);
      ircop->s->script.replies.del ((void *)ircop_reply);
      delete ircop;
    }
  delete ircop_list;
}

// module initialization
static void
ircop_start (void)
{
  ircop_list = new List ();
}

