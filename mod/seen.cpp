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

#define LEVEL_SEEN 0

#define HELP_SEEN "!seen nick - Show when <nick> was last seen on irc."

#define SEEN_MAX 2000		// maximum nicks on seen list at the same time

#define TIMESTR_SIZE 10
#define FIELD_SIZE (NICK_SIZE+TIMESTR_SIZE+1)

#define DEST s->script.dest
#define BUF s->script.buf
#define SEND_TEXT s->script.send_text

/////////////////////
// class definition
/////////////////////

class ModSeen {
public:
  ModSeen (NetServer *, c_char);

  int seen_index (c_char);
  time_t seen_time (c_char);
  void update_seen (c_char);

  String filename;			// file where the database is
  struct seen_type {
    String nick;			// nick
    time_t time;			// when was it last seen
    seen_type () : nick (NICK_SIZE) {}
  } seen[SEEN_MAX+1];
  int seen_num;				// number of nicks

  bool initialized;
  NetServer *s;				// server to which belongs

private:

  void setup_seen (void);
  void change_seen (int);
  void write_seen (c_char, int);

  fstream f;
  char buf[BUF_SIZE+1];
};

List *seen_list;

///////////////
// prototypes
///////////////

extern "C" {

static void seen_cmd_seen (NetServer *);

time_t server2seentime (NetServer *, c_char);	// used by alice.cpp
static ModSeen *server2seen (NetServer *);
static void seen_event (NetServer *);
static void seen_conf (NetServer *, c_char);
static void seen_stop (void);
static void seen_start (void);

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "seen",
  seen_start,
  seen_stop,
  seen_conf,
  NULL
};

}

////////////////////
// class functions
////////////////////

ModSeen::ModSeen (NetServer *server, c_char name)
{
  initialized = 0;
  s = server;
  filename = name;
  seen_num = 0;
  for (int i = 0; i < SEEN_MAX+1; i++)
    seen[i].nick = NULL;
#ifdef HAS_IOS_BINARY
  f.open (filename, ios::in | ios::out | ios::binary);
#else
  f.open (filename, ios::in | ios::out);
#endif
  if (!f)
    {
      s->write_botlog ("error opening %s: %s\n", (c_char)filename, strerror (errno));
      return;
    }
  setup_seen ();
  initialized = 1;
}

// load file to memory
void
ModSeen::setup_seen (void)
{
  seen_num = 0;
  char buf[FIELD_SIZE+1];
  while (f.getline (buf, FIELD_SIZE+1))
    {
      seen[seen_num].time = atol (buf + num_notspaces (buf));
      buf[num_notspaces (buf)] = 0;
      seen[seen_num].nick = buf;
      if (seen_num == SEEN_MAX)
        {
          s->write_botlog ("error reading %s: too many entries (probably you must delete the file)",
                           (c_char)filename);
          mbot_exit ();
        }
      seen_num++;
    }
  f.clear ();
}

// table index for <nick>, -1 if nonexistent
int
ModSeen::seen_index (c_char nick)
{
  for (int i = 0; i < seen_num; i++)
    if (seen[i].nick |= nick)
      return i;
  return -1;
}

// return time for nick, -1 if nonexistant
time_t
ModSeen::seen_time (c_char nick)
{
  int i = seen_index (nick);
  if (i != -1)
    return seen[i].time;
  else
    return -1;
}

// update time for position <i>
void
ModSeen::change_seen (int i)
{
  seen[i].time = s->time_now;
  f.seekp (FIELD_SIZE*i + NICK_SIZE);
  f << setfill ('0') << setw (TIMESTR_SIZE) << resetiosflags (ios::left)
    << seen[i].time;
}

// write nick to file and memory, with the current time, at position i
void
ModSeen::write_seen (c_char nick, int i)
{
  seen[i].nick = nick;
  seen[i].time = s->time_now;
  f.seekp (FIELD_SIZE*i);
  f << setfill (' ') << setw (NICK_SIZE) << setiosflags (ios::left)
    << seen[i].nick << setfill ('0') << setw (TIMESTR_SIZE)
    << resetiosflags (ios::left) << seen[i].time << '\n';
}

// update <nick>, change time or add it if nonexistant, possibly overwriting another
void
ModSeen::update_seen (c_char nick)
{
  if (strncmp (nick, "Guest", 5) == 0)	// ignore PTnet's "Guest*" nicks
    return;
  int i = seen_index (nick);
  if (i != -1)					// doesn't exist
    {
      change_seen (i);				// change time
      return;
    }
  bool added;
  if (seen_num == SEEN_MAX)			// if on the limit
    {
      time_t t = s->time_now;
      i = 0;
      for (int i2 = 0; i2 < seen_num; i2++)	// seach the oldest
        if (seen[i2].time < t)
          {
            i = i2;
            t = seen[i2].time;
          }
      added = 0;
    }
  else						// else add to the end
    {
      added = 1;
      i = seen_num;
    }
  write_seen (nick, i);				// write
  if (added)
    seen_num++;
}

/////////////
// commands
/////////////

extern "C" {

// !seen nick
static void
seen_cmd_seen (NetServer *s)
{
  ModSeen *seen = server2seen (s);
  if (seen == NULL)
    {
      SEND_TEXT (DEST, "This command is not available.");
      return;
    }
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_SEEN);
      return;
    }
  for (int i = 0; i < s->channel_num; i++)	// search in channels
    if (CHANNELS[i]->user_index (BUF[1]) != -1)
      {
        SEND_TEXT (DEST, "%s is currently online.", BUF[1]);
        return;
      }
  time_t t = seen->seen_time (BUF[1]);		// search in seen
  if (t == -1)
    {
      SEND_TEXT (DEST, "Humm, I don't remember that nick.");
      return;
    }
  my_strncpy (BUF[2], asctime (localtime (&t)), MSG_SIZE);
  BUF[2][strlen (BUF[2]) - 1] = 0;
  SEND_TEXT (DEST, "I last saw %s on %s.", BUF[1], BUF[2]);
}

////////////////////
// module managing
////////////////////

time_t
server2seentime (NetServer *s, c_char nick)
{
  ModSeen *seen = server2seen (s);
  if (seen == NULL)
    return -1;
  return seen->seen_time (nick);
}

// return the seen for a given server, NULL if nonexistant
static ModSeen *
server2seen (NetServer *s)
{
  ModSeen *a;
  seen_list->rewind ();
  while ((a = (ModSeen *)seen_list->next ()) != NULL)
    if (a->s == s)
      return a;
  return NULL;
}

// look for join, quit, kick, part and nick to update seen database
static void
seen_event (NetServer *s)
{
  switch (EVENT)
    {
      case EVENT_JOIN:
      case EVENT_QUIT:
      case EVENT_KICK:
      case EVENT_PART:
      case EVENT_NICK:
        ModSeen *seen = server2seen (s);
        if (seen == NULL)
          return;
        mask2nick (CMD[0], BUF[0]);
        seen->update_seen (BUF[0]);
        if (EVENT == EVENT_NICK)
          seen->update_seen (CMD[2]);
        break;
    }
}

// look for who replies to update seen database
static void
seen_reply (NetServer *s)
{
  if (REPLY == RPL_WHOREPLY)
    {
      ModSeen *seen = server2seen (s);
      if (seen != NULL)
        seen->update_seen (CMD[7]);
    }
}

// configuration file's local parser
static void
seen_conf (NetServer *s, c_char bufread)
{
  char buf[3][MSG_SIZE+1];

  strsplit (bufread, buf, 2);

  if (strcasecmp (buf[0], "seen") == 0)
    {
      if (buf[1][0] == 0)
        s->b->conf_error ("sintax error: use \"seen seenfile\"");
      ModSeen *seen = server2seen (s);
      if (seen != NULL)
        s->b->conf_error ("seen already defined in this server");
      seen = new ModSeen (s, buf[1]);
      if (seen == NULL || !seen->initialized)
        s->b->conf_error ("error initializing seen");
      seen_list->add ((void *)seen);
      s->script.events.add ((void *)seen_event);
      s->script.replies.add ((void *)seen_reply);
      s->script.cmd_bind (seen_cmd_seen, LEVEL_SEEN, "!seen", module.mod, HELP_SEEN);
    }

}

// module termination
static void
seen_stop (void)
{
  ModSeen *seen;
  seen_list->rewind ();
  while ((seen = (ModSeen *)seen_list->next ()) != NULL)
    {
      seen->s->script.events.del ((void *)seen_event);
      seen->s->script.replies.del ((void *)seen_reply);
      delete seen;
    }
  delete seen_list;
}

// module initialization
static void
seen_start (void)
{
  seen_list = new List ();
}

}
