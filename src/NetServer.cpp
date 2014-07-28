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

#include "NetServer.h"

NetServer::NetServer (void) : Net (), virtualhost (HOST_SIZE),
  nick (NICK_SIZE), nick_orig (NICK_SIZE), user (USER_SIZE),
  name (NAME_SIZE), away (AWAY_SIZE), quit (QUIT_SIZE), users (this),
  dcc_file (PATH_MAX), dcc_motdfile (PATH_MAX), vars (this), services (this),
  script (this), b (bot)
{
  hosts = host_current = NULL;
  last_try = 0;
  change_time = 0;
  time_now = bot->time_now + change_time;
  cmd_i = 0;
  int i;
  for (i = 0; i < CHANNEL_MAX; i++)
    channels[i] = NULL;
  channel_num = 0;
  for (i = 0; i < DCC_MAX; i++)
    dccs[i] = NULL;
  dcc_num = 0;
  dcc_port = 0;
  bufread[0] = 0;
  dcc_motd = NULL;
  bufpos = 0;
  bytesin = bytesout = 0;
  uptime = 0;
}

NetServer::~NetServer (void)
{
  irc_restart ();				// turn off networking
  if (dcc_motd != NULL)
    delete dcc_motd;
  for (int i = 0; i < channel_num; i++)
    delete channels[i];
}

void
NetServer::irc_pong (c_char pong)
{
  writeln ("PONG :%s", pong);
}

// if msg is NULL, use the default
void
NetServer::irc_quit (c_char msg)
{
  writeln ("QUIT :%s", msg == NULL ? quit : msg);
}

void
NetServer::irc_privmsg (c_char to, c_char format, ...)
{
  char bufprivmsg[MSG_SIZE+1];
  va_list args;
  va_start (args, format);
  vsnprintf (bufprivmsg, MSG_SIZE, format, args);
  va_end (args);
  writeln ("PRIVMSG %s :%s", to, bufprivmsg);
}

void
NetServer::irc_notice (c_char to, c_char format, ...)
{
  char bufnotice[MSG_SIZE+1];
  va_list args;
  va_start (args, format);
  vsnprintf (bufnotice, MSG_SIZE, format, args);
  va_end (args);
  writeln ("NOTICE %s :%s", to, bufnotice);
}

void
NetServer::irc_pass (c_char password)
{
  writeln ("PASS %s", password);
}

void
NetServer::irc_nick (c_char new_nick)
{
  writeln ("NICK %s", new_nick);
}

void
NetServer::irc_user (c_char new_user, c_char new_name)
{
  writeln ("USER %s %s %s :%s", new_user, (c_char)virtualhost != NULL ? (c_char)virtualhost : "0", (c_char)host_current->host, new_name);
}

void
NetServer::irc_whois (c_char what)
{
  writeln ("WHOIS %s", what);
}

void
NetServer::irc_ison (c_char nick_list)
{
  writeln ("ISON %s", nick_list);
}

void
NetServer::irc_watch (c_char nick, bool add)
{
  writeln ("WATCH %c%s", (add ? '+' : '-'), nick);
}

void
NetServer::irc_who (c_char what)
{
  writeln ("WHO %s", what);
}

void
NetServer::irc_away (c_char reason)
{
  writeln ("AWAY :%s", reason);
}

void
NetServer::irc_oper (c_char what_nick, c_char pass)
{
  writeln ("OPER %s %s", what_nick, pass);
}

void
NetServer::irc_kill (c_char what_nick, c_char reason)
{
  writeln ("KILL %s :%s", what_nick, reason);
}

void
NetServer::write_botlog (c_char format, ...)
{
  va_list args;
  va_start (args, format);
  char buf[MSG_SIZE+1];
  vsnprintf (buf, MSG_SIZE, format, args);
  va_end (args);
  char st_time[TIME_SIZE+1];
  get_asctime (bot->time_now, st_time, TIME_SIZE);
  cerr << st_time << " " << ((bool)nick ? (c_char)nick : "(no nick)") << " "
    << (host_current != NULL ? (c_char)host_current->host : "(no host)") << ": "
    << (buf[0] != 0 ? buf : "(no text)") << endl;
}

void
NetServer::irc_connect (void)
{
  if (host_current == NULL)
    {
      write_botlog ("no host defined, can't connect");
      mbot_exit ();
    }

  if (!connected && !connecting)
    {
      time_read = bot->time_now;
      if (difftime (time_now, last_try) < TIME_RETRY)
        return;
      last_try = time_now;
      write_botlog ("connecting to server");
      if (!resolvehost (host_current->host, &addr))
        {
          write_botlog ("error connecting to server: can't resolve host %s",
                        (c_char)host_current->host);
          host_current = host_current->next;
        }
      else if (!create_tcp (host_current->port, (c_char)virtualhost))
        write_botlog ("error connecting to server: failed socket(): %s", strerror (errno));
      return;
    }

  // connecting
  if (!openhost ())
    {
      write_botlog ("error connecting to server: %s", strerror (errno));
      host_current = host_current->next;
    }
  if (!connected)
    return;
  // we're connected
  uptime = time_now;
  if ((bool)host_current->password)
    irc_pass (host_current->password);
  irc_nick (nick);
  irc_user (user, name);
}

// close socket, dccs and delete users from channels
void
NetServer::irc_restart (void)
{
  closesock ();
  nick = nick_orig;
  services.exist = services.nickserv_pass;
  services.identified = 0;

  struct ListUsers::user_type *u;
  users.rewind ();
  while ((u = (struct ListUsers::user_type *)users.next ()) != NULL)
    u->id = false;

  for (int i = 0; i < channel_num; i++)
    {
      for (int i2 = 0; i2 < channels[i]->user_num; i2++)
        delete channels[i]->users[i2];
      channels[i]->user_num = 0;
      channels[i]->joined = 0;
    }

  for (int i = 0; i < dcc_num; i++)
    delete dccs[i];
  dcc_num = 0;

  bufread[0] = 0;
  bufpos = 0;
  bytesin = bytesout = 0;
}

void
NetServer::work (void)
{
  time_t time_last = time_now;
  time_now = bot->time_now + change_time;

  if (!connected)
    {
      irc_connect ();
      return;
    }

  int i = readln ();
  if (i == 1)			// there's a new msg to parse
    {
      irc_parse ();
      reply = atoi (cmd[1]);
      if (reply && strlen (cmd[1]) == 3)
        script.irc_reply ();
      else
        script.irc_event ();
    }
  else if (i == -1)			// error while reading
    {
      write_botlog ("ERROR reading from socket: %s", strerror (errno));
      irc_restart ();
      host_current = host_current->next;
      last_try = time_now;    // wait TIME_RETRY before connecting
      return;
    }
  // time_read uses global time, without change_time
  else if (difftime (bot->time_now, time_read) > TIME_STONED)
    {
      // prevent disconnecting when the system date changes
      if (difftime (time_now, time_last) > TIME_WARP)
        {
          time_read = time_now;
          return;
        }
      write_botlog ("ERROR: server timed out, disconnecting");
      irc_restart ();
      host_current = host_current->next;
      last_try = time_now;    // wait TIME_RETRY before connecting
      return;
    }

  // big mess with dcc's, gotta clean it up sometime
  for (i = 0; i < dcc_num; i++)
    if (dccs[i] != NULL
        && dccs[i]->dcc_work () == 0)	// inactive dcc, delete it
      {
        int i2;
        for (i2 = 0; i2 < dcc_num; i2++)	// delete dependencies
          {
            if (dccs[i2]->dcc_from_index == i)
              dccs[i2]->dcc_from_index = -1;
            if (dccs[i2]->dcc_from_index > i)
              dccs[i2]->dcc_from_index--;
          }
        delete dccs[i];
        for (i2 = i; i2 < dcc_num; i2++)
          {
            dccs[i2] = dccs[i2+1];
            if (dccs[i2] != NULL)
              dccs[i2]->index--;
          }
        dcc_num--;
      }

  // run the timers
  if (time_last != time_now)
    {
      void (*f)(NetServer *);
      script.timers.rewind ();
      while ((f = (void (*)(NetServer *))script.timers.next ()) != NULL)
        f (this);
    }

  // pass control to the workers
  void (*f)(NetServer *);
  works.rewind ();
  while ((f = (void (*)(NetServer *))works.next ()) != NULL)
    f (this);
}

// return 1 if ok, 0 if limit exceeded
bool
NetServer::channel_add (c_char chan_name, c_char chan_key)
{
  if (channel_num >= CHANNEL_MAX)
    return 0;
  channels[channel_num] = new Channel (chan_name, chan_key, this);
  channel_num++;
  return 1;
}

// return 1 if ok, 0 if nonexistent
bool
NetServer::channel_del (c_char chan_name)
{
  int i = channel_index (chan_name);
  if (i == -1)
    return 0;
  delete channels[i];
  for (; i+1 < channel_num; i++)
    channels[i] = channels[i+1];
  channel_num--;
  return 1;
}

// return channel's table index, -1 if nonexistent
int
NetServer::channel_index (c_char chan_name)
{
  for (int i = 0; i < channel_num; i++)
    if (channels[i] != NULL && (channels[i]->name |= chan_name))
      return i;
  return -1;
}

Channel *
NetServer::channel_get (c_char chan_name)
{
  for (int i = 0; i < channel_num; i++)
    if (channels[i] != NULL && (channels[i]->name |= chan_name))
      return channels[i];
  return NULL;
}

void
NetServer::irc_parse (void)
{
  size_t i = 0, i2 = 0;
  char *tmp = (char *)malloc(MSG_SIZE*sizeof(char));
  cmd_i = 0;
  memset (cmd, 0, sizeof (cmd));

  if (bufread[0] == ':') {
    strncpy (tmp, bufread + 1, MSG_SIZE);
    my_strncpy (bufread, tmp, MSG_SIZE);
    free(tmp);
  }

  size_t buflen = strlen (bufread);

  if (buflen != 0)
    while (bufread[i] != ':' && i < buflen && cmd_i < CMD_SIZE)
      {
        while (bufread[i] != ' ' && bufread[i] != '\n' && i != buflen)
          i++;
        my_strncpy (cmd[cmd_i], bufread + i2, i - i2);
        cmd_i++;
        i2 = ++i;
      }

  if (bufread[i] == ':')
    {
      i++;
      strncpy (cmd[cmd_i], bufread + i, strlen (bufread + i));
    }

}

// add a server to h_list, return this bot's server list (recursive function)
NetServer::host_type *
NetServer::host_add (host_type *h_list, c_char host, int port, c_char password)
{
  if (h_list == NULL)					// if it's the first
    {
      h_list = new host_type (host, port, password);
      host_current = h_list;
    }
  else if (h_list->next == host_current)		// if it's the last
    h_list->next = new host_type (host, port, password, host_current);
  else
    return host_add (h_list->next, host, port, password);	// try next one
  return h_list;
}

