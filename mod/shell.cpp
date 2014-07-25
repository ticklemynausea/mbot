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

#include <popen-noshell.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <string>
#include <sstream>
#include <algorithm>
using namespace std;

#define HELP_shell "Can't get you help for this command! :("

#define CMD_LEVEL 10
#define CMD_TIMER "!timer"
#define HELP_timer  "!timer <start|stop|list> [n] - controls shell module's timers"

#define TIMESTR_SIZE 10
#define FIELD_SIZE (NICK_SIZE+TIMESTR_SIZE+1)
#define TRIGGER_SIZE 64
#define COMMAND_SIZE 1024
#define ARGS_SIZE 256
#define RESULT_LINE_SIZE 1024
#define RESULT_LINE_MAX 100
#define FLAGS_START 1

#define SOURCE s->script.source
#define DEST s->script.dest
#define BUF s->script.buf
#define SEND_TEXT s->script.send_text

#define RESPONSE_USERMSG 0
#define RESPONSE_CHANMSG 1
#define RESPONSE_USERNOT 2
#define RESPONSE_CHANNOT 3

// /!\ ACHTUNG! CAUTION WITH THE LINE BELOW         /!\
// /!\ INCLUDING CERTAIN CHARACTERS HERE WILL ALLOW /!\
// /!\ ARBITRARY SHELL COMMANDS TO BE EXECUTED      /!\ 

#define VALID_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-.:/@!#ÁáÀàÃãÂâÉéÈèÊêÍíÌìÎîÓóÒòÕõÔôÚúÙùÛûÇç"

// /!\ CAUTION WITH THE LINE ABOVE. ACHTUNG!        /!\ 

//The characters you want tabs replaced with.
#define TAB_CHAR "  "

extern FILE* popen_noshell(char * const command[], const char *mode);
extern int pclose_noshell(FILE *file);

// utils 

// for use with replace_if

bool is_invalid_char(char c) {
  char list[] = VALID_CHARS;
  int len = sizeof(list)/sizeof(list[1]);
  
  for (int i = 0; i < len; i++) {
    if (list[i] == c)
      return false;
  }
  return true;
}

// http://stackoverflow.com/questions/3418231/c-replace-part-of-a-string-with-another-string

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

struct command_type {
  String* trigger;  // trigger
  int user_level;    // level required to run command
  int cmd_type;  // command type (0=channel, 1=pvt, 2=both)
  String* command;  // shell command
  
  command_type(String* t, int ul, int ct, String* c) : 
    trigger (t),
    user_level (ul),
    cmd_type (ct),
    command (c) {}
    
  ~command_type() {
    delete trigger;
    delete command;
  }
};

struct timer_type {
  String* destination;  // destination of the result
  int interval;  // command type (0=channel, 1=pvt, 2=both)
  int flags; // flags;
  String* command;  // shell command

  NetServer* server; // the server object;
  pthread_t t_id;
  bool alive;
  
  timer_type(String* d, int i, int f, String* c, NetServer* s) : 
    destination (d),
    interval (i),
    flags (f),
    command (c),
    server (s) 
  {
    alive = false;  
  }
    
  ~timer_type() {
    delete destination;
    delete command;
  }
  
  void start();
  void stop();
  
};

// execute
void execute_command(char* command[], char* destination, NetServer* s) {
  int lines_sent = 0;
  char buffer[RESULT_LINE_SIZE];
  FILE* p;
  cout << "stuff " << endl;
  for (unsigned int i = 0; command[i] != NULL; i++) {
    cout << "command " << i << " '" << command[i] << "'" <<endl;
  }
  if ((p = popen_noshell(command, "r")) == NULL) {
    cout << "stuff 1.1" << endl;
    SEND_TEXT(destination, "OMG HELP ME! SOMETHING IS WRONG WITH POOPEN!!!");
    return;
    
  }
  cout << "100" << endl;
  while(!feof(p)) {
    cout << "not feof" << endl;
    if (fgets(buffer, RESULT_LINE_SIZE, p) == NULL) {
      break;
    }

    cout << "buffer: " << buffer << endl;
    
    if (lines_sent >= RESULT_LINE_MAX) {
      SEND_TEXT(destination, "%scommand muted at %d lines.\x02", 
        "\x02", RESULT_LINE_MAX);
      break;
    }
    
    string temp(buffer);
    while (temp.find("\t") != string::npos)
      replace(temp, "\t", TAB_CHAR);

    SEND_TEXT(destination, "%s", temp.c_str());
    lines_sent++;
  }
  cout << "101" << endl;
  pclose_noshell(p);

  // free argv buffers
  for (unsigned int i = 0; command[i] != NULL; i++) {
    cout << "freeing " << command[i] << endl;
    free(command[i]);
  }

  
}

void* wait_thread(void* arg) {
  
  timer_type* me = (timer_type*)arg;

  while (me->alive) {
    sleep(me->interval);
  // /!\
  //  execute_command(me->command->getstr(), me->destination->getstr(), 
  //      me->server);
  }
}

void timer_type::start() {
  this->alive = true;
  int r = pthread_create(&(this->t_id), NULL, &wait_thread, (void*)this);
}

void timer_type::stop() {
  this->alive = false;
  pthread_cancel(this->t_id);
  pthread_join(this->t_id, NULL);
}

// This class defines each instance of the module.
// NetServer *s represents the server the module belongs to.
// If you need additional data (like a list of nicks or channels)
// it is here where it belongs.

struct shell_type {
  NetServer *s;
  List *commands;
  List *timers;

  shell_type (NetServer *server) : 
    s (server) {
      commands = new List();
      timers = new List();
    }
    
  ~shell_type() {
    delete commands;
    delete timers;
  }
};

List * shell_list;


///////////////
// prototypes
///////////////

// This function is used to retrieve the Module instance in a context
// where you only have a server.

static shell_type *server2shell (NetServer *);

// This is a generic callback for server events
static void shell_event (NetServer *);

// This is the function where the module is initialized
static void shell_conf (NetServer *, c_char);

// This function runs when the module starts
static void shell_stop (void);

// This function runs when the module stops.
// This is where you delete stuff you created.
static void shell_start (void);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "shell",
  shell_start,
  shell_stop,
  shell_conf,
  NULL
};

}

// builds the command based on user input
// if the return value is false, buffer contains
// an error message

// this method prevents command injection via shell processing
// by using an alternative version of popen which skips the usage
// of /bin/sh to parse the given command, using execv instead of execl


bool parse_command(char *command_out[], command_type* command, char* mask, char* params) {

  // c++ strings for processing this crap
  string mask_s(mask), params_s(params), command_s(command->command->getstr()); 
  cout << "command:" << command_s << endl;
  cout << "mask_s:" << mask_s << endl;
  cout << "params_s:" << params_s << endl;
  // replace
  replace(command_s, "{n}", mask_s);
  replace(command_s, "{}", params_s);

  // put first token in a char pointer 
  istringstream iss(command_s);
  cout << "command:" << command_s << endl;

  // make a list of pointers for execv from the given char pointer
  
  string token;
  unsigned int i;
  size_t t_len;
  for (i = 0; iss >> token; i++) {
    t_len = token.length();
    command_out[i] = (char*)malloc(COMMAND_SIZE);
    strncpy(command_out[i], token.c_str(), t_len);
    command_out[i][t_len] = 0 ;
  }

  cout << "3" << endl;
  // NULL termination for execv
  
  command_out[i] = NULL;
  cout << "4" << endl;
  return true;
}

//returns the command for a given trigger
command_type* trigger2command(char* trigger, List* command_list) {
  command_type *command;
  command_list->rewind ();
  
  while ((command = (command_type *)command_list->next ()) != NULL) {
    String* trigger_lc = new String(trigger, TRIGGER_SIZE);
    trigger_lc->lower();
    if (*(command->trigger) == *trigger_lc) {
      delete trigger_lc;
      return command;
    } else {
      delete trigger_lc;
    }
  }
  return NULL;
}

/////////////
// commands
/////////////

// !shell cmd
static void shell_cmd (NetServer *s)
{

  char nick[NICK_SIZE];
  shell_type *shell = server2shell (s);

  if (shell == NULL) {
    SEND_TEXT (DEST, "This command is not available.");
    return;
  }

  strsplit (CMD[3], BUF, 1);

  command_type* command = trigger2command(BUF[0], shell->commands);
  

  
  if (command == NULL) {
  
    SEND_TEXT (DEST, "Failed retrieving :(");  
    
  } else {
    
    bool isChannel = (CMD[2][0] == '#') || ((CMD[2][0] == '@') && (CMD[2][1] == '#'));
    if ((isChannel && command->cmd_type == 1) || (!isChannel && command->cmd_type == 0)) {
      if (command->cmd_type == 0) {
        SEND_TEXT (DEST, "This command is only available in channels.");
      } else {
        SEND_TEXT (DEST, "This command is only available in private.");
      }
      return;
    }

    // get a command line string, safe for execution
    char *command_execv[ARGS_SIZE];

    if (parse_command(command_execv, command, CMD[0], BUF[1])) {
      cout << "meeseeks " << command_execv << endl;
      
      for (unsigned int i = 0; command_execv[i] != NULL; i++) {
        cout << "command " << i << " '" << command_execv[i] << "'" << endl;
      }
      execute_command(command_execv, DEST, s); 
      cout << "ZZZ:" << command_execv << endl;  
    }

  }
}

// !timer cmd
static void cmd_timer_list(NetServer *s, shell_type *shell) {
  int n = 1;
  int len = shell->timers->count();
  
  timer_type *p;
  shell->timers->rewind ();
  while ((p = (timer_type *)shell->timers->next ()) != NULL) {
    char c =  (p->alive ? '+' : '-');
    SEND_TEXT(DEST, "%c%d/%d: Every %ds to %s \"%s\"", c, n, len, p->interval, 
      p->destination->getstr(), p->command->getstr());
    n++;
  }
}

static void cmd_timer_stop(NetServer *s, shell_type *shell) {

  int n = atoi(BUF[2]);
  int len = shell->timers->count();
  
  if ((n <= 0) || (n > len)) {
    SEND_TEXT(DEST, "%s", HELP_timer);
  } else {
    int i = 1;
    timer_type *p;
    shell->timers->rewind ();
    while (((p = (timer_type *)shell->timers->next ()) != NULL)
       && (i++ < n));
    
    if (p->alive) {
      p->stop();
      SEND_TEXT(DEST, "Stopped timer %d.", n);
    } else {
      SEND_TEXT(DEST, "Timer %d already stopped!", n);
    }
  }
}

static void cmd_timer_start(NetServer *s, shell_type *shell) {

  int n = atoi(BUF[2]);
  int len = shell->timers->count();
  
  if ((n <= 0) || (n > len)) {
    SEND_TEXT(DEST, "%s", HELP_timer);
  } else {
    int i = 1;
    timer_type *p;
    shell->timers->rewind ();
    while (((p = (timer_type *)shell->timers->next ()) != NULL)
       && (i++ < n));
    
    if (p->alive == false) {
      p->start();
      SEND_TEXT(DEST, "Started timer %d.", n);
    } else {
      SEND_TEXT(DEST, "Timer %d already started!", n);
    }
  }
}

static void shell_cmd_timer (NetServer *s)
{

  shell_type *shell = server2shell (s);

  if (shell == NULL) {
    SEND_TEXT (DEST, "This command is not available.");
    return;
  } 
  
  // formato: !timer cmd n
  strsplit (CMD[3], BUF, 2);
  
  if ((strcmp(BUF[1], "list") == 0) || (strcmp(BUF[1], "") == 0)) {
    cmd_timer_list(s, shell);
  } else if (strcmp(BUF[1], "stop") == 0) {
    cmd_timer_stop(s, shell);
  } else if (strcmp(BUF[1], "start") == 0) {
    cmd_timer_start(s, shell);
  } else {
    SEND_TEXT(DEST, "%s", HELP_timer);
  }
}

/////////////////////
// events
////////////////////

// server replies

void shell_reply(NetServer *s)
{

  // start timers. in a perfect world the timer would be stopped and
  // started with the events regarding the bot's presence on the channels,
  // but this would involve additional parsing of the "target" string
  // so meh.
  if (REPLY == INIT) {
  
    shell_type *shell;
    timer_type *p;
    
    shell = server2shell(s);
    if (shell == NULL) {
      return;
    }

    // start timers
    shell->timers->rewind ();
    while ((p = (timer_type *)shell->timers->next ()) != NULL) {
       if ((p->flags == FLAGS_START) && (p->alive == false)) {
        p->start();
       }
    }
  }

}

/////////////////////////////////////////////////////////////////////////////
//server events
void shell_event (NetServer *s)
{
  // stops timers on connection error
  if (EVENT == EVENT_ERROR) {

    shell_type *shell;
    timer_type *p;
    
    shell = server2shell(s);
    if (shell == NULL) {
      return;
    }

    shell->timers->rewind ();
    while ((p = (timer_type *)shell->timers->next ()) != NULL) {
      if (p->alive == true) {
        p->stop();
      }
    }
    
  }
}

  /////////////////////////////////////////////////////////////////////////////
 /////////////////////////////////////////////////////////////////////////////
// module managing

// returns the invite channel for a given server, NULL if nonexistant
static shell_type * server2shell (NetServer *s)
{
  shell_type *a;
  shell_list->rewind ();
  while ((a = (shell_type *)shell_list->next ()) != NULL)
    if (a->s == s)
      return a;
  return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// configuration file's local parser - on instance load
static void shell_conf (NetServer *s, c_char bufread)
{

  char buf[5][MSG_SIZE+1];
  strsplit (bufread, buf, 4);
  shell_type *shell;

  /////////////////////////////////////////////////////////////////////////////  
  // initialize module for this server
  if ((strcasecmp (buf[0], "shell") == 0)
  ||  (strcasecmp (buf[0], "timer") == 0)) {
    shell = server2shell(s);
    
    if (shell == NULL) {
      shell = new shell_type (s);
      
      if (shell == NULL)
        s->b->conf_error ("error initializing shell");

      shell_list->add ((void *)shell);
      s->script.cmd_bind (shell_cmd_timer, CMD_LEVEL, CMD_TIMER, module.mod, HELP_timer);
      s->script.replies.add ((void *)shell_reply);
      s->script.events.add ((void *)shell_event);
      
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // Create a new command_type instance
  // 0     1        2          3           4
  // shell <trigger> <cmdtype> <userlevel> <shell command>
  if (strcasecmp (buf[0], "shell") == 0) {

    String* n_trigger = new String(buf[1], TRIGGER_SIZE);
    int n_cmdlevel = atoi(buf[3]);
    int n_cmdtype = atoi(buf[2]);
    String* n_command = new String(buf[4], COMMAND_SIZE);

    shell->commands->add(
      (void*) new command_type(
         n_trigger,
         n_cmdlevel,
         n_cmdtype,
         n_command
      )
    );

    s->script.cmd_bind (shell_cmd, n_cmdlevel, buf[1], module.mod, HELP_shell);
    cerr << "command " << buf[1] << " is bound to \"" << buf[4] << "\" with acc lvl "
         << n_cmdlevel <<  endl;

  /////////////////////////////////////////////////////////////////////////////
  // Create a new timerly executed command
  //
  // periodic <#destination,#destination,destination,...> <interval> <autostart> <command> 
  } else if (strcasecmp (buf[0], "timer") == 0) { 

    String* n_destination = new String(buf[1], TRIGGER_SIZE);
    int n_interval = atoi(buf[2]);
    int n_flag = atoi(buf[3]);
    String* n_command = new String(buf[4], COMMAND_SIZE);

    timer_type* p = new  timer_type(n_destination, n_interval, n_flag, n_command, s);
    shell->timers->add((void*)p);

    cerr << "timer " << n_interval << "s will run \"" << buf[4] 
         << "\" and pipe output to " << buf[1] <<  endl;

  }
}

/////////////////////////////////////////////////////////////////////////////
// module termination -- on unload
static void shell_stop (void)
{
  shell_type *shell;
  command_type *command;
  timer_type *timer;
  shell_list->rewind ();

  while ((shell = (shell_type *)shell_list->next ()) != NULL) {
    shell->s->script.events.del ((void *)shell_event);
    shell->commands->rewind();
    while ((command = (command_type*)shell->commands->next ()) != NULL) {
      delete command;
    }
    
    shell->timers->rewind();
    while ((timer = (timer_type*)shell->timers->next ()) != NULL) {
      if (timer->alive) {
        timer->stop();
      }
      delete timer;
    }
    delete shell;
  }

  delete shell_list;
}

/////////////////////////////////////////////////////////////////////////////
// module initialization - on load
static void shell_start (void)
{
  shell_list = new List();
}

