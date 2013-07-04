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

#define DEFAULT_REFRESH 60		// refresh the pages each 60 seconds

#define DEST s->script.dest
#define BUF s->script.buf

struct chanstat_type {
  NetServer *server;
  String channel;
  String file_html;
  String file_stat;
  bool show_addr;
  time_t last_time;
  time_t refresh;
  struct top10_type {
    String nick;
    time_t uptime;
    top10_type () : nick (NICK_SIZE) {}
  } top10[10];
  int usersperday;	// number of users who entered the channel today
  int currentusers;		// number of users currently in the channel
  time_t time_top_currentusers;	// when 
  chanstat_type *next;
  chanstat_type (NetServer *s, c_char c, c_char fh, c_char fs, bool sa,
    chanstat_type *n) : server (s), channel (c, CHANNEL_SIZE),
    file_html (fh, PATH_MAX), file_stat (fs, PATH_MAX), show_addr (sa), 
    last_time (0), refresh (DEFAULT_REFRESH), next (n) {}
};
struct chanstat_type *chanstat_list;

///////////////
// prototypes
///////////////

static void chanstat_work (chanstat_type *);
static void chanstat_add (NetServer *, c_char, c_char, c_char, c_char);
static chanstat_type *server2chanstat (NetServer *);
static void chanstat_event (NetServer *);
static void chanstat_conf (NetServer *, c_char);
static void chanstat_stop (void);
static void chanstat_start (void);
static void chanstat_var (NetServer *, c_char, char *, size_t);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "chanstat",
  chanstat_start,
  chanstat_stop,
  chanstat_conf,
  NULL
};

}
////////////////////
// create the html
////////////////////

static
void chanstat_work (chanstat_type *c)
{
  ofstream f ((c_char)c->file_html);
  if (!f)
    return;
  #define _BUF_SIZE 100
  char buf[_BUF_SIZE+1], buf2[_BUF_SIZE+1];

  f << "<META HTTP-EQUIV=\"refresh\" content=\"" << (int)c->refresh
    << "; url=" << fullpath2file (c->file_html) << "\">\n";
  get_asctime (c->server->time_now, buf, _BUF_SIZE);
  strip_crlf (buf);
  f << "<TITLE>" << c->channel << " IRC channel stats (written at " 
    << buf << ")</TITLE>\n";
  f << "<CENTER><FONT SIZE=\"6\">" << c->server->nick
    << "'s " << c->channel << " stats</FONT></CENTER>\n";
  int index = c->server->channel_index (c->channel);	// get the channel
  int inside = (index != -1);
  if (inside)
    inside = c->server->channels[index]->joined;

  if (inside)						// if inside it
    {
      Channel *chan=c->server->channels[index];

      if (chan->topic)
        f << "<CENTER><FONT SIZE=\"5\">Topic: " << chan->topic
          << "</FONT></CENTER><BR>\n";
      else
        f << "<CENTER><FONT SIZE=\"5\">No topic is set.</FONT></CENTER><BR>\n";

      f << "<CENTER><TABLE BORDER=3>\n"
        << "<TR><TD COLSPAN=3><CENTER><FONT SIZE=5>Channel " << c->channel
        << ", " << chan->user_num << " users</FONT></CENTER></TD></TR>\n"
        << "<TR><TD bgcolor=0000ff><FONT COLOR=ffff00><B>Nickname</B></td><td bgcolor=0000ff><FONT COLOR=ffff00><B>Ident</B></td><td bgcolor=0000ff><font color=ffff00><b>Address</B></td></tr>\n";

      int i;
      for (i = 0; i < chan->user_num; i++)		// list ops
        if (chan->users[i]->is_op)
          {
            mask2user (chan->users[i]->mask, buf);
            if (c->show_addr)
              mask2host (chan->users[i]->mask, buf2);
            else
              my_strncpy (buf2, "(hidden)", _BUF_SIZE);
            f << "<TR><TD><FONT COLOR=ff00ff>@</FONT>" << chan->users[i]->nick
              << "</TD><TD>" << buf << "</TD><TD>" << buf2 << "</TD></TR>\n";
          }
      for (i = 0; i < chan->user_num; i++)		// list voices
        if ((!chan->users[i]->is_op) && (chan->users[i]->is_voice))
          {
            mask2user(chan->users[i]->mask, buf);
            if (c->show_addr)
              mask2host(chan->users[i]->mask, buf2);
            else
              my_strncpy(buf2, "(hidden)", _BUF_SIZE);
            f << "<TR><TD><FONT COLOR=009900>+</FONT>" << chan->users[i]->nick
              << "</TD><TD>" << buf << "</TD><TD>" << buf2 << "</TD></TR>\n";
          }
      for (i = 0; i < chan->user_num; i++)	// list non-ops and non-voices
        if (!chan->users[i]->is_op && !chan->users[i]->is_voice)
          {
            mask2user (chan->users[i]->mask, buf);
            if (c->show_addr)
              mask2host (chan->users[i]->mask, buf2);
            else
              my_strncpy (buf2, "(hidden)", _BUF_SIZE);
            f << "<TR><TD>" << chan->users[i]->nick << "</TD><TD>" << buf
              << "</TD><TD>" << buf2 << "</TD></TR>\n";
          }
      f << "</TABLE>\n";

      // put here bans table

      f << "</CENTER>\n";

    }
  else
    {
      f << "<BR><FONT SIZE=5><B>I (" << c->server->nick
        << ") am currently not on " << c->channel
        << ", try again later.</B></FONT>\n";
    }
  
  f << "<BR>\n<HR>This page is updated every " << (int)c->refresh
    << " seconds.\n";
}

////////////////////
// module managing
////////////////////

// add a chanstat to the list
static void
chanstat_add (NetServer *s, c_char name, c_char f_html, c_char f_stat, c_char show)
{
  chanstat_type *buf = chanstat_list;
  chanstat_list = new chanstat_type (s, name, f_html, f_stat,
                                     !(show[0] == '0'), buf);
}

// return the chanstat for a given server, NULL if nonexistant
static chanstat_type *
server2chanstat (NetServer *s)
{
  chanstat_type *buf = chanstat_list;
  while (buf != NULL)
    {
      if (buf->server == s)
        return buf;
      buf = buf->next;
    }
  return NULL;
}

// check if it's time to update the file whenever an event occurs
static void
chanstat_event (NetServer *s)
{
  chanstat_type *c = server2chanstat (s);
  if (c != NULL)
    if (difftime (s->time_now, c->last_time) > c->refresh)
      {
        chanstat_work (c);
        c->last_time = s->time_now;
      }
}

// configuration file's local parser
static void
chanstat_conf (NetServer *s, c_char bufread)
{
  char buf[6][MSG_SIZE+1];

  strsplit (bufread, buf, 5);

  if (strcasecmp (buf[0], "chanstat") != 0)
    return;
  if (server2chanstat (s) != NULL)
    {
      s->b->conf_error ("chanstat already defined for this server");
      return;
    }
  // 5th parameter is optional for backwards compatibility
  if (buf[3][0] != 0)
    {
      chanstat_add (s, buf[1], buf[2], buf[3], buf[4]);
      s->script.events.add ((void *)chanstat_event);
    }
  else
    s->b->conf_error ("required parameter missing for chanstat");
  s->vars.var_add ("chanstat_refresh", chanstat_var);
}

// module termination
static void
chanstat_stop (void)
{
  chanstat_type *buf = chanstat_list, *buf2;
  while (buf != NULL)
    {
      buf->server->script.events.del ((void *)chanstat_event);
      buf->server->vars.var_del ("chanstat_refresh");
      buf2 = buf->next;
      delete buf;
      buf = buf2;
    }
}

// module initialization
static void
chanstat_start (void)
{
  chanstat_list = NULL;
}

static void
chanstat_var (NetServer *s, c_char name, char *data, size_t n)
{
  if (strcasecmp (name, "chanstat_refresh") == 0)
    {
      chanstat_type *buf = server2chanstat (s);
      if (buf == NULL)
        return;
      if (n != 0)
        ltoa (buf->refresh, data, n);
      else
        buf->refresh = atol (data);
    }
}

