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

#define LEVEL_WHOIS 0

#define HELP_WHOIS "!whois ip - Show info for <ip> using whois.ripe.net's whois server."

#define DEST s->script.dest
#define BUF s->script.buf
#define SEND_TEXT s->script.send_text

/////////////////////
// class definition
/////////////////////

class ModWhois {
public:
  ModWhois (NetServer *);

  bool start (c_char, c_char);
  void parse (void);
  void work (void);
  void stop (void);

  bool in_use;
  NetServer *s;			// server to which belongs
  bool whois_ripe;	// 0 - use normal whois program, 1 - use ripe whois

private:

  int pipes[2];
  pid_t pid;
  size_t linepos;
  String host, dest;
  char text[MSG_SIZE+1], buf[MSG_SIZE+1], line[MSG_SIZE+1];

};

List *whois_list;

///////////////
// prototypes
///////////////

static void whois_cmd_whois (NetServer *);

static ModWhois *server2whois (NetServer *);
static void whois_conf (NetServer *, c_char);
static void whois_stop (void);
static void whois_start (void);
static void whois_var (NetServer *, c_char, char *, size_t);
static void whois_work (NetServer *);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "whois",
  whois_start,
  whois_stop,
  whois_conf,
  NULL
};

}

////////////////////
// class functions
////////////////////

ModWhois::ModWhois (NetServer *server) : in_use (0), s (server), whois_ripe (0),
  host (HOST_SIZE), dest (CHANNEL_SIZE)
{
}

bool
ModWhois::start (c_char hostname, c_char destination)
{
#ifdef WINDOZE
  return 0;
#else	// !WINDOZE
  host = hostname;
  dest = destination;
  if (pipe (pipes) == -1)
    {
      perror ("pipe()");
      return 0;
    }
  pid = fork ();
  switch (pid)
    {
      case -1:
        close (pipes[0]);
        close (pipes[1]);
        return 0;
      case 0:
        dup2 (pipes[1], 1);
        dup2 (pipes[1], 2);
#ifdef WHOIS_PATH
        if (whois_ripe)
          execl (WHOIS_PATH, "whois", (c_char)host, "-h", "whois.ripe.net", NULL);
        else
          {
            snprintf (buf, MSG_SIZE, "-a %s@whois.ripe.net", (c_char)host);
            execl (WHOIS_PATH, "whois", buf, NULL);
          }
        s->write_botlog ("error executing %s: %s", WHOIS_PATH, strerror (errno));
#endif	// WHOIS_PATH
        exit (0);
    }
  close (pipes[1]);
  s->works.add ((void *)whois_work);
  in_use = 1;
  text[0] = 0;
  linepos = 0;
  return 1;
#endif	// !WINDOZE
}

void
ModWhois::parse (void)
{
  char buf2[2][MSG_SIZE+1], line[MSG_SIZE+1];
  for (size_t i = 0; buf[i] != 0; i++)
    {
      if (linepos < MSG_SIZE)
        line[linepos++] = buf[i];
      if (buf[i] == '\n' || linepos == MSG_SIZE)
        {
          line[linepos] = 0;
          linepos = 0;
          strip_crlf (line);
          strsplit (line, buf2, 1);
          if (strcasecmp (buf2[0], "descr:") == 0
              || strcasecmp (buf2[0], "country:") == 0
              || strcasecmp (buf2[0], "person:") == 0)
            {
              if (text[0] == 0)
                my_strncpy (text, buf2[1], MSG_SIZE);
              else
                {
                  char buf3[MSG_SIZE+1];
                  my_strncpy (buf3, text, MSG_SIZE);
                  snprintf (text, MSG_SIZE, "%s, %s", buf3, buf2[1]);
                }
            }
        } 
    }
}

void
ModWhois::work (void)
{
  if (Net::readok (pipes[0]))
    {
      int i = read (pipes[0], buf, MSG_SIZE);
      if (i == 0 || i == -1)
        {
          if (text[0] == 0)
            SEND_TEXT (dest, "No info returned for %s.", (c_char)host);
          else
            SEND_TEXT (dest, "Whois %s: %s", (c_char)host, text);
          stop ();
          return;
        }
      buf[i] = 0;
      parse ();
    }
}

void
ModWhois::stop (void)
{
  s->works.del ((void *)whois_work);
  close (pipes[0]);
#ifndef WINDOZE
  kill (pid, SIGKILL);		// needed for !moddel
  waitpid (pid, NULL, 0);
#endif	// !WINDOZE
  in_use = 0;
}

/////////////
// commands
/////////////

// !whois ip
static void
whois_cmd_whois (NetServer *s)
{
#ifndef WHOIS_PATH
  SEND_TEXT (DEST, "This command is not available.");
#else
  ModWhois *whois = server2whois (s);
  if (whois == NULL)
    {
      SEND_TEXT (DEST, "This command is not available.");
      return;
    }
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_WHOIS);
      return;
    }
  if (whois->in_use)
    SEND_TEXT (DEST, "Another request is being processed.");
  else if (!whois->start (BUF[1], DEST))
    SEND_TEXT (DEST, "Whois request failed.");
#endif		// !WHOIS_PATH
}

////////////////////
// module managing
////////////////////

// return the whois for a given server, NULL if nonexistant
static ModWhois *
server2whois (NetServer *s)
{
  ModWhois *a;
  whois_list->rewind ();
  while ((a = (ModWhois *)whois_list->next ()) != NULL)
    if (a->s == s)
      return a;
  return NULL;
}

// configuration file's local parser
static void
whois_conf (NetServer *s, c_char bufread)
{
  char buf[2][MSG_SIZE+1];

  strsplit (bufread, buf, 1);

  if (strcasecmp (buf[0], "bot") == 0)
    {
      ModWhois *whois = new ModWhois (s);
      if (whois == NULL)
        s->b->conf_error ("error initializing whois");
      whois_list->add ((void *)whois);
      s->vars.var_add ("whois_ripe", whois_var);
      s->script.cmd_bind (whois_cmd_whois, LEVEL_WHOIS, "!whois", module.mod, HELP_WHOIS);
    }

}

// module termination
static void
whois_stop (void)
{
  ModWhois *whois;
  whois_list->rewind ();
  while ((whois = (ModWhois *)whois_list->next ()) != NULL)
    {
      if (whois->in_use)
        whois->stop ();
      whois->s->vars.var_del ("whois_ripe");
      delete whois;
    }
  delete whois_list;
}

// module initialization
static void
whois_start (void)
{
  whois_list = new List ();
}

static void
whois_var (NetServer *s, c_char name, char *data, size_t n)
{
  ModWhois *whois = server2whois (s);
  if (whois == NULL)
    return;
  if (strcasecmp (name, "whois_ripe") == 0)
    {
      if (n != 0)
        itoa (whois->whois_ripe, data, n);
      else
        whois->whois_ripe = atoi (data);
    }
}

static void
whois_work (NetServer *s)
{
  ModWhois *whois = server2whois (s);
  if (whois == NULL)
    s->works.del ((void *)whois_work);
  else
    whois->work ();
}

