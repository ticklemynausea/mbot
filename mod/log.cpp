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

class ModLog {
public:
  NetServer *s;
  String what;				// channel or nick to log

  ModLog (NetServer *server, c_char w, Log *l, bool lip);
  c_char log_mask (c_char);
  void log_event_join (void);
  void log_event_quit (c_char nick);
  void log_event_kick (void);
  void log_event_part (void);
  void log_event_topic (void);
  void log_event_nick (c_char oldnick);
  void log_event_mode (void);
  void log_event_privmsg_channel (void);
  void log_event_privmsg_pvt (void);
  void log_event_notice_channel (void);

private:
  Log *log;				// where to log
  bool logip;				// whether to log the ip
  char buf[MSG_SIZE+1];
};

List *logger_list;

///////////////
// prototypes
///////////////

static void log_add (NetServer *, c_char, Log *, bool);
static ModLog *server_what2logger (NetServer *, c_char);
static void log_event (NetServer *);
static void log_conf (NetServer *, c_char);
static void log_stop (void);
static void log_start (void);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "log",
  log_start,
  log_stop,
  log_conf,
  NULL
};

}

////////////////////
// class functions  
////////////////////

ModLog::ModLog (NetServer *server, c_char w, Log *l, bool lip) : s (server),
  what (w, CHANNEL_SIZE), log (l), logip (lip)
{
}

c_char
ModLog::log_mask (c_char mask)
{
  if (logip)
    return mask;
  char nick[NICK_SIZE+1], user[USER_SIZE+1];
  static char newmask[MASK_SIZE+1];
  mask2nick (mask, nick);
  mask2user (mask, user);
  snprintf (newmask, MASK_SIZE, "%s!%s@fakehost", nick, user);
  return newmask;
}

void
ModLog::log_event_join (void)
{
  *log << log_mask (CMD[0]) << " JOIN" << EOL;
}

void
ModLog::log_event_quit (c_char nick)
{
  *log << nick << " QUIT " << CMD[2] << EOL;
}

void
ModLog::log_event_kick (void)
{
  mask2nick (CMD[0], buf);
  *log << buf << " KICK " << CMD[3] << " - " << CMD[4] << EOL;
}

void
ModLog::log_event_part (void)
{
  mask2nick (CMD[0], buf);
  *log << buf << " PART" << EOL;
}

void
ModLog::log_event_topic (void)
{
  mask2nick (CMD[0], buf);
  *log << buf << " TOPIC " << CMD[3] << EOL;
}

void
ModLog::log_event_nick (c_char oldnick)
{
  *log << oldnick << " NICK " << CMD[2] << EOL;
}

void
ModLog::log_event_mode (void)
{
  char buf2[MSG_SIZE+1], buf3[MSG_SIZE+1];
  int num_modes;
  mask2nick (CMD[0], buf);
  snprintf (buf2, MSG_SIZE, "%s MODE %s", buf, CMD[3]);

  // add the nick(s) to the end of buf2
  for (int i = num_modes = 0; CMD[3][i] != 0; i++)
    if (CMD[3][i] != '+' && CMD[3][i] != '-')
      {
        // careful with the indexes
        if (++num_modes >= CMD_SIZE)
          break;
        snprintf (buf3, MSG_SIZE, "%s %s", buf2,
                  CMD[3+num_modes]);
        my_strncpy (buf2, buf3, MSG_SIZE);
      }
  *log << buf2 << EOL;
}

void
ModLog::log_event_privmsg_channel (void)
{
  *log << '<' << SOURCE << "> " << CMD[3] << EOL;
}

void
ModLog::log_event_privmsg_pvt (void)
{
  *log << CMD[0] << ' ' << CMD[3] << EOL;
}

void
ModLog::log_event_notice_channel (void)
{
  *log << '(' << SOURCE << ") " << CMD[3] << EOL;
}

////////////////////
// module managing
////////////////////

// add a server to the logs list
static void
log_add (NetServer *s, c_char what, Log *l, bool lip)
{
  ModLog *logger = new ModLog (s, what, l, lip);
  if (logger == NULL)
    s->b->conf_error ("error initializing log");
  logger_list->add ((void *)logger);
}

// return the logger for a given server/destination, NULL if nonexistant
static ModLog *
server_what2logger (NetServer *s, c_char what)
{
  ModLog *logger;
  logger_list->rewind ();
  while ((logger = (ModLog *)logger_list->next ()) != NULL)
    if (logger->s == s && (logger->what |= what))
      return logger;
  return NULL;
}

// events watcher, to log most of them
static void
log_event (NetServer *s)
{
  ModLog *logger;
  char buf[MSG_SIZE+1];

  switch (EVENT)
    {
      case EVENT_QUIT:
        mask2nick (CMD[0], buf);
        for (int i = 0; i < s->channel_num; i++)	// search all channels
          if (CHANNELS[i]->last_deleted_nick |= buf)	// if it was here
            {
              logger = server_what2logger (s, CHANNELS[i]->name);
              if (logger != NULL)
                logger->log_event_quit (buf);
            }
        break;

      case EVENT_NICK:
        mask2nick (CMD[0], buf);
        for (int i = 0; i < s->channel_num; i++)	// search all channels
          if (CHANNELS[i]->last_deleted_nick |= buf)	// if it was here
            {
              logger = server_what2logger (s, CHANNELS[i]->name);
              if (logger != NULL)
                logger->log_event_nick (buf);
            }
        break;

      case EVENT_JOIN:
      case EVENT_KICK:
      case EVENT_PART:
      case EVENT_TOPIC:
      case EVENT_MODE:
        logger = server_what2logger (s, CMD[2]);
        if (logger != NULL)
          switch (EVENT)
            {
              case EVENT_JOIN:
                logger->log_event_join ();
                break;
              case EVENT_KICK:
                logger->log_event_kick ();
                break;
              case EVENT_PART:
                logger->log_event_part ();
                break;
              case EVENT_TOPIC:
                logger->log_event_topic ();
                break;
              case EVENT_MODE:
                logger->log_event_mode ();
                break;
            }
        break;

      case EVENT_PRIVMSG:
        if (CMD[2][0] == '#')			// if it's a channel
          {
            logger = server_what2logger (s, CMD[2]);
            if (logger != NULL)
              logger->log_event_privmsg_channel ();
          }
        else					// pvt with the bot
          {
            logger = server_what2logger (s, NULL);	// logs them all
            if (logger != NULL)
              logger->log_event_privmsg_pvt ();
            logger = server_what2logger (s, SOURCE);	// specific pvt
            if (logger != NULL)
              logger->log_event_privmsg_pvt ();
          }
        break;

      case EVENT_NOTICE:
        if (CMD[2][0] == '#')			// if it's a channel
          {
            logger = server_what2logger (s, CMD[2]);
            if (logger != NULL)
              logger->log_event_notice_channel ();
          }
        /*
          i have chosen not to log notices to pvt because /onotice, /wall,
          etc use notices and the bot might receive and put them in a public
          log file... which is probably a bad idea
        */
        break;
    }
}

// configuration file's local parser
static void
log_conf (NetServer *s, c_char bufread)
{
  char buf[5][MSG_SIZE+1];

  strsplit (bufread, buf, 4);

  if (strcasecmp (buf[0], "log") == 0)
    {
      Log *log = s->b->log_get (buf[1]);
      if (log == NULL)
        {
          snprintf (buf[0], MSG_SIZE, "inexistant loghandle: %s", buf[1]);
          s->b->conf_error (buf[0]);
        }

      bool first = 1;	// if it's the first log for this server
      ModLog *l;
      logger_list->rewind ();
      while ((l = (ModLog *)logger_list->next ()) != NULL)
        if (l->s == s)
          {
            first = 0;
            break;
          }
      if (first)
        s->script.events.add ((void *)log_event);

      bool logip = 1;
      if (buf[3][0] != 0)
        {
          if (strcmp (buf[3], "0") == 0)
            logip = 0;
          else if (strcmp (buf[3], "1") != 0)
            s->b->conf_error ("this parameter must be \"1\" to log real IPs or \"0\" to use fake ones");
        }
      

      if (strcasecmp (buf[2], "<privmsg>") == 0)	//log all pvts
        log_add (s, NULL, log, logip);
      else
        log_add (s, buf[2], log, logip);
    }
}

// module termination
static void
log_stop (void)
{
  ModLog *logger;
  logger_list->rewind ();
  while ((logger = (ModLog *)logger_list->next ()) != NULL)
    {
      logger->s->script.events.del ((void *)log_event);
      delete logger;
    }
  delete logger_list;
}

// module initialization
static void
log_start (void)
{
  logger_list = new List ();
}

