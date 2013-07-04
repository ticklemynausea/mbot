#ifndef _H_SCRIPT
#define _H_SCRIPT

#include "List.h"
#include "Strings.h"
#include "defines.h"

class Module;
class NetServer;

// shortcuts

#define CMD s->cmd
#define REPLY s->reply
#define EVENT s->event
#define USERS s->users
#define CHANNEL_INDEX s->channel_index
#define CHANNELS s->channels
#define DCCS s->dccs
#define DCC_NUM s->dcc_num

// server replies

#define INIT			001
#define RPL_ISUPPORT		005
#define RPL_AWAY		301
#define RPL_WHOISUSER		311
#define RPL_WHOISSERVER		312
#define RPL_WHOISOPERATOR	313
#define RPL_ENDOFWHO		315
#define RPL_WHOISIDLE		317
#define RPL_ENDOFWHOIS		318
#define RPL_WHOISCHANNELS	319
#define RPL_NOTOPIC		331
#define RPL_TOPIC		332
#define RPL_WHOREPLY		352
#define ERR_NOSUCHNICK		401
#define ERR_NOSUCHCHANNEL	403
#define ERR_TOOMANYCHANNELS	405
#define ERR_UNKNOWNCOMMAND	421
#define ERR_NICKNAMEINUSE	433
#define ERR_INVITEONLYCHAN	473
#define ERR_BANNEDFROMCHAN	474
#define ERR_CHANOPRIVSNEEDED	482
#define RPL_WATCHONLINE		600
#define RPL_WATCHOFFLINE	601
#define RPL_WATCHSTOP		602
#define RPL_WATCHSTARTONLINE	604
#define RPL_WATCHSTARTOFFLINE	605

// server events

#define EVENT_PING	1
#define EVENT_ERROR	2
#define EVENT_NOTICE	3
#define EVENT_JOIN	4
#define EVENT_NICK	5
#define EVENT_KICK	6
#define EVENT_PART	7
#define EVENT_MODE	8
#define EVENT_PRIVMSG	9
#define EVENT_TOPIC	10
#define EVENT_QUIT	11

// these are here to avoid repeating them
#define SCRIPT_HURT_BOT "I'm not _that_ dumb, wiseguy.. :P"
#define SCRIPT_INVALID_CHAN "I'm not on that channel."

class Script {
public:

  EXPORT Script (NetServer *);
  EXPORT ~Script (void);

  struct cmd_type {
    String name;
    int level;
    void (*action)(NetServer *);
    Module *module;
    c_char help;
    cmd_type (c_char n, int l, void (*a)(NetServer *), Module *m, c_char h) :
      name (n, MSG_SIZE), level (l), action (a), module (m), help (h) {}
  };

  EXPORT void cmd_bind (void (*)(NetServer *), int, c_char, Module *, c_char);
  EXPORT bool cmd_unbind (c_char);
  EXPORT cmd_type *cmd_get (c_char);

  EXPORT void send_partyline (c_char, ...);
  EXPORT void send_text (c_char, c_char, ...);

  void irc_reply (void);
  void irc_event (void);

  bool server_jumped;	// true if !server was used (event_error() uses this)

  char source[HOST_SIZE+1],		// these 2 are for privmsg
       dest[CHANNEL_SIZE+1];

  String ctcp_version,
         ctcp_finger,
         ctcp_source;

  char buf[10][MSG_SIZE+1];

  List cmds,				// commands list (for pvt and dcc)
       events, replies, timers;		// list of handlers

  NetServer *s;			// server to which belongs

private:

  void reply_init (void);
  void reply_ison (void);
  void reply_notopic (void);
  void reply_topic (void);
  void reply_whoreply (void);
  void reply_endofwho (void);
  void reply_nosuchnick (void);
  void reply_nosuchchannel (void);
  void reply_toomanychannels (void);
  void reply_unknowncommand (void);
  void reply_nicknameinuse (void);
  void reply_inviteonlychan (void);
  void reply_bannedfromchan (void);
  void reply_chanoprivsneeded (void);
  void reply_watchoffline (void);

  void event_ping (void);
  void event_error (void);
  void event_notice (void);
  void event_join (void);
  void event_quit (void);
  void event_nick (void);
  void event_kick (void);
  void event_part (void);
  void event_mode (void);
  void event_privmsg (void);
  void event_topic (void);

};

#include "mbot.h"
#include "scriptcmd.h"

#endif	// _H_SCRIPT
