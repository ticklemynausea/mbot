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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <easycurl.h>

// mbot macros
#define SOURCE s->script.source
#define DEST s->script.dest
#define BUF s->script.buf
#define SEND_TEXT s->script.send_text

#define CMD_LEVEL 10
#define CMD_SAVE_TRIGGER "!saveurls"
#define CMD_LOAD_TRIGGER "!loadurls"
#define CMD_STAT_TRIGGER "!staturls"

#define HELP_save "!saveurls - save urls to file"
#define HELP_load "!loadurls - load urls from file - will overwrite whatever is in memory!"
#define HELP_stat "!staturls [n] - check status of urls in memory"

// buffer sizes
#define MAX_URL_LENGTH 1024
#define MAX_URL_DETECT 3
#define TIME_BUFFER_SIZE 80

// used to prevent nick highlights
#define BLOCK_CHAR ""

// file to store the url database
#define URL_FILE "urlsniffer.list"

// number of seconds it takes before an old url is announced to the channel as old
#define TIMEIN_OLD 0


struct sniffing_channel_type {
  String* channel;
  bool tellall; // sniff all links or just text/html
  bool tellfail; // message on fail?
  bool tellold; // tell if the url was already seen by the channel

  sniffing_channel_type(String* s, bool th, bool tf, bool to) : 
    channel (s), 
    tellall (th),
    tellfail (tf),
    tellold (to) {}
    
  ~sniffing_channel_type() {
    delete channel;
  }
};

/* For telling on old urls */
struct url_type {
  string url;
  string channel;
  string nick;
  time_t timestamp;
  
  url_type(string u, string c, string n) :
    url (u), channel(c), nick(n), timestamp(time(0))
    { }

  url_type(string u, string c, string n, string t) {
    this->url = u;
    this->channel = c;
    this->nick = n;
    
    long l;
    sscanf(t.c_str(), "%ld", &l);
    this->timestamp = (time_t)l;
  }

};

struct urlsniffer_type {
  NetServer *s;
  List* channels;
  vector<url_type> urls_seen;

  bool sniffing_channel(c_char channel);
  bool telling_fail_on_channel(c_char channel);
  bool telling_all_on_channel(c_char channel);
  bool telling_old_on_channel(c_char channel);
  time_t how_old_url(string url, string channel, string newnick, string& orignick);
  int save_urls_to_file();
  int load_urls_from_file();
  urlsniffer_type (NetServer *server) : 
    s (server) {}
  
  ~urlsniffer_type () {
    delete channels;
  }
};


List * urlsniffer_list;

///////////////
// Utils
///////////////

void unixtime2human(char* buffer, time_t t) {
  struct tm* dt;
  dt = localtime(&t);
  strftime(buffer, TIME_BUFFER_SIZE, "%c", dt);
}

void unixtime2human2(char* buffer, long age) {
  int days = age / 60 / 60 / 24;
  int hours = (age / 60 / 60) % 24;
  int minutes = (age / 60) % 60;

  char pl_days[2] = {0};
  if (days != 1)
    strcpy(pl_days, "s");
  char pl_hours[2] = {0};
  if (hours != 1)
    strcpy(pl_hours, "s");
  char pl_minutes[2] = {0};
  if (days != 1)
    strcpy(pl_minutes, "s");
  
  if (days == 0 && hours == 0) {
    if (minutes == 0) {
      snprintf(buffer, TIME_BUFFER_SIZE, "just now", minutes, pl_minutes);
    } else {
      snprintf(buffer, TIME_BUFFER_SIZE, "%d minute%s ago", minutes, pl_minutes);
    }
  } else if (days == 0) {
    if (minutes > 0) {
      snprintf(buffer, TIME_BUFFER_SIZE, "%d hour%s, %d minute%s ago", hours, pl_hours, minutes, pl_minutes);
    } else {
      snprintf(buffer, TIME_BUFFER_SIZE, "%d hour%s ago", hours, pl_hours);
    }
  } else {
    if (hours > 0) {
      snprintf(buffer, TIME_BUFFER_SIZE, "%d day%s, %d hour%s ago", days, pl_days, hours, pl_hours);
    } else {
      snprintf(buffer, TIME_BUFFER_SIZE, "%d day%s ago", days, pl_days);
    }
  }
}

char* ircstrip(char* src, char* dst) {
  int dst_i = 0;
  bool strp_colors = false;
  for (int i = 0; src[i] != 0; i++) {
    switch (src[i]) {
      case 0x02:
      case 0x1F:
      case 0x16:
      case 0x0F:
        break;
      case 0x03:
        strp_colors = true;
        break;
      default:
        if (strp_colors and (isdigit(src[i]) or src[i] == ',')) {
          NULL; 
        } else {
          strp_colors = false;
          dst[dst_i++] = src[i];
        }

        break;
    }
  }
  dst[dst_i] = 0;
  return dst;
}

int geturls (char* src, char dst[][MSG_SIZE]) {
  int count = 0;
  char *r = NULL;
  r = strtok(src, " ");
  while (r != NULL) {
    char temp[MSG_SIZE];
    ircstrip(r, temp);
    if ((strncmp(temp, "http://", 7) == 0) 
    or  (strncmp(temp, "https://", 8) == 0) 
    or  (strncmp(temp, "www.", 4) == 0)) {
      if (count < MAX_URL_DETECT) {
       strcpy(dst[count], temp);
       count++;
      } else {
        break;
      }
    }
    r = strtok(NULL, " ");
  }
  return count;
}

// as the module does not keep the irc messages where the strings came from,
// this uses an heuristic to decide if the url is being quoted:
// the original poster's nick must come before the url.

bool is_quoting_url(string irc_message, string url, string orignick) {
  int n = irc_message.find(orignick);
  int m = irc_message.find(url);
  return n != string::npos && m != string::npos && n < m;
}

///////////////
// prototypes
///////////////
static urlsniffer_type *server2urlsniffer (NetServer *);
static void urlsniffer_event (NetServer *);
static void urlsniffer_conf (NetServer *, c_char);
static void urlsniffer_stop (void);
static void urlsniffer_start (void);
extern size_t decode_html_entities_utf8(char *dest, const char *src);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "urlsniffer",
  urlsniffer_start,
  urlsniffer_stop,
  urlsniffer_conf,
  NULL
};

}


//check to see if a certain channel is in the mod's sniff list
bool urlsniffer_type::sniffing_channel(c_char channel) {
  
  sniffing_channel_type *s;
  this->channels->rewind ();
  while ((s = (sniffing_channel_type *)this->channels->next()) != NULL)
  {
    if (*(s->channel) == channel)
      return true;
  }
  return false;
}

bool urlsniffer_type::telling_fail_on_channel(c_char channel) {
  
  sniffing_channel_type *s;
  this->channels->rewind ();
  while ((s = (sniffing_channel_type *)this->channels->next()) != NULL)
  { 
    if (*(s->channel) == channel)
      return s->tellfail;
  }
  return false;
}

bool urlsniffer_type::telling_all_on_channel(c_char channel) {
  
  sniffing_channel_type *s;
  this->channels->rewind ();
  while ((s = (sniffing_channel_type *)this->channels->next()) != NULL)
  {
    if (*(s->channel) == channel)
      return s->tellall;
  }
  return false;
}

bool urlsniffer_type::telling_old_on_channel(c_char channel) {
  
  sniffing_channel_type *s;
  this->channels->rewind ();
  while ((s = (sniffing_channel_type *)this->channels->next()) != NULL)
  {
    if (*(s->channel) == channel)
      return s->tellold;
  }
  return false;
}

int urlsniffer_type::save_urls_to_file() {
  ofstream f;
  f.open(URL_FILE, fstream::trunc);
  int i = 0;
  for (vector<url_type>::iterator it = this->urls_seen.begin() ; it != urls_seen.end(); ++it) {
    f << it->timestamp << " " << it->nick << " " << it->channel << " " << it->url << endl;
    i++;
  }
  f.close();
  return i;
  
}

int urlsniffer_type::load_urls_from_file() {
  
  ifstream f;
  f.open(URL_FILE);
  string timestamp, nick, channel, url;
  int i = 0;

  if (f.is_open()) {
    this->urls_seen.clear();
    while (!f.eof()) {
    
      f >> timestamp;
      f >> nick;
      f >> channel;
      f >> url;
      
      if (!f.eof()) {
        url_type url_t(url, channel, nick, timestamp);
        this->urls_seen.push_back(url_t);
        i++;
      }
        
    }
  }
  f.close();
  return i;
}

time_t urlsniffer_type::how_old_url(string url, string channel, string newnick, string& orignick) {
  time_t now = time(0);
  bool found = false;
  for (vector<url_type>::iterator it = this->urls_seen.begin() ; it != urls_seen.end(); ++it) {

    if (it->url == url && it->channel == channel) {
      found = true;
      orignick = string(it->nick);
      return now - it->timestamp;
    }
  }
  
  if (!found) {
    url_type url_t(url, channel, newnick);
    this->urls_seen.push_back(url_t);
  }
  return (time_t)0;
}

/////////////
// commands
/////////////

static void
urlsniffer_saveurls_cmd (NetServer *s)
{

  urlsniffer_type *urlsniffer = server2urlsniffer (s);

  if (urlsniffer == NULL) {
    SEND_TEXT (DEST, "This command is not available.");
    return;
  }

  int result = urlsniffer->save_urls_to_file();
  SEND_TEXT(DEST, "%d URLs saved to file!", result);
}

static void
urlsniffer_loadurls_cmd (NetServer *s)
{
  urlsniffer_type *urlsniffer = server2urlsniffer (s);
  
  if (urlsniffer == NULL) {
    SEND_TEXT (DEST, "This command is not available.");
    return;
  }

  int result = urlsniffer->load_urls_from_file();
  SEND_TEXT(DEST, "%d URLs loaded from file!", result);
}
  
static void
urlsniffer_staturls_cmd (NetServer *s)
{
  urlsniffer_type *urlsniffer = server2urlsniffer (s);
  
  if (urlsniffer == NULL) {
    SEND_TEXT (DEST, "This command is not available.");
    return;
  }
  
  strsplit (CMD[3], BUF, 1);
  
  /*string params(BUF[1]);
  istringstream oss(params);*/
  int n = atoi(BUF[1]);
  int len = urlsniffer->urls_seen.size();
  
  if ((n <= 0) || (n > len)) {
    SEND_TEXT(DEST, "%d URLs in memory!", len);
  } else {
    url_type url = urlsniffer->urls_seen.at(n-1);
    char buffer[TIME_BUFFER_SIZE];
    unixtime2human(buffer, url.timestamp);
    SEND_TEXT(DEST, "%d/%d: at %s on %s by %s: %s", n, len, buffer, url.channel.c_str(), url.nick.c_str(), url.url.c_str());
  }
  
  
  /*for (vector<url_type>::iterator it = urlsniffer->urls_seen.begin() ; it != urlsniffer->urls_seen.end(); ++it) {
    SEND_TEXT(DEST, "%ld %s %s %s", it->timestamp, it->nick.c_str(), it->channel.c_str(), it->url.c_str());
  }*/
}

/////////////////////
// events
////////////////////

//urlsniffer event
void
urlsniffer_event (NetServer *s)
{

  if (EVENT != EVENT_PRIVMSG
      || CMD[3][0] == ''				// ctcp
      || (CMD[2][0] >= '0' && CMD[2][0] <= '9'))	// dcc chat
    return;

  urlsniffer_type* urlsniffer = server2urlsniffer(s);


  if (!((CMD[2][0] == '#') || ((CMD[2][0] == '@') && (CMD[2][1] == '#'))))
    return;
	    
  if (!urlsniffer->sniffing_channel(CMD[2]))
    return;
	
  char url_list[MAX_URL_DETECT][MSG_SIZE];
  int count;
  char cmdcopy[MSG_SIZE];
  strncpy(cmdcopy, CMD[3], MSG_SIZE);
  count = geturls(cmdcopy, url_list);
  bool haschecked = false;
  for (int i = 0; i < count; i++) {
    std::string message;
    for(int j = 0; j < i; j++) {
      if (strcmp(url_list[i], url_list[j]) == 0) {
      haschecked = true;
      }
    }
    
    if (haschecked) {
      haschecked = false;
      continue;
    }

    ostringstream oss;
    string url = url_list[i];
    string nick = SOURCE;
    string channel(CMD[2]);
    
    //telling on old urls?
    string orignick;
    time_t age;
    
    bool tell_old = urlsniffer->telling_old_on_channel(CMD[2]);
    
    if (tell_old) {
    
      age = urlsniffer->how_old_url(url, channel, nick, orignick); 
      tell_old =  nick != orignick; //don't tell on original link poster 

      if (age > TIMEIN_OLD && tell_old) {
      
        string irc_message = CMD[3];
        bool is_quoting = is_quoting_url(irc_message, url, orignick);

        if (!is_quoting) {
          
          char buffer[TIME_BUFFER_SIZE];
          unixtime2human2(buffer, age);
          //nick.insert(1, BLOCK_CHAR); //kept commented to nag the nick on purpose
          orignick.insert(1, BLOCK_CHAR);
          SEND_TEXT(DEST, "BAH! %s's link is OLD! (shown %s by %s)", nick.c_str(), buffer, orignick.c_str());
          
          continue;
        }
      
      }
      
    }

    //prevent highlights
    nick.insert(1, BLOCK_CHAR);
    
    EasyCurl e(url);
    if (e.requestWentOk == 1) {
       if (e.isHtml) {
         oss << nick << "'s link title: " << e.html_title << " (";
       } else {
         oss << nick << "'s link title: N/A (";
       }
       if (e.response_code != "200") {
         oss << "HTTP " << e.response_code << ", ";
       }
       oss << e.response_content_type;
       if (e.response_content_length == "-1") {
         oss << ")";
       } else {
         oss << ", " << e.response_content_length << " bytes)";
       }
    } else {
      oss << nick << "'s link sucks. (" << e.error_message << ")";
    }
    

    
    if ((!e.requestWentOk) && !urlsniffer->telling_fail_on_channel(CMD[2]))
      return;
      
    if (!e.isHtml && !urlsniffer->telling_all_on_channel(CMD[2]))
      return;
    
    
    SEND_TEXT (DEST, "%s", oss.str().c_str());
  }
}


////////////////////
// module managing
////////////////////

// returns the invite channel for a given server, NULL if nonexistant
static urlsniffer_type *
server2urlsniffer (NetServer *s)
{
  urlsniffer_type *a;
  urlsniffer_list->rewind ();
  while ((a = (urlsniffer_type *)urlsniffer_list->next ()) != NULL)
    if (a->s == s)
      return a;
  return NULL;
}

// configuration file's local parser
static void
urlsniffer_conf (NetServer *s, c_char bufread)
{

  char buf[5][MSG_SIZE+1];
  strsplit (bufread, buf, 4);
 
  if (strcasecmp (buf[0], "urlsniffer") != 0)
    return;
  
  //verificar se ja foi inicializado!
  urlsniffer_type *urlsniffer = server2urlsniffer(s);
  if (urlsniffer == NULL) {
    urlsniffer = new urlsniffer_type (s);
    if (urlsniffer == NULL)
      s->b->conf_error ("error initializing urlsniffer");
	    urlsniffer_list->add ((void *)urlsniffer);
	    urlsniffer->channels = new List();
	    s->script.events.add ((void *)urlsniffer_event);

	    s->script.cmd_bind (urlsniffer_saveurls_cmd, CMD_LEVEL, CMD_SAVE_TRIGGER, module.mod, HELP_save);	  
	    s->script.cmd_bind (urlsniffer_loadurls_cmd, CMD_LEVEL, CMD_LOAD_TRIGGER, module.mod, HELP_load);
	    s->script.cmd_bind (urlsniffer_staturls_cmd, CMD_LEVEL, CMD_STAT_TRIGGER, module.mod, HELP_load);
	    
    int result = urlsniffer->load_urls_from_file();
    cout <<   "  URLsniffer: Loaded " << result << " URLs from file" << endl;
  }
  
  urlsniffer->channels->add ((void *) new sniffing_channel_type(
    new String(buf[1], CHANNEL_SIZE),
    (strncmp(buf[2], "tellall", 7) == 0),
    (strncmp(buf[3], "tellfail", 8) == 0),
    (strncmp(buf[4], "tellold", 7) == 0)
  ));
}

// module termination
static void
urlsniffer_stop (void)
{
  sniffing_channel_type *channel;
  urlsniffer_type *urlsniffer;
  urlsniffer_list->rewind ();
  while ((urlsniffer = (urlsniffer_type *)urlsniffer_list->next ()) != NULL) {
    int result = urlsniffer->save_urls_to_file();
    cout << "  URLsniffer: Saved " << result << " URLs to file" << endl;
    urlsniffer->s->script.events.del ((void *)urlsniffer_event);
    urlsniffer->channels->rewind();
    while ((channel = (sniffing_channel_type*)urlsniffer->channels->next ()) != NULL) {
      delete channel;
    }
  
    delete urlsniffer;
  }
  delete urlsniffer_list;
}

// module initialization
static void
urlsniffer_start (void)
{
  urlsniffer_list = new List();
}

