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

#define LEVEL_LEARN_MINUS 7
#define LEVEL_LEARN_PLUS 7
#define LEVEL_LEARN 7
#define LEVEL_FORGET 7
#define LEVEL_WORDCOUNT 0
#define LEVEL_WORD2TEXT 0

#define HELP_LEARN_MINUS "!learn- word num \002-\002 Delete <num> words from <word>'s definition, starting from the end."
#define HELP_LEARN_PLUS "!learn+ word text \002-\002 Add <text> to <word>'s definition. If <word> doesn't exist, create it."
#define HELP_LEARN "!learn word [text] \002-\002 Add or replace <word> with <text>. If <text> is not especified, delete <word> instead (if it exists)."
#define HELP_FORGET "!forget word \002-\002 Delete the especified <word>."
#define HELP_WORDCOUNT "!wordcount \002-\002 Say the number of words defined."
#define HELP_WORD2TEXT "?? word \002-\002 Search <word> in the database and return its content."

#define HASH_SIZE 1000			// hash table's size
#define WORD_SIZE 30			// dictionary word's size
#define TEXT_SIZE MSG_SIZE		// dictionary definition's size
#define NOTDEF_DEFAULT "Not defined... lousy word anyway :D"

#define DEST s->script.dest
#define BUF s->script.buf
#define SEND_TEXT s->script.send_text

/////////////////////
// class definition
/////////////////////

class ModWord {
public:
  ModWord (NetServer *, c_char );
  ~ModWord (void);

  struct word_type {
    String word;		// key word
    long index;			// its position on the file
    word_type *next;
    word_type (c_char w, long i) : word (w, WORD_SIZE), index (i),
      next (NULL) {}
  };
  struct word_type *words[HASH_SIZE+1];

  void word2text (c_char, char *, size_t);
  word_type *get_word (c_char) const;
  void add_word (c_char, c_char);
  int del_word (c_char);

  String filename,		// name of the word file
         wordnotdef;		// msg to show when a word is not defined
  int word_num;			// number of words
  bool notice;			// wether to send notice or privmsg
  Log *log;
  List server_list;		// servers to which belongs

private:

  void open_words (void);
  void setup_words (void);
  void register_word (c_char, long);
  void unregister_word (c_char);
  void delete_words (void);
  u_int hash (c_char, u_int) const;

  fstream f;
  char buf[BUF_SIZE+1];
  Bot *b;
};

List *word_list;

///////////////
// prototypes
///////////////

static void word_cmd_learn_minus (NetServer *);
static void word_cmd_learn_plus (NetServer *);
static void word_cmd_learn (NetServer *);
static void word_cmd_forget (NetServer *);
static void word_cmd_wordcount (NetServer *);
static void word_cmd_word2text (NetServer *);

extern "C" {
EXPORT void server2wordtext (NetServer *, c_char, char *, size_t);
}

static bool learn_minus_num (char *str, size_t num);
static ModWord *file2word (c_char);
static ModWord *server2word (NetServer *);
static void word_add (NetServer *, c_char);
static void word_conf (NetServer *, c_char);
static void word_stop (void);
static void word_start (void);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "word",
  word_start,
  word_stop,
  word_conf,
  NULL
};

}

////////////////////
// class functions
////////////////////

ModWord::ModWord (NetServer *server, c_char name) : filename (name, PATH_MAX),
  wordnotdef (NOTDEF_DEFAULT, MSG_SIZE), word_num (0), notice (0),
  log (NULL), b (server->b)
{
  server_list.add ((void *)server);
  open_words ();
  setup_words ();
}

ModWord::~ModWord (void)
{
  delete_words ();
}

void
ModWord::open_words (void)
{
#ifdef HAS_IOS_BINARY
  f.open (filename, ios::in | ios::out | IOS_NOCREATE | ios::binary);
#else
  f.open (filename, ios::in | ios::out | IOS_NOCREATE);
#endif
  if (!f)
    {
      b->write_botlog ("error opening %s: %s", (c_char)filename, strerror (errno));
      mbot_exit ();
    }
}

// read the word file into the table
void
ModWord::setup_words (void)
{
  for (int i = 0; i < HASH_SIZE; i++)
    words[i] = NULL;
  word_num = 0;
  f.clear ();
  f.seekg (0);
  int f_index;
  while (1)
    {
      f_index = f.tellg ();
      if (!f.getline (buf, BUF_SIZE))
        break;
      buf[num_notspaces (buf)] = 0;		// buf now has the word
      register_word (buf, f_index);
    }
}

// put the word on the table
void
ModWord::register_word (c_char word, long f_index)
{
  int i = hash (word, HASH_SIZE);
  word_type *w = words[i], *w_previous = NULL;

  if (w == NULL)			// if the list is empty
    {
      words[i] = new word_type (word, f_index);
      word_num++;
      return;
    }

  while (w != NULL)			// else, find the end of the table
    {
      w_previous = w;
      w = w->next;
    }
  w = new word_type (word, f_index);
  w_previous->next = w;
  word_num++;
}

// delete the word from the table
void
ModWord::unregister_word (c_char word)
{
  int i = hash (word, HASH_SIZE);
  word_type *w = words[i], *w_next = NULL;
  if (w == NULL)
    return;

  if (w->word |= word)			// if it's the first
    {
      words[i] = w->next;
      delete w;
      word_num--;
      return;
    }

  while (w->next != NULL)		// else find it
    {
      if (strcasecmp (w->next->word, word) == 0)
        {
          w_next = w->next->next;
          delete w->next;
          w->next = w_next;
          word_num--;
          return;
        }
      w = w->next;
    }
}

// delete everything in the word table
void
ModWord::delete_words (void)
{
  word_type *w, *w2;
  for (int i = 0; i < HASH_SIZE; i++)
    {
      w = words[i];
      words[i] = NULL;
      while (w != NULL)
        {
          w2 = w->next;
          delete w;
          w = w2;
        }
    }
}

// put in text the definition of word, or \0 if nonexistant
void
ModWord::word2text (c_char word, char *text, size_t textlen)
{
  word_type *w = get_word (word);
  if (w != NULL)
    {
      f.clear ();
      f.seekg (w->index);
      f.getline (buf, BUF_SIZE);
      my_strncpy (text, buf + strlen (word) + 1, textlen);
      return;
    }
  text[0] = 0;
}

// return pointer to word, NULL if nonexistant
struct ModWord::word_type *
ModWord::get_word (c_char word) const
{
  word_type *w;
  w = words[hash (word, HASH_SIZE)];
  while (w != NULL)
    {
      if (w->word |= word)
        return w;
      w = w->next;
    }
  return NULL;
}

// create a new word at the end of file
void
ModWord::add_word (c_char word, c_char text)
{
  if (text[0] == 0)
    return;
  f.clear ();
  f.seekp (0, ios::end);
  register_word (word, f.tellp ());
  f << word << ' ' << text << '\n';
  f.flush ();
}

// delete a word.
// return 0 on nonexistent word, 1 on temporary file error, 2 on success
int
ModWord::del_word (c_char word)
{
  word_type *w = get_word (word);
  if (w == NULL)
    return 0;
  // delete from file
  f.clear ();
  f.seekg (w->index);		// offset of word to delete
  f.getline (buf, BUF_SIZE);	// move a line forward
  fstreamtmp tmp;
  if (!tmp.is_open ())
    {
      b->write_botlog ("error creating temporary file: %s", strerror (errno));
      return 1;
    }
  while (f.getline (buf, BUF_SIZE))	// copy the rest to tmp
    tmp << buf << '\n';
  f.clear ();
  f.seekp (w->index);
  tmp.seekg (0);
  while (tmp.getline (buf, BUF_SIZE))
    f << buf << '\n';
  f.clear ();
  long filepos = f.tellp ();
  f.close ();
  truncate (filename, filepos);
  open_words ();

  // delete from memory
  unregister_word (word);

  // rearrange other word's index
  f.seekg (0);
  int pos = 0, wordread = 0;
  while (wordread < word_num)
    {
      f.getline (buf, BUF_SIZE);
      buf[num_notspaces (buf)] = 0;	// buf now has the word
      w = get_word (buf);
      if (w != NULL)
        w->index = pos;
      pos = f.tellg (); 
      wordread++;
    }
  return 2;
}

u_int
ModWord::hash (c_char st, u_int hash_max) const
{
  size_t i, s = strlen (st), r = 0;
  for (i = 0; i < s; i++)
    r = r + toupper (st[i]);
  return r % hash_max;
}

/////////////
// commands
/////////////

// !learn- word num
static void
word_cmd_learn_minus (NetServer *s)
{
  ModWord *w = server2word (s);
  if (w == NULL)
    return;
  strsplit (CMD[3], BUF, 3);
  if (BUF[2][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_LEARN_MINUS);
      return;
    }
  if (strlen (BUF[1]) > WORD_SIZE)
    {
      SEND_TEXT (DEST, "Word \002%s\002 too large.", BUF[1]);
      return;
    }
  int num = atoi (BUF[2]);
  if (num <= 0)
    {
      SEND_TEXT (DEST, "Invalid number \002%d\002.", num);
      return;
    }
  if (w->log != NULL)
    *w->log << CMD[2] << ' ' << CMD[0] << ' ' << CMD[3] << EOL;
  w->word2text (BUF[1], BUF[3], MSG_SIZE);	// get the previous one
  if (BUF[3][0] == 0)
    {
      SEND_TEXT (DEST, "Word \002%s\002 doesn't exist.", BUF[1]);
      return;
    }
  if (!learn_minus_num (BUF[3], num))
    {
      SEND_TEXT (DEST, "Error changing word \002%s\002.", BUF[1]);
      return;
    }
  if (w->del_word (BUF[1]) != 2)
    {
      SEND_TEXT (DEST, "Error deleting word \002%s\002, maybe temporary file creation failed.", BUF[1]);
      return;
    }
  w->add_word (BUF[1], BUF[3]);
  SEND_TEXT (DEST, "Word \002%s\002 changed.", BUF[1]);
}

// !learn+ word text
static void
word_cmd_learn_plus (NetServer *s)
{
  ModWord *w = server2word (s);
  if (w == NULL)
    return;
  strsplit (CMD[3], BUF, 2);
  if (BUF[2][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_LEARN_PLUS);
      return;
    }
  if (strlen (BUF[1]) > WORD_SIZE)
    {
      SEND_TEXT (DEST, "Word \002%s\002 too large.", BUF[1]);
      return;
    }
  if (w->log != NULL)
    *w->log << CMD[2] << ' ' << CMD[0] << ' ' << CMD[3] << EOL;
  w->word2text (BUF[1], BUF[3], MSG_SIZE);	// get the previous one
  if (BUF[3][0] != 0)				// if it exists
    {
      if (w->del_word (BUF[1]) != 2)
      {
        SEND_TEXT (DEST, "Error deleting word \002%s\002, maybe temporary file creation failed.", BUF[1]);
        return;
      }
      SEND_TEXT (DEST, "Word \002%s\002 changed.", BUF[1]);
    }
  else
    SEND_TEXT (DEST, "Word \002%s\002 added.", BUF[1]);
  snprintf (BUF[4], MSG_SIZE, "%s %s", BUF[3], BUF[2]);
  w->add_word (BUF[1], BUF[4]);
}

// !learn word [text]
static void
word_cmd_learn (NetServer *s)
{
  ModWord *w = server2word (s);
  if (w == NULL)
    return;
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_LEARN);
      return;
    }
  if (strlen (BUF[1]) > WORD_SIZE)
    {
      SEND_TEXT (DEST, "Word \002%s\002 too large.", BUF[1]);
      return;
    }
  if (w->log != NULL)
    *w->log << CMD[2] << ' ' << CMD[0] << ' ' << CMD[3] << EOL;
  if (w->get_word (BUF[1]) != NULL)	// if it already exists
    {
      if (w->del_word (BUF[1]) != 2)
      {
        SEND_TEXT (DEST, "Error deleting word \002%s\002, maybe temporary file creation failed.", BUF[1]);
        return;
      }
      if (BUF[2][0] == 0)
        SEND_TEXT (DEST, "Word \002%s\002 deleted.", BUF[1]);
      else
        SEND_TEXT (DEST, "Word \002%s\002 changed.", BUF[1]);
    }
  else
    {
      if (BUF[2][0] == 0)
        {
          SEND_TEXT (DEST, "Word \002%s\002 doesn't exist.", BUF[1]);
          return;
        }
      SEND_TEXT (DEST, "Word \002%s\002 added.", BUF[1]);
    }
  w->add_word (BUF[1], BUF[2]);
}

// !forget word
static void
word_cmd_forget (NetServer *s)
{
  ModWord *w = server2word (s);
  if (w == NULL)
    return;
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_FORGET);
      return;
    }
  int result = w->del_word (BUF[1]);
  if (result == 0)
    SEND_TEXT (DEST, "Word \002%s\002 doesn't exist.", BUF[1]);
  else if (result == 1)
    SEND_TEXT (DEST, "Error deleting word \002%s\002, maybe temporary file creation failed.", BUF[1]);
  else
    {
      if (w->log != NULL)
        *w->log << CMD[2] << ' ' << CMD[0] << ' ' << CMD[3] << EOL;
      SEND_TEXT (DEST, "Word \002%s\002 deleted.", BUF[1]);
    }
}

// !wordcount
static void
word_cmd_wordcount (NetServer *s)
{
  ModWord *w = server2word (s);
  if (w != NULL)
    SEND_TEXT (DEST, "There are %d words defined.", w->word_num);
}

// ?? word
static void
word_cmd_word2text (NetServer *s)
{
  ModWord *w = server2word (s);
  if (w == NULL)
    return;
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    return;
  w->word2text (BUF[1], BUF[2], MSG_SIZE);
  if (BUF[2][0] == 0)
    my_strncpy (BUF[2], w->wordnotdef, MSG_SIZE);
  if (w->notice)		// XXX broken in dcc chat!
    s->irc_notice (DEST, "\002%s\002 == %s", BUF[1], BUF[2]);
  else
    SEND_TEXT (DEST, "%s", BUF[2]);
}

////////////////////
// module managing
////////////////////

extern "C" {

void
server2wordtext (NetServer *s, c_char word, char *text, size_t textlen)
{
  ModWord *w = server2word (s);
  if (w == NULL)
    text[0] = 0;
  else
    w->word2text (word, text, textlen);
}

}

// remove <num> words from <str>, return <str>
// strlen(str) and <num> are garanteed to be >=1
static bool
learn_minus_num (char *str, size_t num)
{
  size_t count = 0, pos = strlen (str);
  bool in_word = 1;
  // trim spaces from end
  while (str[--pos] == ' ')
    if (pos == 0)		// it was only spaces? shouldn't happen!
      return 0;
  // ready for words
  for (; pos != 0; pos--)
    {
      if (str[pos] == ' ')
        {
          in_word = 0;
          continue;
        }
      if (!in_word)		// new word found
        {
          in_word = 1;
          if (++count == num)
            {
              str[pos+1] = 0;
              return 1;
            }
        }
    }
  return 0;
}

// returns the word object for a given word file, NULL if nonexistant
static ModWord *
file2word (c_char filename)
{
  ModWord *w;
  word_list->rewind ();
  while ((w = (ModWord *)word_list->next ()) != NULL)
    if (strcmp (w->filename, filename) == 0)
      return w;
  return NULL;
}

// returns the word object for a given server, NULL if nonexistant
static ModWord *
server2word (NetServer *s)
{
  ModWord *w;
  word_list->rewind ();
  while ((w = (ModWord *)word_list->next ()) != NULL)
    if (w->server_list.exist ((void *)s))
      return w;
  return NULL;
}

// adds a server/channel pair to the list
static void
word_add (NetServer *s, c_char name)
{
  // check if a word using <name> already exists
  ModWord *word = file2word (name);
  if (word != NULL)
    {
      word->server_list.add ((void *)s);
      return;
    }
  // if not, create it
  word_list->add ((void *)new ModWord (s, name));
}

// configuration file's local parser
static void
word_conf (NetServer *s, c_char bufread)
{
  char buf[3][MSG_SIZE+1];

  strsplit (bufread, buf, 2);

  if (strcasecmp (buf[0], "words") == 0)
    {
      if (buf[1][0] == 0)
        s->b->conf_error ("sintax error: use \"words wordfile\"");
      if (server2word (s) != NULL)
        s->b->conf_error ("words already defined for this server");
      word_add (s, buf[1]);
      s->script.cmd_bind (word_cmd_learn_minus, LEVEL_LEARN_MINUS, "!learn-", module.mod, HELP_LEARN_MINUS);
      s->script.cmd_bind (word_cmd_learn_plus, LEVEL_LEARN_PLUS, "!learn+", module.mod, HELP_LEARN_PLUS);
      s->script.cmd_bind (word_cmd_learn, LEVEL_LEARN, "!learn", module.mod, HELP_LEARN);
      s->script.cmd_bind (word_cmd_forget, LEVEL_FORGET, "!forget", module.mod, HELP_FORGET);
      s->script.cmd_bind (word_cmd_wordcount, LEVEL_WORDCOUNT, "!wordcount", module.mod, HELP_WORDCOUNT);
      s->script.cmd_bind (word_cmd_word2text, LEVEL_WORD2TEXT, "??", module.mod, HELP_WORD2TEXT);
    }

  else if (strcasecmp (buf[0], "wordnotice") == 0)
    {
      ModWord *w = server2word (s);
      if (w != NULL)
        w->notice = 1;
      else
        s->b->conf_error ("words not yet defined for this server");
    }

  else if (strcasecmp (buf[0], "wordnotdef") == 0)
    {
      ModWord *w = server2word (s);
      if (w == NULL)
        s->b->conf_error ("words not yet defined for this server");
      strsplit (bufread, buf, 1);
      w->wordnotdef = buf[1];
    }

  else if (strcasecmp (buf[0], "wordlog") == 0)
    {
      ModWord *w = server2word (s);
      if (w == NULL)
        s->b->conf_error ("words not yet defined for this server");
      w->log = s->b->log_get (buf[1]);
      if (w->log == NULL)
        {
          snprintf (buf[0], MSG_SIZE, "inexistant loghandle: %s", buf[1]);
          s->b->conf_error (buf[0]);
        }
    }
}

// module termination
static void
word_stop (void)
{
  ModWord *w;
  word_list->rewind ();
  while ((w = (ModWord *)word_list->next ()) != NULL)
    delete w;
  delete word_list;
}

// module initialization
static void
word_start (void)
{
  word_list = new List ();
}

