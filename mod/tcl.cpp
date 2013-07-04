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

missing tcl commands:

getuser
setuser
chattr
newignore
killignore
ignorelist
isignore
restart
rehash
dccsimul
console
echo
dcclist DA PRA FAZER MAS TENHO Q DESCOBRIR COMO SE FAZEM LISTAS DE LISTAS EM C->TCL
setdccaway
connect
listen
link
unlink
encrypt
decrypt
encpass
md5
logfile
chanbans
ischanban
resetbans
chanexempts
ischanexempt
ischanjuped
resetexempts
chaninvites
ischaninvite
resetinvites
getchanjoin
onchansplit
getchanidle
getchanmode
channel
savechannels
loadchannels
newchanban
newban
killchanban
killban
isban
ispermban
matchban
banlist
newchanexempt
newexempt
killchanexempt
killexempt
isexempt
ispermexempt
matchexempt
exemptlist
newchaninvite
newinvite
killchaninvite
killinvite
isinvite
isperminvite
matchinvite
invitelist
addchanrec
delchanrec
getchaninfo
setchaninfo
setlaston
setudef
renudef
deludef
compresstofile
uncompresstofile
compressfile
uncompressfile
notes
erasenotes
listnotes
storenote
sendnote

*/

#include "mbot.h"
#include <tcl.h>

#define LEVEL_TCL 10

#define HELP_TCL "!tcl <load | eval> <arg> - Loads the tcl script <arg>, or evaluates <arg> as a tcl expression"

#define SOURCE s->script.source
#define DEST s->script.dest
#define BUF s->script.buf
#define SEND_TEXT s->script.send_text

#if (TCL_MAJOR_VERSION < 8) || ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION <= 3))
#define TCLARG (ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
#else
#define TCLARG (ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[])
#endif

#if (TCL_MAJOR_VERSION < 8) || ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION <= 3))
#define TCLVAR (ClientData clientData, Tcl_Interp *interp, char *name1, char *name2, int flags)
#else
#define TCLVAR (ClientData clientData, Tcl_Interp *interp, const char *name1, const char *name2, int flags)
#endif

#define TIMERID_SIZE 50

// this indexes the bindtypes array, so be careful
enum bind_enum {
  BIND_MSG, BIND_DCC, BIND_PUB, BIND_MSGM, BIND_PUBM,
  BIND_NOTC, BIND_JOIN, BIND_PART, BIND_SIGN, BIND_TOPC,
  BIND_KICK, BIND_NICK, BIND_CTCP, BIND_CTCR, BIND_RAW,
  BIND_CHAT
};

struct bindtype_type {
  char *name;
  bool stackable;
};

static struct bindtype_type bindtypes[] = {
  { "msg", 0 }, { "dcc", 0 }, { "pub", 0 }, { "msgm", 1 }, { "pubm", 1 },
  { "notc", 1 }, { "join", 1 }, { "part", 1 }, { "sign", 1 }, { "topc", 1 },
  { "kick", 1 }, { "nick", 1 }, { "ctcp", 1 }, { "ctcr", 1 }, { "raw", 1 },
  { "chat", 1 }, { NULL, 0 }
};

enum timer_enum {
  TCL_TIMER, TCL_UTIMER
};

/////////////////////
// class definition
/////////////////////

class ModTcl {
public:
  ModTcl (NetServer *);
  ~ModTcl (void);

  bool loadscript (char *);
  int attr2level (c_char);
  void bind_add (int type, c_char attr, c_char command, c_char proc);
  void bind_del (int, c_char);
  void event_join (void);
  void event_part (void);
  void event_quit (void);
  void event_topic (void);
  void event_kick (void);
  void event_nick (void);
  void event_privmsg (void);
  void event_notice (void);

  struct bind_type {
    int type;
    int level;
    String command;
    String proc;
    bind_type (int t, int l, c_char c, c_char p): type (t), level (l),
      command (c, MSG_SIZE), proc (p, MSG_SIZE) {}
  };
  List binds;

  void do_bind_raw (bind_type *bind);

  struct timer_type {
    time_t time;
    char *command;
    char timerid[TIMERID_SIZE+1];
    timer_enum type;
    timer_type *next;
  };
  timer_type *timers;

  char *timer_add (time_t, c_char, timer_enum);
  bool timer_del (c_char, timer_enum);
  char *escape_chars (c_char str);

  Tcl_Interp *interp;
  NetServer *s;
  bool initialized;

private:

  void do_bind_join (bind_type *bind);
  void do_bind_part (bind_type *bind);
  void do_bind_sign (bind_type *bind);
  void do_bind_topc (bind_type *bind);
  void do_bind_kick (bind_type *bind);
  void do_bind_nick (bind_type *bind);
  void do_bind_chat (bind_type *bind);
  void do_bind_dcc (bind_type *bind);
  void do_bind_pub (bind_type *bind);
  void do_bind_pubm (bind_type *bind);
  void do_bind_msg (bind_type *bind);
  void do_bind_msgm (bind_type *bind);
  void do_bind_notc (bind_type *bind);
  void do_bind_ctcp_ctcr (bind_type *bind);

  u_int timerseq;

  void bind_delall (void);
  void timer_delall (void);
  void escape_bufs (c_char b0, c_char b1 = NULL, c_char b2 = NULL,
    c_char b3 = NULL, c_char b4 = NULL, c_char b5 = NULL, c_char b6 = NULL,
    c_char b7 = NULL);

  char buf[10][MSG_SIZE+1];
  // used by escape_bufs()
  String buf0, buf1, buf2, buf3, buf4, buf5, buf6, buf7;

};

List *tcl_list;

///////////////
// prototypes
///////////////

ModTcl *interp2tcl (Tcl_Interp *);
bool check_args (ModTcl *, int, int);
static void tcl_bind_error (NetServer *, c_char);
static int tcl_proc_adduser TCLARG;
static int tcl_proc_assoc TCLARG;
static int tcl_proc_backup TCLARG;
static int tcl_proc_bind TCLARG;
static int tcl_proc_botisop TCLARG;
static int tcl_proc_botlist TCLARG;
static int tcl_proc_botonchan TCLARG;
static int tcl_proc_chanlist TCLARG;
static int tcl_proc_channels TCLARG;
static int tcl_proc_chnick TCLARG;
static int tcl_proc_countusers TCLARG;
static int tcl_proc_ctime TCLARG;
static int tcl_proc_dccbroadcast TCLARG;
static int tcl_proc_dccdumpfile TCLARG;
static int tcl_proc_dccputchan TCLARG;
static int tcl_proc_dccused TCLARG;
static int tcl_proc_delhost TCLARG;
static int tcl_proc_deluser TCLARG;
static int tcl_proc_die TCLARG;
static int tcl_proc_dnslookup TCLARG;
static int tcl_proc_dumpfile TCLARG;
static int tcl_proc_finduser TCLARG;
static int tcl_proc_flushmode TCLARG;
static int tcl_proc_getchan TCLARG;
static int tcl_proc_getchanhost TCLARG;
static int tcl_proc_getdccaway TCLARG;
static int tcl_proc_getdccidle TCLARG;
static int tcl_proc_getting_users TCLARG;
static int tcl_proc_hand2idx TCLARG;
static int tcl_proc_hand2nick TCLARG;
static int tcl_proc_handonchan TCLARG;
static int tcl_proc_isdynamic TCLARG;
static int tcl_proc_islinked TCLARG;
static int tcl_proc_isop TCLARG;
static int tcl_proc_isvoice TCLARG;
static int tcl_proc_killassoc TCLARG;
static int tcl_proc_killdcc TCLARG;
static int tcl_proc_killtimer TCLARG;
static int tcl_proc_killutimer TCLARG;
static int tcl_proc_maskhost TCLARG;
static int tcl_proc_matchattr TCLARG;
static int tcl_proc_myip TCLARG;
static int tcl_proc_nick2hand TCLARG;
static int tcl_proc_onchan TCLARG;
static int tcl_proc_passwdok TCLARG;
static int tcl_proc_pushmode TCLARG;
static int tcl_proc_putcmdlog TCLARG;
static int tcl_proc_putdcc TCLARG;
static int tcl_proc_putlog TCLARG;
static int tcl_proc_putloglev TCLARG;
static int tcl_proc_putserv TCLARG;
static int tcl_proc_putxferlog TCLARG;
static int tcl_proc_rand TCLARG;
static int tcl_proc_reload TCLARG;
static int tcl_proc_resetchan TCLARG;
static int tcl_proc_save TCLARG;
static int tcl_proc_setchan TCLARG;
static int tcl_proc_strftime TCLARG;
static int tcl_proc_topic TCLARG;
static int tcl_proc_unixtime TCLARG;
static int tcl_proc_userlist TCLARG;
static int tcl_proc_unames TCLARG;
static int tcl_proc_timer TCLARG;
static int tcl_proc_utimer TCLARG;
static int tcl_proc_timers TCLARG;
static int tcl_proc_utimers TCLARG;
static int tcl_proc_validchan TCLARG;
static int tcl_proc_setudef TCLARG;
static int tcl_proc_valididx TCLARG;
static int tcl_proc_validuser TCLARG;

static char *tcl_var_botnick TCLVAR;
static char *tcl_var_botname TCLVAR;
static char *tcl_var_server TCLVAR;
static char *tcl_var_uptime TCLVAR;
static char *tcl_var_server_online TCLVAR;
static char *tcl_var_config TCLVAR;
static char *tcl_var_username TCLVAR;

static bool tcl_add (NetServer *);
static ModTcl *server2tcl (NetServer *);
static void tcl_event (NetServer *);
static void tcl_reply (NetServer *);
static void tcl_conf (NetServer *, c_char);
static void tcl_stop (void);
static void tcl_start (void);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "tcl",
  tcl_start,
  tcl_stop,
  tcl_conf,
  NULL
};

}

////////////////////
// class functions
////////////////////

ModTcl::ModTcl (NetServer *server) : timers (NULL), s (server),
  initialized (0), timerseq (0)
{
  interp = Tcl_CreateInterp ();
  Tcl_Init (interp);
  Tcl_CreateCommand (interp, "adduser", tcl_proc_adduser, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "assoc", tcl_proc_assoc, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "backup", tcl_proc_backup, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "bind", tcl_proc_bind, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "botisop", tcl_proc_botisop, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "botlist", tcl_proc_botlist, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "botonchan", tcl_proc_botonchan, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "chanlist", tcl_proc_chanlist, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "channels", tcl_proc_channels, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "chnick", tcl_proc_chnick, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "countusers", tcl_proc_countusers, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "ctime", tcl_proc_ctime, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "dccbroadcast", tcl_proc_dccbroadcast, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "dccdumpfile", tcl_proc_dccdumpfile, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "dccputchan", tcl_proc_dccputchan, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "dccused", tcl_proc_dccused, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "delhost", tcl_proc_delhost, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "deluser", tcl_proc_deluser, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "die", tcl_proc_die, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "dnslookup", tcl_proc_dnslookup, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "dumpfile", tcl_proc_dumpfile, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "finduser", tcl_proc_finduser, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "flushmode", tcl_proc_flushmode, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "getchan", tcl_proc_getchan, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "getchanhost", tcl_proc_getchanhost, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "getdccaway", tcl_proc_getdccaway, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "getdccidle", tcl_proc_getdccidle, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "getting-users", tcl_proc_getting_users, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "hand2idx", tcl_proc_hand2idx, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "hand2nick", tcl_proc_hand2nick, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "handonchan", tcl_proc_handonchan, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "isdynamic", tcl_proc_isdynamic, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "islinked", tcl_proc_islinked, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "isop", tcl_proc_isop, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "isvoice", tcl_proc_isvoice, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "killassoc", tcl_proc_killassoc, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "killdcc", tcl_proc_killdcc, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "killtimer", tcl_proc_killtimer, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "killutimer", tcl_proc_killutimer, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "maskhost", tcl_proc_maskhost, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "matchattr", tcl_proc_matchattr, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "myip", tcl_proc_myip, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "nick2hand", tcl_proc_nick2hand, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "onchan", tcl_proc_onchan, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "passwdok", tcl_proc_passwdok, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "pushmode", tcl_proc_pushmode, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "putcmdlog", tcl_proc_putcmdlog, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "putdcc", tcl_proc_putdcc, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "puthelp", tcl_proc_putserv, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "putquick", tcl_proc_putserv, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "putlog", tcl_proc_putlog, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "putloglev", tcl_proc_putloglev, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "putserv", tcl_proc_putserv, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "putxferlog", tcl_proc_putxferlog, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "rand", tcl_proc_rand, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "reload", tcl_proc_reload, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "resetchan", tcl_proc_resetchan, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "save", tcl_proc_save, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "setchan", tcl_proc_setchan, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "strftime", tcl_proc_strftime, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "topic", tcl_proc_topic, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "unixtime", tcl_proc_unixtime, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "userlist", tcl_proc_userlist, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "unames", tcl_proc_unames, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "timer", tcl_proc_timer, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "utimer", tcl_proc_utimer, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "timers", tcl_proc_timers, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "utimers", tcl_proc_utimers, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "validchan", tcl_proc_validchan, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "setudef", tcl_proc_setudef, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "valididx", tcl_proc_valididx, (ClientData)0, NULL);
  Tcl_CreateCommand (interp, "validuser", tcl_proc_validuser, (ClientData)0, NULL);

  Tcl_TraceVar (interp, "botnick", TCL_TRACE_READS, tcl_var_botnick, (ClientData)s);
  Tcl_TraceVar (interp, "botname", TCL_TRACE_READS, tcl_var_botname, (ClientData)s);
  Tcl_TraceVar (interp, "server", TCL_TRACE_READS, tcl_var_server, (ClientData)s);
  Tcl_TraceVar (interp, "uptime", TCL_TRACE_READS, tcl_var_uptime, (ClientData)s);
  Tcl_TraceVar (interp, "server-online", TCL_TRACE_READS, tcl_var_server_online, (ClientData)s);
  Tcl_TraceVar (interp, "config", TCL_TRACE_READS, tcl_var_config, (ClientData)s);
  Tcl_TraceVar (interp, "username", TCL_TRACE_READS, tcl_var_username, (ClientData)s);

  Tcl_SetVar (interp, "assoc-length", "512", 0);
  Tcl_SetVar (interp, "lastbind", "", 0);
  Tcl_SetVar (interp, "numversion", VERSION, 0);
  Tcl_SetVar (interp, "version", VERSION_STRING, 0);
  Tcl_SetVar (interp, "isjuped", "0", 0);
  Tcl_SetVar (interp, "handlen", "512", 0);
  initialized = 1;
}

ModTcl::~ModTcl (void)
{
  if (interp != NULL)
    Tcl_DeleteInterp (interp);
  bind_delall ();
  timer_delall ();
}

bool
ModTcl::loadscript (char *name)
{
  int i = Tcl_EvalFile (interp, name);
#ifdef HAVE_TCL_GETSTRINGRESULT
  if (i != 0)
    s->write_botlog ("%s", (c_char)Tcl_GetStringResult (interp));
#endif
  return (i == 0);
}

// convert eggdrop flags to mbot levels
int
ModTcl::attr2level (c_char attr)
{
  if (attr == NULL)
    return 0;
  // op: 0 = first, 1 = &, 2 = |
  int op = 0, level = 0, tmp;
  while (attr[0] != 0)
    {
      switch (*(attr++))
        {
          case '-': tmp = 0;	// everyone
                    break;
          case '&': op = 1;
                    break;
          case '|': op = 2;
                    break;
          // standard flags
          case 'm': tmp = 9;	// almost every feature
                    break;
          case 'n': tmp = 10;	// absolute control
                    break;
          case 't': tmp = 10;	// all features dealing with the botnet
                    break;
          case 'x': tmp = 3;	// can _send_ and receive files
                    break;
          case 'j': tmp = 9;	// like a "master" of the file area
                    break;
          case 'c': tmp = 0;	// didn't understand this one...
                    break;
          case 'p': tmp = 1;	// access to the party line
                    break;
          case 'b': tmp = 0;	// marks a user that is really a bot
                    break;
          case 'w': tmp = 0;	// user needs wasop test for +stopnethack procedure
                    break;
          case 'e': tmp = 0;	// user is exempted from stopnethack protection
                    break;
          case 'u': tmp = 0;	// user record is not sent to other bots
                    break;
          case 'h': tmp = 0;	// use nice bolds & inverse in the help files
                    break;
          // global or channel-specific flags
          case 'o': tmp = 5;	// ask for channel op status on the channel
                    break;
          case 'v': tmp = 3;	// ask for channel voice status on the channel
                    break;
          case 'd': tmp = -1;	// user cannot gain ops on any of the bot's channels
                    break;
          case 'k': tmp = -1;	// user is kicked and banned automatically
                    break;
          case 'f': tmp = 5;	// user is not punished for flooding, etc
                    break;
          case 'a': tmp = 5;	// auto-op
                    break;
          case 'g': tmp = 3;	// auto-voice
                    break;
          case 'q': tmp = -1;	// user does not get voice on +autovoice channels
                    break;
        }
      switch (op)
        {
          case 0: level = tmp;		// first
                  break;
          case 1: if (level > tmp)	// &
                    level = tmp;
                  break;
          case 2: if (level < tmp)	// |
                    level = tmp;
                  break;
        }
    }
  return level;
}

void
ModTcl::bind_add (int type, c_char attr, c_char command, c_char proc)
{
  bind_type *bind = new bind_type (type, attr2level (attr), command, proc);
  binds.add ((void *)bind);
}

// delete all binds with <type> and <command>
void
ModTcl::bind_del (int type, c_char command)
{
  bind_type *bind;
  binds.rewind ();
  while ((bind = (bind_type *)binds.next ()) != NULL)
    {
      if (bind->type == type && (bind->command |= command))
        {
          delete bind;
          binds.del ((void *)bind);
          binds.rewind ();
        }
    }
}

void
ModTcl::do_bind_join (bind_type *bind)
{
  snprintf (BUF[0], MSG_SIZE, "%s %s", CMD[2], CMD[0]);
  if (!match_mask (BUF[0], bind->command))
    return;
  Tcl_SetVar (interp, "lastbind", "join", 0);
  struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
  mask_split (CMD[0], BUF[0], BUF[1], BUF[2]);
  // procname <nick> <user@host> <handle> <channel>
  escape_bufs ((c_char)bind->proc, BUF[0], BUF[1], BUF[2],
               (u ? (c_char)u->mask : "*"), CMD[2]);
  snprintf (BUF[3], MSG_SIZE, "%s \"%s\" \"%s@%s\" \"%s\" \"%s\"",
            (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3,
            (c_char)buf4, (c_char)buf5);
  if (Tcl_Eval (interp, BUF[3]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::do_bind_part (bind_type *bind)
{
  snprintf (BUF[0], MSG_SIZE, "%s %s", CMD[2], CMD[0]);
  if (!match_mask (BUF[0], bind->command))
    return;
  Tcl_SetVar (interp, "lastbind", "part", 0);
  struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
  mask_split (CMD[0], BUF[0], BUF[1], BUF[2]);
  // procname <nick> <user@host> <handle> <channel> <msg>
  escape_bufs ((c_char)bind->proc, BUF[0], BUF[1], BUF[2],
               (u ? (c_char)u->mask : "*"), CMD[2], CMD[3]);
  if (CMD[3][0] == 0)
    snprintf (BUF[3], MSG_SIZE, "%s \"%s\" \"%s@%s\" \"%s\" \"%s\"",
              (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3,
              (c_char)buf4, (c_char)buf5);
  else
    snprintf (BUF[3], MSG_SIZE, "%s \"%s\" \"%s@%s\" \"%s\" \"%s\" \"%s\"",
              (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3,
              (c_char)buf4, (c_char)buf5, (c_char)buf6);
  if (Tcl_Eval (interp, BUF[3]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::do_bind_sign (bind_type *bind)
{
  bool matches = 0;
  int i;
  for (i = 0; i < s->channel_num; i++)
    {
      snprintf (BUF[0], MSG_SIZE, "%s %s", (c_char)s->channels[i]->name, CMD[0]);
      if (match_mask (BUF[0], bind->command))
        {
          matches = 1;
          break;
        }
    }
  if (!matches)
    return;
  Tcl_SetVar (interp, "lastbind", "sign", 0);
  struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
  mask_split (CMD[0], BUF[0], BUF[1], BUF[2]);
  // procname <nick> <user@host> <handle> <channel> <reason>
  escape_bufs ((c_char)bind->proc, BUF[0], BUF[1], BUF[2],
               (u ? (c_char)u->mask : "*"), (c_char)s->channels[i]->name,
               CMD[2]);
  snprintf (BUF[3], MSG_SIZE, "%s \"%s\" \"%s@%s\" \"%s\" \"%s\" \"%s\"",
            (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3,
            (c_char)buf4, (c_char)buf5, (c_char)buf6);
  if (Tcl_Eval (interp, BUF[3]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::do_bind_kick (bind_type *bind)
{
  snprintf (BUF[0], MSG_SIZE, "%s %s", CMD[2], CMD[3]);
  if (!match_mask (BUF[0], bind->command))
    return;
  Tcl_SetVar (interp, "lastbind", "kick", 0);
  struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
  mask_split (CMD[0], BUF[0], BUF[1], BUF[2]);
  // procname <nick> <user@host> <handle> <channel> <kicked-nick> <reason>
  escape_bufs ((c_char)bind->proc, BUF[0], BUF[1], BUF[2],
               (u ? (c_char)u->mask : "*"), CMD[2], CMD[3], CMD[4]);
  snprintf (BUF[3], MSG_SIZE, "%s \"%s\" \"%s@%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
            (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3,
            (c_char)buf4, (c_char)buf5, (c_char)buf6, (c_char)buf7);
  if (Tcl_Eval (interp, BUF[3]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::do_bind_nick (bind_type *bind)
{
  snprintf (BUF[0], MSG_SIZE, "%s %s", CMD[2], CMD[3]);
  if (!match_mask (BUF[0], bind->command))
    return;
  Tcl_SetVar (interp, "lastbind", "nick", 0);
  struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
  mask_split (CMD[0], BUF[0], BUF[1], BUF[2]);
  // procname <nick> <user@host> <handle> <channel> <newnick>
  escape_bufs ((c_char)bind->proc, BUF[0], BUF[1], BUF[2],
               (u ? (c_char)u->mask : "*"), BUF[4], CMD[2]);
  snprintf (BUF[3], MSG_SIZE, "%s \"%s\" \"%s@%s\" \"%s\" \"%s\" \"%s\"",
            (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3,
            (c_char)buf4, (c_char)buf5, (c_char)buf6);
  if (Tcl_Eval (interp, BUF[3]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::do_bind_dcc (bind_type *bind)
{
  strsplit (CMD[3], BUF, 1);
  if (!(bind->command |= BUF[0]))
    return;
  Tcl_SetVar (interp, "lastbind", "dcc", 0);
  struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
  // procname <handle> <idx> <args>
  escape_bufs ((c_char)bind->proc, (u ? (c_char)u->mask : "*"), CMD[2], BUF[1]);
  snprintf (BUF[6], MSG_SIZE, "%s \"%s\" \"%s\" \"%s\"",
            (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3);
  if (Tcl_Eval (interp, BUF[6]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::do_bind_chat (bind_type *bind)
{
  if (!match_mask (CMD[3], bind->command))
    return;
  Tcl_SetVar (interp, "lastbind", "chat", 0);
  struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
  // procname <nick> <channel#> <text>
  escape_bufs ((c_char)bind->proc, (u ? (c_char)u->mask : "*"), CMD[3]);
  snprintf (BUF[6], MSG_SIZE, "\"%s\" \"%s\" \"0\" \"%s\"",
            (c_char)buf0, (c_char)buf1, (c_char)buf2);
  if (Tcl_Eval (interp, BUF[6]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::do_bind_pub (bind_type *bind)
{
  strsplit (CMD[3], BUF, 1);
  if (!(bind->command |= BUF[0]))
    return;
  Tcl_SetVar (interp, "lastbind", "pub", 0);
  mask_split (CMD[0], BUF[3], BUF[4], BUF[5]);
  struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
  // procname <nick> <user@host> <handle> <channel> <text>
  escape_bufs ((c_char)bind->proc, BUF[3], BUF[4], BUF[5],
               (u ? (c_char)u->mask : "*"), CMD[2], BUF[1]);
  snprintf (BUF[6], MSG_SIZE, "%s \"%s\" \"%s@%s\" \"%s\" \"%s\" \"%s\"",
            (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3,
            (c_char)buf4, (c_char)buf5, (c_char)buf6);
  if (Tcl_Eval (interp, BUF[6]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::do_bind_pubm (bind_type *bind)
{
  snprintf (BUF[0], MSG_SIZE, "%s %s", CMD[2], CMD[3]);
  if (!match_mask (BUF[0], bind->command))
    return;
  Tcl_SetVar (interp, "lastbind", "pubm", 0);
  mask_split (CMD[0], BUF[3], BUF[4], BUF[5]);
  struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
  // procname <nick> <user@host> <handle> <channel> <text>
  escape_bufs ((c_char)bind->proc, BUF[3], BUF[4], BUF[5],
               (u ? (c_char)u->mask : "*"), CMD[2], CMD[3]);
  snprintf (BUF[6], MSG_SIZE, "%s \"%s\" \"%s@%s\" \"%s\" \"%s\" \"%s\"",
            (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3,
            (c_char)buf4, (c_char)buf5, (c_char)buf6);
  if (Tcl_Eval (interp, BUF[6]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::do_bind_msg (bind_type *bind)
{
  strsplit (CMD[3], BUF, 1);
  if (!match_mask (BUF[0], bind->command))
    return;
  Tcl_SetVar (interp, "lastbind", "msg", 0);
  mask_split (CMD[0], BUF[3], BUF[4], BUF[5]);
  struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
  // procname <nick> <user@host> <handle> <text>
  escape_bufs ((c_char)bind->proc, BUF[3], BUF[4], BUF[5],
               (u ? (c_char)u->mask : "*"), BUF[1]);
  snprintf (BUF[6], MSG_SIZE, "%s \"%s\" \"%s@%s\" \"%s\" \"%s\"",
            (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3,
            (c_char)buf4, (c_char)buf5);
  if (Tcl_Eval (interp, BUF[6]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::do_bind_msgm (bind_type *bind)
{
  if (!match_mask (CMD[3], bind->command))
    return;
  Tcl_SetVar (interp, "lastbind", "msgm", 0);
  mask_split (CMD[0], BUF[3], BUF[4], BUF[5]);
  struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
  // procname <nick> <user@host> <handle> <text>
  escape_bufs ((c_char)bind->proc, BUF[3], BUF[4], BUF[5],
               (u ? (c_char)u->mask : "*"), BUF[1]);
  snprintf (BUF[6], MSG_SIZE, "%s \"%s\" \"%s@%s\" \"%s\" \"%s\"",
            (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3,
            (c_char)buf4, (c_char)buf5);
  if (Tcl_Eval (interp, BUF[6]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::do_bind_notc (bind_type *bind)
{
  if (!match_mask (CMD[3], bind->command))
    return;
  Tcl_SetVar (interp, "lastbind", "notc", 0);
  mask_split (CMD[0], BUF[3], BUF[4], BUF[5]);
  struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
  // procname <nick> <user@host> <handle> <text> <dest>
  escape_bufs ((c_char)bind->proc, BUF[3], BUF[4], BUF[5],
               (u ? (c_char)u->mask : "*"), BUF[1], CMD[2]);
  snprintf (BUF[6], MSG_SIZE, "%s \"%s\" \"%s@%s\" \"%s\" \"%s\" \"%s\"",
            (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3,
            (c_char)buf4, (c_char)buf5, (c_char)buf6);
  if (Tcl_Eval (interp, BUF[6]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::do_bind_ctcp_ctcr (bind_type *bind)
{
  size_t len = strlen (CMD[3]);
  if (len < 3)		// must have \001<char>\001 at least
    return;
  memcpy (BUF[2], CMD[3]+1, len-2);
  BUF[2][len-2] = 0;
  strsplit (BUF[2], BUF, 1);
  if (!match_mask (BUF[0], bind->command))
    return;
  Tcl_SetVar (interp, "lastbind", (char *)bindtypes[bind->type].name, 0);
  mask_split (CMD[0], BUF[3], BUF[4], BUF[5]);
  struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
  // procname <nick> <user@host> <handle> <dest> <keyword> <text>
  escape_bufs ((c_char)bind->proc, BUF[3], BUF[4], BUF[5],
               (u ? (c_char)u->mask : "*"), CMD[2], BUF[0], BUF[1]);
  snprintf (BUF[6], MSG_SIZE, "%s \"%s\" \"%s@%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
            (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3,
            (c_char)buf4, (c_char)buf5, (c_char)buf6, (c_char)buf7);
  if (Tcl_Eval (interp, BUF[6]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::do_bind_raw (bind_type *bind)
{
  if (!match_mask (CMD[1], bind->command))
    return;
  BUF[0][0] = 0;
  bool first = 1;
  for (int i = 2; i <= s->cmd_i; i++)
    {
      if (first)
        first = 0;
      else
        strcat (BUF[0], " ");
      if (i == s->cmd_i)	// not entirely correct, but better than nothing
        strcat (BUF[0], ":");
      strcat (BUF[0], CMD[i]);
    }
  Tcl_SetVar (interp, "lastbind", "raw", 0);
  // procname <from> <keyword> <text>
  escape_bufs ((c_char)bind->proc, CMD[0], CMD[1], BUF[0]);
  snprintf (BUF[3], MSG_SIZE, "%s \"%s\" \"%s\" \"%s\"",
            (c_char)buf0, (c_char)buf1, (c_char)buf2, (c_char)buf3);
  if (Tcl_Eval (interp, BUF[3]) != 0)
    s->write_botlog ("%s", Tcl_GetVar (interp, "errorInfo", TCL_GLOBAL_ONLY));
}

void
ModTcl::event_join (void)
{
  int level = USERS.match_mask_level (CMD[0]);
  bind_type *bind;
  binds.rewind ();
  while ((bind = (bind_type *)binds.next ()) != NULL)
    if (bind->type == BIND_JOIN && level >= bind->level)
      do_bind_join (bind);
}

void
ModTcl::event_part (void)
{
  int level = USERS.match_mask_level (CMD[0]);
  bind_type *bind;
  binds.rewind ();
  while ((bind = (bind_type *)binds.next ()) != NULL)
    if (bind->type == BIND_PART && level >= bind->level)
      do_bind_part (bind);
}

void
ModTcl::event_quit (void)
{
  int level = USERS.match_mask_level (CMD[0]);
  bind_type *bind;
  binds.rewind ();
  while ((bind = (bind_type *)binds.next ()) != NULL)
    if (bind->type == BIND_SIGN && level >= bind->level)
      do_bind_sign (bind);
}

void
ModTcl::event_topic (void)
{
  int level = USERS.match_mask_level (CMD[0]);
  bind_type *bind;
  binds.rewind ();
  while ((bind = (bind_type *)binds.next ()) != NULL)
    if (bind->type == BIND_TOPC && level >= bind->level)
      do_bind_topc (bind);
}

void
ModTcl::event_kick (void)
{
  bind_type *bind;
  binds.rewind ();
  while ((bind = (bind_type *)binds.next ()) != NULL)
    if (bind->type == BIND_KICK)
      do_bind_kick (bind);
}

void
ModTcl::event_nick (void)
{
  int level = USERS.match_mask_level (CMD[0]);
  bind_type *bind;
  binds.rewind ();
  while ((bind = (bind_type *)binds.next ()) != NULL)
    if (bind->type == BIND_NICK && level >= bind->level)
      do_bind_nick (bind);
}


void
ModTcl::event_privmsg (void)
{
  int level = USERS.match_mask_level (CMD[0]);
  bind_type *bind;
  binds.rewind ();

  // ctcp
  if (CMD[3][0] == '\001')
    {
      while ((bind = (bind_type *)binds.next ()) != NULL)
        if (bind->type == BIND_CTCP && level >= bind->level)
          do_bind_ctcp_ctcr (bind);
      return;
    }

  // dcc chat
  if (isdigit (CMD[2][0]))
    {
      while ((bind = (bind_type *)binds.next ()) != NULL)
        {
          if (bind->type == BIND_CHAT)
            do_bind_chat (bind);
          else if (bind->type == BIND_DCC && level >= bind->level)
            do_bind_dcc (bind);
        }
      return;
    }

  // channel
  if (CMD[2][0] == '#')
    {
      while ((bind = (bind_type *)binds.next ()) != NULL)
        {
          if (level < bind->level)
            continue;
          if (bind->type == BIND_PUB)
            do_bind_pub (bind);
          else if (bind->type == BIND_PUBM)
            do_bind_pubm (bind);
        }
      return;
    }

  // pvt
  while ((bind = (bind_type *)binds.next ()) != NULL)
    {
      if (level < bind->level)
        continue;
      if (bind->type == BIND_MSG)
        do_bind_msg (bind);
      else if (bind->type == BIND_MSGM)
        do_bind_msgm (bind);
    }
}

void
ModTcl::event_notice (void)
{
  int level = USERS.match_mask_level (CMD[0]);
  bind_type *bind;
  binds.rewind ();

  // ctcp
  if (CMD[3][0] == '\001')
    {
      while ((bind = (bind_type *)binds.next ()) != NULL)
        if (bind->type == BIND_CTCR && level >= bind->level)
          do_bind_ctcp_ctcr (bind);
      return;
    }

  while ((bind = (bind_type *)binds.next ()) != NULL)
    if (bind->type == BIND_NOTC && level >= bind->level)
      do_bind_notc (bind);
}

void
ModTcl::bind_delall (void)
{
  bind_type *bind;
  binds.rewind ();
  while ((bind = (bind_type *)binds.next ()) != NULL)
    delete bind;
}

char *
ModTcl::timer_add (time_t time, c_char command, timer_enum type)
{
  timer_type *buf = timers;
  timers = new timer_type;
  timers->time = time;
  timers->command = NULL;
  strset (&timers->command, command, MSG_SIZE);
  snprintf (timers->timerid, TIMERID_SIZE, "timer%u", timerseq++);
  timers->type = type;
  timers->next = buf;
  return timers->timerid;
}

bool
ModTcl::timer_del (c_char timerid, timer_enum type)
{
  if (timers == NULL)
    return 0;
  timer_type *buf, *buf2;
  if (strcmp (timerid, timers->timerid) == 0 && type == timers->type)
    {
      buf = timers->next;
      free (timers->command);
      delete (timers);
      timers = buf;
      return 1;
    }

  for (buf = timers; buf->next != NULL; buf = buf->next)
    if (strcmp (timerid, buf->next->timerid) == 0 && type == timers->type)
      {
        buf2 = buf->next->next;
        free (buf->next->command);
        delete (buf->next);
        buf->next = buf2;
        return 1;
      }

  return 0;
}

void
ModTcl::timer_delall (void)
{
  timer_type *buf = timers, *buf2;
  while (buf != NULL)
    {
      buf2 = buf->next;
      free (buf->command);
      delete (buf);
      buf = buf2;
    }
}

// escape chars «[]{}\"» with a \. returns pointer to static
// escaped string which gets overwritten in each call. DON'T free() it!
// call with str=NULL to free result
char *
ModTcl::escape_chars (c_char str)
{
  const char chars[] = "[]{}\\\"";
  static char *result = NULL;
  if (result != NULL)
    free (result);
  if (str == NULL)
    {
      result = NULL;
      return NULL;
    }
  size_t len = strlen (str), count = 0;
  for (size_t i = 0; i < len; i++)
    if (strchr (chars, str[i]) != NULL)
      count++;
  result = (char *)malloc (len+count+1);
  count = 0;
  for (size_t i = 0; i < len; i++)
    {
      if (strchr (chars, str[i]) != NULL)
        result[count++] = '\\';
      result[count++] = str[i];
    }
  result[count] = 0;
  return result;
}

// escape each b* to buf*, only b0 is required
void
ModTcl::escape_bufs (c_char b0, c_char b1, c_char b2, c_char b3, c_char b4,
                   c_char b5, c_char b6, c_char b7)
{
  buf0 = escape_chars (b0);
  if (b1 == NULL)
    return;
  buf1 = escape_chars (b1);
  if (b2 == NULL)
    return;
  buf2 = escape_chars (b2);
  if (b3 == NULL)
    return;
  buf3 = escape_chars (b3);
  if (b4 == NULL)
    return;
  buf4 = escape_chars (b4);
  if (b5 == NULL)
    return;
  buf5 = escape_chars (b5);
  if (b6 == NULL)
    return;
  buf6 = escape_chars (b6);
  if (b7 == NULL)
    return;
  buf7 = escape_chars (b7);
}

/////////////////
// tcl commands
/////////////////

// get the tcl object using the given interpreter
ModTcl *
interp2tcl (Tcl_Interp *interp)
{
  ModTcl *tcl;
  tcl_list->rewind ();
  while ((tcl = (ModTcl *)tcl_list->next ()) != NULL)
    if (tcl->interp != NULL && interp != NULL
        && memcmp (tcl->interp, interp, sizeof (Tcl_Interp)) == 0)
      return tcl;
  return NULL;
}

bool
check_args (ModTcl *tcl, int argc_used, int argc_required)
{
  if (tcl == NULL)
    return 0;
  if (argc_used < argc_required)
    {
      Tcl_AppendResult (tcl->interp, "wrong number of arguments", NULL);
      return 0;
    }
  return 1;
}

static void
tcl_bind_error (NetServer *s, c_char bind)
{
  s->write_botlog ("WARNING: tcl bind '%s' is not implemented!", bind);
}

static int
tcl_proc_adduser TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  mask2user (argv[2], tcl->BUF[0]);
  mask2host (argv[2], tcl->BUF[1]);
  snprintf (tcl->BUF[2], MSG_SIZE, "%s!%s@%s", argv[1], tcl->BUF[0], tcl->BUF[1]);
  if (tcl->USERS.abs_mask2user (tcl->BUF[2]) == NULL)
    Tcl_AppendResult (interp, "0", NULL);
  else
    Tcl_AppendResult (interp, "1", NULL);
  return TCL_OK;
}

static int
tcl_proc_assoc TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2) && !check_args (tcl, argc, 3))
    return TCL_ERROR;
  return TCL_OK;
}

static int
tcl_proc_backup TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  my_strncpy (tcl->BUF[0], tcl->USERS.filename, MSG_SIZE);
  snprintf (tcl->BUF[1], MSG_SIZE, "%s.backup", tcl->BUF[0]);
  tcl->USERS.filename = tcl->BUF[1];
  tcl->USERS.save_users ();
  tcl->USERS.filename = tcl->BUF[0];
  return TCL_OK;
}

static int
tcl_proc_bind TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 5))
    return TCL_ERROR;

  for (int i = 0; bindtypes[i].name != NULL; i++)
    {
      if (strcasecmp (argv[1], bindtypes[i].name) != 0)
        continue;
      if (!bindtypes[i].stackable)
        tcl->bind_del (i, argv[3]);
      tcl->bind_add (i, argv[2], argv[3], argv[4]);
      return TCL_OK;
    }
  tcl->s->write_botlog ("WARNING: unknown tcl bind '%s'.", argv[1]);
  return TCL_OK;
}

static int
tcl_proc_chanlist TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  for (int i = 0; i < tcl->s->channel_num; i++)
    if (tcl->CHANNELS[i]->name |= argv[1])
      {
        for (int i2 = 0; i2 < tcl->CHANNELS[i]->user_num; i2++)
          Tcl_AppendElement (interp, tcl->CHANNELS[i]->users[i2]->nick.getstr ());
        return TCL_OK;
      }
  Tcl_AppendResult (interp, "nonexistant channel", NULL);
  return TCL_ERROR;
}

static int
tcl_proc_channels TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  for (int i = 0; i < tcl->s->channel_num; i++)
    Tcl_AppendElement (interp, tcl->CHANNELS[i]->name.getstr ());
  return TCL_OK;
}

static int
tcl_proc_botisop TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  int i = tcl->CHANNEL_INDEX (argv[1]);
  Channel::user_type *u;
  if (i != -1
      && (u = tcl->CHANNELS[i]->user_get (tcl->s->nick)) != NULL
      && u->is_op)
    Tcl_AppendResult (interp, "1", NULL);
  else
    Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_botlist TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  return TCL_OK;
}

static int
tcl_proc_botonchan TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  int i = tcl->CHANNEL_INDEX (argv[1]);
  if (i != -1
      && tcl->CHANNELS[i]->user_get (tcl->s->nick) != NULL)
    Tcl_AppendResult (interp, "1", NULL);
  else
    Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_chnick TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  if (tcl->USERS.abs_set_mask (argv[1], argv[2]))
    Tcl_AppendResult (interp, "1", NULL);
  else
    Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_countusers TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  itoa (tcl->USERS.count (), tcl->BUF[0], MSG_SIZE);
  Tcl_AppendResult (interp, tcl->BUF[0], NULL);
  return TCL_OK;
}

static int
tcl_proc_ctime TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  Tcl_AppendResult (interp, get_asctime (atol (argv[1]), tcl->BUF[0], MSG_SIZE), NULL);
  return TCL_OK;
}

static int
tcl_proc_dccbroadcast TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  tcl->s->script.send_partyline ("%s", argv[1]);
  return TCL_OK;
}

static int
tcl_proc_dccdumpfile TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  tcl->s->write_botlog ("WARNING: tcl command 'dccdumpfile' sends any file!");
  int i = atoi (argv[1]);
  if (i < 0 || i >= tcl->DCC_NUM)
    return TCL_OK;
  Text file;
  if (!file.read_file (argv[2]))
    return TCL_OK;
  c_char line;
  while ((line = file.get_line ()) != NULL)
    tcl->DCCS[i]->writeln ("%s", line);
  return TCL_OK;
}

static int
tcl_proc_dccputchan TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  tcl->s->script.send_partyline ("(channel %s) %s", argv[1], argv[2]);
  return TCL_OK;
}

static int
tcl_proc_dccused TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  Tcl_AppendResult (interp, itoa (tcl->DCC_NUM, tcl->BUF[0], MSG_SIZE), NULL);
  return TCL_OK;
}

static int
tcl_proc_delhost TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_deluser TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  if (tcl->USERS.del_user (argv[1]))
    Tcl_AppendResult (interp, "1", NULL);
  else
    Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_die TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1) && !check_args (tcl, argc, 2))
    return TCL_ERROR;
  for (int i = 0; i < bot->server_num; i++)
    if (argc == 1)
      bot->servers[i]->irc_quit ("EXIT");
    else
      bot->servers[i]->irc_quit (argv[1]);
  mbot_exit ();
  return TCL_OK;
}

static int
tcl_proc_dnslookup TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  return TCL_OK;
}

static int
tcl_proc_dumpfile TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  tcl->s->write_botlog ("WARNING: tcl command 'dumpfile' sends any file!");
  Text file;
  if (!file.read_file (argv[2]))
    {
      Tcl_AppendResult (interp, "can't open file", NULL);
      return TCL_ERROR;
    }
  const char *line;
  while ((line = file.get_line ()) != NULL)
    {
      if (line[0] == 0)
        continue;
      tcl->SEND_TEXT (argv[1], "%s", line);
    }
  return TCL_OK;
}

static int
tcl_proc_finduser TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  ListUsers::user_type *u = tcl->USERS.match_mask2user (argv[1]);
  if (u == NULL)
    Tcl_AppendResult (interp, "*", NULL);
  else
    Tcl_AppendResult (interp, (c_char)u->mask, NULL);
  return TCL_OK;
}

static int
tcl_proc_flushmode TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  return TCL_OK;
}

static int
tcl_proc_getchan TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_getchanhost TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  int i = tcl->s->channel_index (argv[2]);
  if (i == -1)
    return TCL_OK;
  for (int i2 = 0; i2 < tcl->CHANNELS[i]->user_num; i2++)
    {
      if (!(tcl->CHANNELS[i]->users[i2]->nick |= argv[1]))
        continue;
      mask2user (tcl->CHANNELS[i]->users[i2]->mask, tcl->BUF[0]);
      mask2host (tcl->CHANNELS[i]->users[i2]->mask, tcl->BUF[1]);
      snprintf (tcl->BUF[2], MSG_SIZE, "%s@%s", tcl->BUF[0], tcl->BUF[1]);
      Tcl_AppendResult (interp, tcl->BUF[2], NULL);
      return TCL_OK;
    }
  return TCL_OK;
}

static int
tcl_proc_getdccaway TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  return TCL_OK;
}

static int
tcl_proc_getdccidle TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  int i = atoi (argv[1]);
  if (i >= 0 && i < tcl->DCC_NUM)
    Tcl_AppendResult (interp, utoa (tcl->s->time_now - tcl->DCCS[i]->time_read, tcl->BUF[0], MSG_SIZE), NULL);
  else
    Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_getting_users TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_hand2idx TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  ListUsers::user_type *u;
  for (int i = 0; i < tcl->DCC_NUM; i++)
    {
      if (tcl->DCCS[i]->status != DCC_CHAT)
        continue;
      u = tcl->USERS.match_mask2user (tcl->DCCS[i]->mask);
      if (u->mask |= argv[1])
        {
          Tcl_AppendResult (interp, itoa (i, tcl->BUF[0], MSG_SIZE), NULL);
          return TCL_OK;
        }
    }
  Tcl_AppendResult (interp, "-1", NULL);
  return TCL_OK;
}

static int
tcl_proc_hand2nick TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  int i = tcl->s->channel_index (argv[2]);
  if (i != -1)
    for (int i2 = 0; i2 < tcl->CHANNELS[i]->user_num; i2++)
      if (match_mask (tcl->CHANNELS[i]->users[i2]->mask, argv[1]))
        {
          Tcl_AppendResult (interp, (c_char)tcl->CHANNELS[i]->users[i2]->nick, NULL);
          return TCL_OK;
        }
  Tcl_AppendResult (interp, "", NULL);
  return TCL_OK;
}

static int
tcl_proc_handonchan TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  int i = tcl->s->channel_index (argv[2]);
  if (i != -1)
    for (int i2 = 0; i2 < tcl->CHANNELS[i]->user_num; i2++)
      if (match_mask (tcl->CHANNELS[i]->users[i2]->mask, argv[1]))
        {
          Tcl_AppendResult (interp, "1", NULL);
          return TCL_OK;
        }
  Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_isdynamic TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_islinked TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_isop TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  int i = tcl->CHANNEL_INDEX (argv[2]);
  Channel::user_type *u;
  if (i != -1
      && (u = tcl->CHANNELS[i]->user_get (argv[1])) != NULL
      && u->is_op)
    Tcl_AppendResult (interp, "1", NULL);
  else
    Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_isvoice TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  int i = tcl->CHANNEL_INDEX (argv[2]);
  Channel::user_type *u;
  if (i != -1
      && (u = tcl->CHANNELS[i]->user_get (argv[1])) != NULL
      && u->is_voice)
    Tcl_AppendResult (interp, "1", NULL);
  else
    Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_killassoc TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  return TCL_OK;
}

static int
tcl_proc_killdcc TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  int i = atoi (argv[1]);
  if (i >= 0 && i < tcl->DCC_NUM)
    tcl->DCCS[i]->dcc_stop ();
  return TCL_OK;
}

static int
tcl_proc_killtimer TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  if (!tcl->timer_del (argv[1], TCL_TIMER))
    {
      Tcl_AppendResult (interp, "invalid timer", NULL);
      return TCL_ERROR;
    }
  return TCL_OK;
}

static int
tcl_proc_killutimer TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  if (!tcl->timer_del (argv[1], TCL_UTIMER))
    {
      Tcl_AppendResult (interp, "invalid timer", NULL);
      return TCL_ERROR;
    }
  return TCL_OK;
}

static int
tcl_proc_maskhost TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  mask_split (argv[1], tcl->BUF[0], tcl->BUF[1], tcl->BUF[2]);
  // make sure we always have a correct mask
  snprintf (tcl->BUF[3], MASK_SIZE, "%s!%s@%s", tcl->BUF[0], tcl->BUF[1], tcl->BUF[2]);
  make_generic_mask (tcl->BUF[3], tcl->BUF[4]);
  Tcl_AppendResult (interp, tcl->BUF[4], NULL);
  return TCL_OK;
}

static int
tcl_proc_matchattr TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  int userlevel = tcl->USERS.match_mask_level (argv[1]);
  int flagslevel = tcl->attr2level (argv[2]);
  if ((userlevel >= flagslevel && flagslevel != -1)
      || (userlevel == -1 && flagslevel == -1))
    Tcl_AppendResult (interp, "1", NULL);
  else
    Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_myip TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  utoa (Net::sock2addr (tcl->s->sock), tcl->BUF[0], MSG_SIZE);
  Tcl_AppendResult (interp, tcl->BUF[0], NULL);
  return TCL_OK;
}

static int
tcl_proc_nick2hand TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2) && !check_args (tcl, argc, 3))
    return TCL_ERROR;

  int i = 0, i2;
  if (argc == 3)
    {
      // if the channel doesn't exist, don't enter the for() ahead
      i2 = tcl->s->channel_index (argv[2]) + 1;
      if (i2 != 0)
        i = i2 - 1;
    }
  else
    i2 = tcl->s->channel_num;

  Channel::user_type *u;
  for (; i < i2; i++)
    {
      u = tcl->CHANNELS[i]->user_get (argv[1]);
      if (u == NULL)
        continue;
      ListUsers::user_type *u2 = tcl->USERS.match_mask2user (u->mask);
      if (u2 == NULL)
        Tcl_AppendResult (interp, "*", NULL);
      else
        Tcl_AppendResult (interp, (c_char)u2->mask, NULL);
      return TCL_OK;
    }

  Tcl_AppendResult (interp, "", NULL);
  return TCL_OK;
}

static int
tcl_proc_onchan TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  int i = tcl->CHANNEL_INDEX (argv[2]);
  if (i != -1
      && tcl->CHANNELS[i]->user_get (argv[1]) != NULL)
    Tcl_AppendResult (interp, "1", NULL);
  else
    Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_passwdok TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  if (tcl->USERS.abs_check_pass (argv[1], argv[2]))
    Tcl_AppendResult (interp, "1", NULL);
  else
    Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_pushmode TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3) && !check_args (tcl, argc, 4))
    return TCL_ERROR;
  if (argc == 3)
    tcl->s->writeln ("MODE %s %s", argv[1], argv[2]);
  else
    tcl->s->writeln ("MODE %s %s %s", argv[1], argv[2], argv[3]);
  return TCL_OK;
}

static int
tcl_proc_putcmdlog TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  tcl->s->write_botlog ("tcl command: %s", argv[1]);
  return TCL_OK;
}

static int
tcl_proc_putdcc TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  tcl->s->script.send_text (argv[1], "%s", argv[2]);
  return TCL_OK;
}

static int
tcl_proc_putlog TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  tcl->s->write_botlog ("tcl misc: %s", argv[1]);
  return TCL_OK;
}

static int
tcl_proc_putloglev TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 4))
    return TCL_ERROR;
  tcl->s->write_botlog ("tcl %s %s: %s", argv[1], argv[2], argv[3]);
  return TCL_OK;
}

static int
tcl_proc_putserv TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  tcl->s->writeln ("%s", argv[1]);
  return TCL_OK;
}

static int
tcl_proc_putxferlog TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  tcl->s->write_botlog ("tcl file-area: %s", argv[1]);
  return TCL_OK;
}

static int
tcl_proc_rand TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  snprintf (tcl->BUF[0], 50, "%lu", (u_long) random_num (atoi (argv[1])));
  Tcl_AppendResult (interp, tcl->BUF[0], NULL);
  return TCL_OK;
}

static int
tcl_proc_reload TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  tcl->USERS.load_users ();
  return TCL_OK;
}

static int
tcl_proc_resetchan TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  tcl->s->write_botlog ("WARNING: tcl command 'resetchan' does nothing!");
  return TCL_OK;
}

static int
tcl_proc_save TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  tcl->USERS.save_users ();
  return TCL_OK;
}

static int
tcl_proc_setchan TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  return TCL_OK;
}

static int
tcl_proc_strftime TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2) && !check_args (tcl, argc, 3))
    return TCL_ERROR;
  time_t time;
  if (argc == 3)
    time = atol (argv[2]);
  else
    time = tcl->s->time_now;
  struct tm *t = localtime (&time);
  if (strftime (tcl->BUF[0], MSG_SIZE, argv[1], t) == 0)
    Tcl_AppendResult (interp, "", NULL);
  else
    Tcl_AppendResult (interp, tcl->BUF[0], NULL);
  return TCL_OK;
}

static int
tcl_proc_topic TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  int i = tcl->CHANNEL_INDEX (argv[1]);
  if (i != -1)
    Tcl_AppendResult (interp, (c_char)tcl->CHANNELS[i]->topic, NULL);
  return TCL_OK;
}

static int
tcl_proc_unixtime TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  Tcl_AppendResult (interp, utoa (tcl->s->time_now, tcl->BUF[0], MSG_SIZE), NULL);
  return TCL_OK;
}

static int
tcl_proc_userlist TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1) && !check_args (tcl, argc, 2))
    return TCL_ERROR;
  int level = LEVEL_MIN;
  if (argc == 2)
    level = tcl->attr2level (argv[1]);
  ListUsers::user_type *u;
  tcl->USERS.rewind ();
  while ((u = (ListUsers::user_type *)tcl->USERS.next ()) != NULL)
    if (u->level >= level)
      Tcl_AppendElement (interp, u->mask.getstr ());
  return TCL_OK;
}

static int
tcl_proc_unames TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  Tcl_AppendResult (interp, HOST, NULL);
  return TCL_OK;
}

static int
tcl_proc_timer TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  char *timerid = tcl->timer_add (tcl->s->time_now + atoi (argv[1])*60, argv[2], TCL_TIMER);
  Tcl_AppendResult (interp, timerid, NULL);
  return TCL_OK;
}

static int
tcl_proc_utimer TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  char *timerid = tcl->timer_add (tcl->s->time_now + atoi (argv[1]), argv[2], TCL_UTIMER);
  Tcl_AppendResult (interp, timerid, NULL);
  return TCL_OK;
}

static int
tcl_proc_timers TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  if (tcl->timers == NULL)
    return 0;

  char mins[10], *xv[3], *x;
  for (ModTcl::timer_type *buf = tcl->timers; buf != NULL; buf = buf->next)
    if (buf->type == TCL_TIMER)
      {
        snprintf (mins, sizeof mins, "%u", (buf->time - tcl->s->time_now)/60);
        xv[0] = mins;
        xv[1] = buf->command;
        xv[2] = buf->timerid;
        x = Tcl_Merge (3, xv);
        Tcl_AppendElement (interp, x);
        Tcl_Free ((char *)x);
      }
  return TCL_OK;
}

static int
tcl_proc_utimers TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 1))
    return TCL_ERROR;
  if (tcl->timers == NULL)
    return 0;

  char mins[10], *xv[3], *x;
  for (ModTcl::timer_type *buf = tcl->timers; buf != NULL; buf = buf->next)
    if (buf->type == TCL_UTIMER)
      {
        snprintf (mins, sizeof mins, "%u", buf->time - tcl->s->time_now);
        xv[0] = mins;
        xv[1] = buf->command;
        xv[2] = buf->timerid;
        x = Tcl_Merge (3, xv);
        Tcl_AppendElement (interp, x);
        Tcl_Free ((char *)x);
      }
  return TCL_OK;
}

static int
tcl_proc_validchan TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  if (tcl->s->channel_get (argv[1]) == NULL)
    Tcl_AppendResult (interp, "0", NULL);
  else
    Tcl_AppendResult (interp, "1", NULL);
  return TCL_OK;
}

static int
tcl_proc_setudef TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 3))
    return TCL_ERROR;
  tcl->s->write_botlog ("ERROR: tcl command 'setudef' is not implemented!");
  return TCL_ERROR;	// not implemented
}

static int
tcl_proc_valididx TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  int i = atoi (argv[1]);
  if (i >= 0 && i < tcl->DCC_NUM)
    Tcl_AppendResult (interp, "1", NULL);
  else
    Tcl_AppendResult (interp, "0", NULL);
  return TCL_OK;
}

static int
tcl_proc_validuser TCLARG
{
  ModTcl *tcl = interp2tcl (interp);
  if (!check_args (tcl, argc, 2))
    return TCL_ERROR;
  if (tcl->USERS.abs_mask2user (argv[1]) == NULL)
    Tcl_AppendResult (interp, "0", NULL);
  else
    Tcl_AppendResult (interp, "1", NULL);
  return TCL_OK;
}

/*
  it's better to have local scrap variables for tcl_var_* funcs because 
  they can be called from any (including our) tcl procs, which might already
  use s->buf.
*/

static char *
tcl_var_botnick TCLVAR
{
  Tcl_SetVar (interp, "botnick", ((NetServer *)clientData)->nick.getstr (), 0);
  return TCL_OK;
}

static char *
tcl_var_botname TCLVAR
{
  Tcl_SetVar (interp, "botname", ((NetServer *)clientData)->name.getstr (), 0);
  return TCL_OK;
}

static char *
tcl_var_server TCLVAR
{
  char buf[MSG_SIZE+1];
  snprintf (buf, MSG_SIZE, "%s:",
    ((NetServer *)clientData)->host_current->host.getstr ());
  utoa (((NetServer *)clientData)->host_current->port, buf+strlen (buf), MSG_SIZE-strlen (buf));
  Tcl_SetVar (interp, "server", buf, 0);
  return TCL_OK;
}

static char *
tcl_var_uptime TCLVAR
{
  char buf[MSG_SIZE+1];
  utoa (((NetServer *)clientData)->uptime, buf, MSG_SIZE);
  Tcl_SetVar (interp, "uptime", buf, 0);
  return TCL_OK;
}

static char *
tcl_var_server_online TCLVAR
{
  char buf[MSG_SIZE+1];
  utoa (((NetServer *)clientData)->uptime, buf, MSG_SIZE);
  Tcl_SetVar (interp, "server-online", buf, 0);
  return TCL_OK;
}

static char *
tcl_var_config TCLVAR
{
  Tcl_SetVar (interp, "config", ((NetServer *)clientData)->b->conf_file.getstr (), 0);
  return TCL_OK;
}

static char *
tcl_var_username TCLVAR
{
  Tcl_SetVar (interp, "username", ((NetServer *)clientData)->user.getstr (), 0);
  return TCL_OK;
}

////////////////////
// module managing
////////////////////

// add a tcl to the list
static bool
tcl_add (NetServer *s)
{
  ModTcl *tcl = new ModTcl (s);
  tcl_list->add ((void *)tcl);
  return tcl->initialized;
}

// return the tcl for a given server, NULL if nonexistant
static
ModTcl *server2tcl (NetServer *s)
{
  ModTcl *tcl;
  tcl_list->rewind ();
  while ((tcl = (ModTcl *)tcl_list->next ()) != NULL)
    if (tcl->s == s)
      return tcl;
  return NULL;
}

// events handler
static void
tcl_event (NetServer *s)
{
  ModTcl *tcl = server2tcl (s);
  if (tcl == NULL)
    return;

  switch (EVENT)
    {
      case EVENT_JOIN:
        tcl->event_join ();
        break;
      case EVENT_PART:
        tcl->event_part ();
        break;
      case EVENT_TOPIC:
        tcl->event_topic ();
        break;
      case EVENT_QUIT:
        tcl->event_quit ();
        break;
      case EVENT_KICK:
        tcl->event_kick ();
        break;
      case EVENT_NICK:
        tcl->event_nick ();
        break;
      case EVENT_MODE:
        // XXX yep...
        break;
      case EVENT_PRIVMSG:
        tcl->event_privmsg ();
        break;
      case EVENT_NOTICE:
        tcl->event_notice ();
        break;
    }

  ModTcl::bind_type *bind;
  tcl->binds.rewind ();
  while ((bind = (ModTcl::bind_type *)tcl->binds.next ()) != NULL)
    if (bind->type == BIND_RAW)
      tcl->do_bind_raw (bind);
}

// replies handler
static void
tcl_reply (NetServer *s)
{
  ModTcl *tcl = server2tcl (s);
  if (tcl == NULL)
    return;
}

// timer handler
static void
tcl_timer (NetServer *s)
{
  ModTcl *tcl = server2tcl (s);
  if (tcl == NULL)
    return;
  ModTcl::timer_type *buf = tcl->timers;
  while (buf != NULL)
    {
      if (s->time_now >= buf->time)
        {
          if (Tcl_Eval (tcl->interp, buf->command) != 0)
            tcl->s->write_botlog ("%s", Tcl_GetVar (tcl->interp, "errorInfo", TCL_GLOBAL_ONLY));
          tcl->timer_del (buf->timerid, buf->type);
        }
      buf = buf->next;
    }
}

// !tcl <load | eval> <args>
static void
tcl_cmd_tcl (NetServer *s)
{
  ModTcl *tcl = server2tcl (s);
  if (tcl == NULL)
    return;
  strsplit (CMD[3], BUF, 2);
  if (BUF[2][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_TCL);
      return;
    }
  if (strcasecmp (BUF[1], "load") == 0)
    {
      if (tcl->loadscript (BUF[2]))
        SEND_TEXT (DEST, "Script loaded.");
      else
        SEND_TEXT (DEST, "Error loading script.");
    }
  else if (strcasecmp (BUF[1], "eval") == 0)
    {
      if (Tcl_Eval (tcl->interp, BUF[2]) != 0)
        tcl->s->write_botlog ("%s", Tcl_GetVar (tcl->interp, "errorInfo", TCL_GLOBAL_ONLY));
    }
  else
    SEND_TEXT (DEST, "Unknown tcl option.");
}

// configuration file's local parser
static void
tcl_conf (NetServer *s, c_char bufread)
{
  char buf[3][MSG_SIZE+1];

  strsplit (bufread, buf, 2);

  if (strcasecmp (buf[0], "bot") == 0)
    {
      ModTcl *tcl = server2tcl (s);
      if (tcl != NULL)		// if there's already a tcl for this server
        return;
      if (!tcl_add (s))
        bot->conf_error ("failed tcl initialization");
      s->script.events.add ((void *)tcl_event);
      s->script.replies.add ((void *)tcl_reply);
      s->script.timers.add ((void *)tcl_timer);
      s->script.cmd_bind (tcl_cmd_tcl, LEVEL_TCL, "!tcl", module.mod, HELP_TCL);
    }

  else if (strcasecmp (buf[0], "tcl") == 0)
    {
      ModTcl *tcl = server2tcl (s);
      if (tcl == NULL)
        bot->conf_error ("tcl interpreter not set for this server");
      else
        if (!tcl->loadscript (buf[1]))
          bot->conf_error ("can't load tcl script");
    }
}

// module termination
static void
tcl_stop (void)
{
  ModTcl *tcl;
  tcl_list->rewind ();
  while ((tcl = (ModTcl *)tcl_list->next ()) != NULL)
    {
      tcl->s->script.events.del ((void *)tcl_event);
      tcl->s->script.replies.del ((void *)tcl_reply);
      tcl->s->script.timers.del ((void *)tcl_timer);
      delete tcl;
    }
  delete tcl_list;
}

// module initialization
static void
tcl_start (void)
{
  tcl_list = new List;
}

