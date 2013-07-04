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

#define FLOOD_MSG 3			// equal lines with the same mask
#define FLOOD_MASK 10			// lines with the same mask
#define FLOOD_WARN 0	// lines with the same mask, after reaching the others, to kikar
		// if !0, sends flood_kickmsg to pvt, on the first time

#define SOURCE s->script.source
#define DEST s->script.dest
#define BUF s->script.buf
#define SEND_TEXT s->script.send_text

struct flood_type {
  NetServer *server;
  u_int flood_maxmsg;
  u_int flood_maxmask;
  u_int flood_maxwarn;
  String flood_kickmsg;
  flood_type *next;
  flood_type (NetServer *s, flood_type *n) : server (s),
    flood_maxmsg (FLOOD_MSG), flood_maxmask (FLOOD_MASK),
    flood_maxwarn (FLOOD_WARN), flood_kickmsg (KICK_SIZE), next (n) {}
};
struct flood_type *flood_list;

///////////////
// prototypes
///////////////

static void flood_add (NetServer *);
static flood_type *server2flood (NetServer *);
static void flood_event (NetServer *);
static void flood_conf (NetServer *, c_char);
static void flood_stop (void);
static void flood_start (void);
static void flood_var (NetServer *, c_char, char *, size_t);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "flood",
  flood_start,
  flood_stop,
  flood_conf,
  NULL
};

}

////////////////////
// module managing
////////////////////

// add a flood to the list
static void
flood_add (NetServer *s)
{
  flood_type *buf = flood_list;
  flood_list = new flood_type (s, buf);
}

// returns the flood for a given server, NULL if nonexistant
static flood_type *
server2flood (NetServer *s)
{
  flood_type *buf = flood_list;
  while (buf != NULL)
    {
      if (buf->server == s)
        return buf;
      buf = buf->next;
    }
  return NULL;
}

// watch privmsgs to keep flood status, and take action when necessary
static void
flood_event (NetServer *s)
{
  if (EVENT != EVENT_PRIVMSG || CMD[2][0] != '#')
    return;
  flood_type *flood = server2flood (s);
  if (flood == NULL)
    return;
  int reallevel = USERS.match_mask_reallevel (CMD[0]);
  int i = CHANNEL_INDEX (CMD[2]);
  if (i == -1)
    return;

  // different mask, reset flood status
  if (strcasecmp (CMD[0], CHANNELS[i]->last_mask) != 0)
    {
      my_strncpy (CHANNELS[i]->last_mask, CMD[0], MASK_SIZE);
      my_strncpy (CHANNELS[i]->last_msg, CMD[3], MSG_SIZE);
      CHANNELS[i]->mask_num = 1;
      CHANNELS[i]->msg_num = 1;
      CHANNELS[i]->warn_num = 0;
      return;
    }

  // reached maximum different msgs in a row by the same mask
  if (CHANNELS[i]->mask_num == flood->flood_maxmask)
    {
      if (reallevel < 4)
        {
          if (flood->flood_maxwarn != 0
              && CHANNELS[i]->warn_num != flood->flood_maxwarn)
            {
              if (CHANNELS[i]->warn_num == 0 && (bool)flood->flood_kickmsg)
                SEND_TEXT (SOURCE, "%s", (c_char)flood->flood_kickmsg);
              CHANNELS[i]->warn_num++;
              return;
            }
          CHANNELS[i]->irc_kick (SOURCE, flood->flood_kickmsg);
        }
      CHANNELS[i]->mask_num = 1;
      CHANNELS[i]->warn_num = 0;
      return;
    }

  // same mask and same msg
  if (strcasecmp (CMD[3], CHANNELS[i]->last_msg) == 0)
    {
      if (CHANNELS[i]->msg_num == flood->flood_maxmsg)
        {
          if (reallevel < 4)
            {
              if (flood->flood_maxwarn != 0
                  && CHANNELS[i]->warn_num != flood->flood_maxwarn)
                {
                  if (CHANNELS[i]->warn_num == 0 && (bool)flood->flood_kickmsg)
                    SEND_TEXT (SOURCE, "%s", (c_char)flood->flood_kickmsg);
                  CHANNELS[i]->warn_num++;
                  return;
                }
              CHANNELS[i]->irc_kick (SOURCE, flood->flood_kickmsg);
            }
          CHANNELS[i]->msg_num = 1;
          CHANNELS[i]->warn_num = 0;
        } 
      else 
        CHANNELS[i]->msg_num++;
    }
  else
    {
      my_strncpy (CHANNELS[i]->last_msg, CMD[3], MSG_SIZE);
      CHANNELS[i]->msg_num = 1;
      CHANNELS[i]->warn_num = 0;
    }
  CHANNELS[i]->mask_num++;
}

// configuration file's local parser
static void
flood_conf (NetServer *s, c_char bufread)
{
  char buf[2][MSG_SIZE+1];

  strsplit (bufread, buf, 1);

  if (strcasecmp (buf[0], "bot") == 0)
    {
      if (server2flood (s) == NULL)
        {
          flood_add (s);
          s->script.events.add ((void *)flood_event);
          s->vars.var_add ("flood_maxmsg", flood_var);
          s->vars.var_add ("flood_maxmask", flood_var);
          s->vars.var_add ("flood_maxwarn", flood_var);
          s->vars.var_add ("flood_kickmsg", flood_var);
        }
    }
}

// module termination
static void
flood_stop (void)
{
  flood_type *buf = flood_list, *buf2;
  while (buf != NULL)
    {
      buf->server->script.events.del ((void *)flood_event);
      buf->server->vars.var_del ("flood_maxmsg");
      buf->server->vars.var_del ("flood_maxmask");
      buf->server->vars.var_del ("flood_maxwarn");
      buf->server->vars.var_del ("flood_kickmsg");
      buf2 = buf->next;
      delete buf;
      buf = buf2;
    }
}

// module initialization
void
flood_start (void)
{
  flood_list = NULL;
}

static void
flood_var (NetServer *s, c_char name, char *data, size_t n)
{
  flood_type *buf = server2flood (s);
  if (buf == NULL)
    return;
  if (strcasecmp (name, "flood_maxmsg") == 0)
    {
      if (n != 0)
        itoa (buf->flood_maxmsg, data, n);
      else
        buf->flood_maxmsg = atoi (data);
    }
  else if (strcasecmp (name, "flood_maxmask") == 0)
    {
      if (n != 0)
        itoa (buf->flood_maxmask, data, n);
      else
        buf->flood_maxmask = atoi (data);
    }
  else if (strcasecmp (name, "flood_maxwarn") == 0)
    {
      if (n != 0)
        itoa (buf->flood_maxwarn, data, n);
      else
        buf->flood_maxwarn = atoi (data);
    }
  else if (strcasecmp (name, "flood_kickmsg") == 0)
    {
      if (n != 0)
        my_null_strncpy (data, buf->flood_kickmsg, n);
      else
        buf->flood_kickmsg = data;
    }
}

