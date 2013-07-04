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

#include <xmlmemory.h>
#include <parser.h>

#define LEVEL_FRESHMEAT 0
#define LEVEL_GILDOT 0
#define LEVEL_USEPERL 0
#define LEVEL_SLASHDOT 0
#define LEVEL_LINUXTODAY 0
#define LEVEL_SECURITYFOCUS 0

#define HELP_WEB "Show the news headlines for this site."

#define SOURCE s->script.source
#define DEST s->script.dest
#define BUF s->script.buf
#define SEND_TEXT s->script.send_text

enum web_type { WEB_FRESHMEAT, WEB_GILDOT, WEB_USEPERL,
  WEB_SLASHDOT, WEB_LINUXTODAY, WEB_SECURITYFOCUS };

/////////////////////
// class definition
/////////////////////

class ModWeb {
public:
  ModWeb (NetServer *);

  bool start (web_type, c_char, c_char);
  void send_web (c_char, ...);
  c_char get_child (xmlNodePtr, c_char);
  bool web_common (void);
  void web_freshmeat (void);
  void web_gildot (void);
  void web_useperl (void);
  void web_slashdot (void);
  void web_linuxtoday (void);
  void web_securityfocus (void);
  void work (void);
  void stop (void);

  bool in_use, web_pvt, web_notice;
  NetServer *s;			// server to which belongs

private:

  web_type site;
  NetHttp *http;
  String source, dest;
  xmlDocPtr doc;
  xmlNodePtr cur;

};

List *web_list;

///////////////
// prototypes
///////////////

static void web_cmd_freshmeat (NetServer *);

static ModWeb *server2web (NetServer *);
static void web_conf (NetServer *, c_char);
static void web_stop (void);
static void web_start (void);
static void web_var (NetServer *, c_char, char *, size_t);
static void web_work (NetServer *);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "web",
  web_start,
  web_stop,
  web_conf,
  NULL
};

}

////////////////////
// class functions
////////////////////

ModWeb::ModWeb (NetServer *server) : in_use (0), web_pvt (0),
  web_notice (0), s (server), source (HOST_SIZE), dest (CHANNEL_SIZE)
{
}

bool
ModWeb::start (web_type si, c_char src, c_char destination)
{
#ifdef WINDOZE
  return 0;
#else	// !WINDOZE
  site = si;
  source = src;
  dest = destination;
  char *site_host = NULL, *site_file = NULL;
  switch (site)
    {
      case WEB_FRESHMEAT:
        site_host = "freshmeat.net";
        site_file = "/backend/fm.rdf";
        break;
      case WEB_GILDOT:
        site_host = "www.gildot.org";
        site_file = "/gildot.xml";
        break;
      case WEB_USEPERL:
        site_host = "use.perl.org";
        site_file = "/useperl.rdf";
        break;
      case WEB_SLASHDOT:
        site_host = "slashdot.org";
        site_file = "/slashdot.xml";
        break;
      case WEB_LINUXTODAY:
        site_host = "linuxtoday.com";
        site_file = "/backend/linuxtoday.xml";
        break;
      case WEB_SECURITYFOCUS:
        site_host = "www.securityfocus.com";
        site_file = "/rss/news.xml";
        break;
    }
  http = new NetHttp (site_host, site_file, 10000);
  if (http == NULL)
    return 0;
  s->works.add ((void *)web_work);
  in_use = 1;
  return 1;
#endif	// !WINDOZE
}

void
ModWeb::send_web (c_char format, ...)
{
  char buftext[MSG_SIZE+1];
  va_list args;
  va_start (args, format);
  vsnprintf (buftext, MSG_SIZE, format, args);
  va_end (args);
  if (web_pvt)
    {
      if (web_notice)
        s->irc_notice (source, "%s", buftext);
      else
        s->irc_privmsg (source, "%s", buftext);
    }
  else
    {
      if (web_notice)
        s->irc_notice (dest, "%s", buftext);
      else
        s->irc_privmsg (dest, "%s", buftext);
    }
}

/*
  find a child of <node> with <name> and return its content
*/
c_char
ModWeb::get_child (xmlNodePtr node, c_char name)
{
  if (node == NULL && node->childs != NULL)
    return NULL;
  node = node->childs;
  while (node != NULL)
    {
      if (node->name != NULL
          && node->childs != NULL
          && strcmp ((c_char)node->name, name) == 0)
        return (c_char)node->childs->content;
      node = node->next;
    }
  return NULL;
}

/*
  all web_* call this first, it initializes the parsing
  return 0 on failure
*/
bool
ModWeb::web_common (void)
{
  doc = xmlParseMemory (http->html.getstr (), http->html.getlen ());
  if (doc == NULL)
    return 0;
  cur = xmlDocGetRootElement(doc);
  if (cur == NULL)
    {
      xmlFreeDoc (doc);
      return 0;
    }
  if (cur == NULL || (cur = cur->childs) == NULL)
    {
      xmlFreeDoc (doc);
      return 0;
    }
  return 1;
}

void
ModWeb::web_freshmeat (void)
{
  if (!web_common ())
    {
      send_web ("freshmeat.net: error parsing document.");
      xmlFreeDoc (doc);
      return;
    }

  String text ("", MSG_SIZE);
  while (cur != NULL)
    {
      if (strcmp ((const char *)cur->name, "item") == 0)
        {
          c_char str = get_child (cur, "title");
          if (str != NULL)
            {
              if (text.getlen () != 0)
                text += " | ";
              text += str;
            }
        }

      cur = cur->next;
    }
  xmlFreeDoc (doc);

  send_web ("freshmeat.net: %s", (c_char)text);
}

void
ModWeb::web_gildot (void)
{
  if (!web_common ())
    {
      send_web ("gildot.org: error parsing document.");
      xmlFreeDoc (doc);
      return;
    }

  String text ("", MSG_SIZE);
  while (cur != NULL)
    {
      if (strcmp ((const char *)cur->name, "story") == 0
          && cur->childs != NULL && cur->childs->childs != NULL
          && cur->childs->childs->content != NULL)
        {
          if (text.getlen () != 0)
            text += " | ";
          text += (c_char)cur->childs->childs->content;
        }
      cur = cur->next;
    }
  xmlFreeDoc (doc);

  send_web ("gildot.org: %s", (c_char)text);
}

void
ModWeb::web_useperl (void)
{
  if (!web_common ())
    {
      send_web ("use.perl.org: error parsing document.");
      xmlFreeDoc (doc);
      return;
    }

  String text ("", MSG_SIZE);
  while (cur != NULL)
    {
      if (strcmp ((const char *)cur->name, "item") == 0
          && cur->childs != NULL && cur->childs->childs != NULL
          && cur->childs->childs->content != NULL)
        {
          if (text.getlen () != 0)
            text += " | ";
          text += (c_char)cur->childs->childs->content;
        }
      cur = cur->next;
    }
  xmlFreeDoc (doc);

  send_web ("use.perl.org: %s", (c_char)text);
}

void
ModWeb::web_slashdot (void)
{
  if (!web_common ())
    {
      send_web ("slashdot.org: error parsing document.");
      xmlFreeDoc (doc);
      return;
    }

  String text ("", MSG_SIZE);
  while (cur != NULL)
    {
      if (strcmp ((const char *)cur->name, "story") == 0
          && cur->childs != NULL && cur->childs->childs != NULL
          && cur->childs->childs->content != NULL)
        {
          if (text.getlen () != 0)
            text += " | ";
          text += (c_char)cur->childs->childs->content;
        }
      cur = cur->next;
    }
  xmlFreeDoc (doc);

  send_web ("slashdot.org: %s", (c_char)text);
}

void
ModWeb::web_linuxtoday (void)
{
  if (!web_common ())
    {
      send_web ("linuxtoday.com: error parsing document.");
      xmlFreeDoc (doc);
      return;
    }

  String text ("", MSG_SIZE);
  while (cur != NULL)
    {
      if (strcmp ((const char *)cur->name, "story") == 0
          && cur->childs != NULL && cur->childs->childs != NULL
          && cur->childs->childs->content != NULL)
        {
          if (text.getlen () != 0)
            text += " | ";
          text += (c_char)cur->childs->childs->content;
          text.strip_crlf ();
        }
      cur = cur->next;
    }
  xmlFreeDoc (doc);

  send_web ("linuxtoday.com: %s", (c_char)text);
}

void
ModWeb::web_securityfocus (void)
{
  if (!web_common () || (cur = cur->childs) == NULL)
    {
      send_web ("securityfocus.com: error parsing document.");
      xmlFreeDoc (doc);
      return;
    }

  String text ("", MSG_SIZE);
  while (cur != NULL)
    {
      if (strcmp ((const char *)cur->name, "item") == 0)
        {
          c_char str = get_child (cur, "title");
          if (str != NULL)
            {
              if (text.getlen () != 0)
                text += " | ";
              text += str;
            }
        }
      cur = cur->next;
    }
  xmlFreeDoc (doc);

  send_web ("securityfocus.com: %s", (c_char)text);
}

void
ModWeb::work (void)
{
  switch (http->work ())
    {
      case 0:
        return;
      case 1:
        break;
      default:		// >1
        send_web ("Error accessing %s.", (c_char)http->host);
        stop ();
        return;
    }
  switch (site)
    {
      case WEB_FRESHMEAT:
        web_freshmeat ();
        break;
      case WEB_GILDOT:
        web_gildot ();
        break;
      case WEB_USEPERL:
        web_useperl ();
        break;
      case WEB_SLASHDOT:
        web_slashdot ();
        break;
      case WEB_LINUXTODAY:
        web_linuxtoday ();
        break;
      case WEB_SECURITYFOCUS:
        web_securityfocus ();
        break;
    }
  stop ();
}

void
ModWeb::stop (void)
{
  s->works.del ((void *)web_work);
  delete http;
  in_use = 0;
}

/////////////
// commands
/////////////

// !freshmeat
static void
web_cmd_freshmeat (NetServer *s)
{
  ModWeb *web = server2web (s);
  if (web == NULL)
    return;
  if (web->in_use)
    web->send_web ("Another request is being processed.");
  else if (!web->start (WEB_FRESHMEAT, SOURCE, DEST))
    web->send_web ("ModWeb request failed.");
}

// !gildot
static void
web_cmd_gildot (NetServer *s)
{
  ModWeb *web = server2web (s);
  if (web == NULL)
    return;
  if (web->in_use)
    web->send_web ("Another request is being processed.");
  else if (!web->start (WEB_GILDOT, SOURCE, DEST))
    web->send_web ("Web request failed.");
}

// !useperl
static void
web_cmd_useperl (NetServer *s)
{
  ModWeb *web = server2web (s);
  if (web == NULL)
    return;
  if (web->in_use)
    web->send_web ("Another request is being processed.");
  else if (!web->start (WEB_USEPERL, SOURCE, DEST))
    web->send_web ("Web request failed.");
}

// !slashdot
static void
web_cmd_slashdot (NetServer *s)
{
  ModWeb *web = server2web (s);
  if (web == NULL)
    return;
  if (web->in_use)
    web->send_web ("Another request is being processed.");
  else if (!web->start (WEB_SLASHDOT, SOURCE, DEST))
    web->send_web ("Web request failed.");
}

// !linuxtoday
static void
web_cmd_linuxtoday (NetServer *s)
{
  ModWeb *web = server2web (s);
  if (web == NULL)
    return;
  if (web->in_use)
    web->send_web ("Another request is being processed.");
  else if (!web->start (WEB_LINUXTODAY, SOURCE, DEST))
    web->send_web ("Web request failed.");
}

// !securityfocus
static void
web_cmd_securityfocus (NetServer *s)
{
  ModWeb *web = server2web (s);
  if (web == NULL)
    return;
  if (web->in_use)
    web->send_web ("Another request is being processed.");
  else if (!web->start (WEB_SECURITYFOCUS, SOURCE, DEST))
    web->send_web ("Web request failed.");
}

////////////////////
// module managing
////////////////////

// return the web for a given server, NULL if nonexistant
static ModWeb *
server2web (NetServer *s)
{
  ModWeb *a;
  web_list->rewind ();
  while ((a = (ModWeb *)web_list->next ()) != NULL)
    if (a->s == s)
      return a;
  return NULL;
}

// configuration file's local parser
static void
web_conf (NetServer *s, c_char bufread)
{
  char buf[2][MSG_SIZE+1];

  strsplit (bufread, buf, 1);

  if (strcasecmp (buf[0], "bot") == 0)
    {
      ModWeb *web = new ModWeb (s);
      if (web == NULL)
        s->b->conf_error ("error initializing web");
      web_list->add ((void *)web);
      s->vars.var_add ("web_pvt", web_var);
      s->vars.var_add ("web_notice", web_var);
      s->script.cmd_bind (web_cmd_freshmeat, LEVEL_FRESHMEAT, "!freshmeat", module.mod, HELP_WEB);
      s->script.cmd_bind (web_cmd_gildot, LEVEL_GILDOT, "!gildot", module.mod, HELP_WEB);
      s->script.cmd_bind (web_cmd_useperl, LEVEL_USEPERL, "!useperl", module.mod, HELP_WEB);
      s->script.cmd_bind (web_cmd_slashdot, LEVEL_SLASHDOT, "!slashdot", module.mod, HELP_WEB);
      s->script.cmd_bind (web_cmd_linuxtoday, LEVEL_LINUXTODAY, "!linuxtoday", module.mod, HELP_WEB);
      s->script.cmd_bind (web_cmd_securityfocus, LEVEL_SECURITYFOCUS, "!securityfocus", module.mod, HELP_WEB);
    }
}

// module termination
static void
web_stop (void)
{
  ModWeb *web;
  web_list->rewind ();
  while ((web = (ModWeb *)web_list->next ()) != NULL)
    {
      if (web->in_use)
        web->stop ();
      delete web;
    }
  delete web_list;
}

// module initialization
static void
web_start (void)
{
  LIBXML_TEST_VERSION
#ifdef HAVE_XMLKEEPBLANKSDEFAULT
  xmlKeepBlanksDefault (0);
#endif
  web_list = new List ();
}

static void
web_var (NetServer *s, c_char name, char *data, size_t n)
{
  ModWeb *web = server2web (s);
  if (web == NULL)
    return;
  if (strcasecmp (name, "web_pvt") == 0)
    {
      if (n != 0)
	itoa (web->web_pvt, data, n);
      else
        web->web_pvt = atoi (data);
    }
  else if (strcasecmp (name, "web_notice") == 0)
    {
      if (n != 0)
	itoa (web->web_notice, data, n);
      else
        web->web_notice = atoi (data);
    }
}

static void
web_work (NetServer *s)
{
  ModWeb *web = server2web (s);
  if (web == NULL)
    s->works.del ((void *)web_work);
  else
    web->work ();
}

