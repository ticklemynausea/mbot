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
#define SEND_TEXT s->script.send_text

#define MAX_VARNAME_SIZE 40		// max size the name of a var can be
#define MAX_VARVAL_SIZE MSG_SIZE	// max size the value of a var can be
#define MAX_LINE_SIZE 16384		// max size a line of text can be
#define MAX_RANDLIST_SIZE 200		// max # of <li></li> in a random
#define MAX_SRAI 20			// max <srai> followed in a response

/////////////////////
// class definition
/////////////////////

class ModAlice {
public:

  ModAlice (NetServer *, c_char, c_char, Log *);
  ~ModAlice (void);

  // Breaks lines, cleans up text and logs, then sends to respond2
  char *respond (char *);
  int setvar (bool, c_char, c_char);
  // saves internal variables to file
  void savevars (c_char);
  // saves default vars to a new file. Also loads defaults into memory
  void loadvars (c_char);

  NetServer *s;
  bool initialized;

private:

  c_char getvar (c_char varname, c_char vardefault);
  // checks a line of AIML for syntax errors
  int checkline (char *line);
  // splits the aiml files into patterns.txt and templates.txt
  int prepare (c_char);

  // makes alice's answer pretty. (cleans up text)
  void print (char *line);
  // makes substitutions on text
  void substitute (Text &subst, char *text);
  // gets a response
  char *respond2 (char *text);

  void replacer (char *line);
  int splitaiml (c_char, fstream &);
  int match (c_char text, c_char pattern, int final);
  void randomize (char *text);
  void reevaluate (char *line );

  struct var_array { 
    bool protect;
    int def;
    String varname;
    String value;
    var_array *next;
    var_array (bool p, int d, c_char vn, c_char v, var_array *n) : protect (p),
      def (d), varname (vn, MAX_VARNAME_SIZE), value (v, MAX_VARVAL_SIZE),
      next (n) {}
  };

  /* star holds "*" and "_", that holds our that, pbuffer is sort of
     a general scrap variable, although it does eventually hold our answer. */
  char star[MAX_LINE_SIZE+1], that[MAX_LINE_SIZE+1];
  char pbuffer[MAX_LINE_SIZE+1];

  // best holds the pattern matched
  String best;

  // char's for our init file
  fstream templatefile, patternfile;
  Text personfile, person2file, substitutefile, personffile;
  String datafile;

  // should we log?
  bool logging, log_error;
  Log *log;

  // new variables for the getvar setvar code
  var_array *root_var;

  // file positions for each letter
  long chunk[28];

  // these 4 are for checkline()
  int cc;		// checkcount
  bool hp,		// have we had a pattern?
       hh,		// had that?
       he;		// had template?

  /* topic holds our topic which we convert to conditional tags. */
  /* this isn't coded yet */
  char topic[MAX_VARVAL_SIZE+1];

  String vars;

  int srai_num;		// number of <srai> tags followed in a response

};

List *alice_list;

///////////////
// prototypes
///////////////

// removes all spaces including '\t' '\v' ...
static void spacetrim (char *line);
// finds string from end
static char *strlstr (char *line, c_char token);
static void stripurl (char *line );
// replaces string1 with string2 in line
static int replace (char *line, c_char string1, c_char string2);
// removes all the text from first to last
static void strremove (char *text, c_char first, c_char last);
// cleans up text
static void cleaner (char *text);

static ModAlice *server2alice (NetServer *);
static void alice_event (NetServer *);
static void alice_conf (NetServer *, c_char);
static void alice_stop (void);
static void alice_start (void);

extern "C" {

EXPORT struct Module::module_type module = {
  MODULE_VERSION,
  "alice",
  alice_start,
  alice_stop,
  alice_conf,
  NULL
};

}

//////////////////
// alice helpers
//////////////////

// delete spaces from the beginning and end of <line>
void
spacetrim (char *line)
{
  int i = 0;
  while (isspace (line[i]))
    i++;
  if (i != 0)
    strcpy (line, line+i);
  while ((i = strlen (line)-1) >= 0)
    if (isspace (line[i]))
      line[i] = 0;
    else
      break;
}

char *
strlstr (char *line, c_char token)
{
  char *pos = NULL;
  while ((line = strstr (line, token)))
    {
      pos = line;
      line++;
    }
  return pos;
}

void
stripurl (char *line)
{
  strremove (line, "<a href=", ">");
  replace (line, "</a>", "");
  strremove (line, "<A HREF=", ">");
  replace (line, "</A>", "");
  while (replace (line, "<br>", ""));
  while (replace (line, "<BR>", ""));
  replace (line, "<ul>", "");
  while (replace (line, "<li>", ""));
  while (replace (line, "</li>", ""));
  replace (line, "</ul>", "");
  strremove (line, "<APPLET", "</APPLET>");
  strremove (line, "<applet", "</applet>");        
  replace (line, "&lt;", "<");
  replace (line, "&gt;", ">");
  replace (line, "\n", "");
}

void
strremove (char *text, c_char first, c_char last)
{
  char *pos1, *pos2;

  if (!first || !last || !text)
    return;
  if (!(pos1 = strstr (text, first)))
    return;
  if (!(pos2 = strstr (pos1, last)))
    return;
  if (pos1 >= pos2)
    return;
  pos2 += strlen (last);
  memmove (pos1, pos2, strlen (pos2) + 1);
}

int
replace (char *string, c_char strfrom, c_char strto)
{
  char *pos, *tmp;
  size_t len, len2;

  if (!string || !strfrom || !strto)
    return 0;
  pos = strstr (string, strfrom);
  if (!pos)
    return 0;

  len = strlen (strfrom);
  len2 = strlen (strto);

  if (len > len2)
    {
      tmp = pos + len - len2;
      memmove (pos, tmp, strlen (tmp) + 1);
    }
  if (len < len2)
    {
      char buf[MAX_LINE_SIZE+1];
      int buflen;

      tmp = pos + len;
      buflen = strlen (tmp) + 1;
      memcpy (buf, tmp, buflen);
      memcpy (pos + len2, buf, buflen);
    }

  strncpy (pos, strto, len2);
  return 1;
}

void
cleaner (char *text)
{
  char symb;
  char *oldtext = text;

  while ((symb = *text))
    if (isalnum (symb) || symb == ' ' || symb == '-')
      text++;
    else
      memmove (text, text + 1, strlen (text) + 1);
  spacetrim (oldtext);
}

////////////////////
// class functions
////////////////////

ModAlice::ModAlice (NetServer *server, c_char init_file, c_char v, Log *l) :
  s (server), initialized (0), best (PATH_MAX), datafile (PATH_MAX),
  log (l), root_var (NULL), cc (0), hp (0), hh (0), he (0), vars (v, PATH_MAX)
{
  StringFixed aimlscrap (PATH_MAX), patternscrap (PATH_MAX),
               templatescrap (PATH_MAX);

  // load up the init file
  ifstream init (init_file);
  if (!init)
    {
      s->write_botlog ("can't open init file %s: %s", init_file, strerror (errno));
      return;
    }
  // we use pbuffer as a scrap variable
  char *comment, *epos;
  while (init.getline (pbuffer, MAX_LINE_SIZE))
    {
      // eliminate comments
      comment = strchr (pbuffer, '#');
      if (comment != NULL)
        comment[0] = 0;
      // delete all spaces
      while (replace (pbuffer, " ", ""));
      strip_crlf (pbuffer);
      epos = strchr (pbuffer, '=');
      epos++;
      if (strstr (pbuffer, "aimlfile"))
        aimlscrap = epos;
      else if (strstr (pbuffer, "datafile"))
        datafile = epos;
      else if (strstr (pbuffer, "personfile")
               && !personfile.read_file (epos))
        {
          s->write_botlog ("error opening personfile %s: %s", epos, strerror (errno));
          return;
        }
      else if (strstr (pbuffer, "person2file")
               && !person2file.read_file (epos))
        {
          s->write_botlog ("error opening person2file %s: %s", epos, strerror (errno));
          return;
        }
      else if (strstr (pbuffer, "personffile")
               && !personffile.read_file (epos))
        {
          s->write_botlog ("error opening personffile %s: %s", epos, strerror (errno));
          return;
        }
      else if (strstr (pbuffer, "substitutefile")
               && !substitutefile.read_file (epos))
        {
          s->write_botlog ("error opening substitutefile %s: %s", epos, strerror (errno));
          return;
        }
      else if (strstr (pbuffer, "patternfile"))
        patternscrap = epos;
      else if (strstr (pbuffer, "templatefile"))
        templatescrap = epos;
      else if (strstr (pbuffer, "logging"))
        {
          if (*epos == 'y' || *epos == 'Y' || *epos == '1')
            logging = 1;
          else 
            logging = 0;
        }
      else if (strstr (pbuffer, "log_error"))
        {
          if (*epos == 'y' || *epos == 'Y' || *epos == '1')
            log_error = 1; 
          else 
            log_error = 0;
        }
    }

  // check to see if it needs to create new patterns.txt and templates.txt

#ifdef HAS_IOS_BINARY
  patternfile.open (patternscrap, ios::in | ios::out | IOS_NOCREATE | ios::binary);
  templatefile.open (templatescrap, ios::in | ios::out | IOS_NOCREATE | ios::binary);
#else
  patternfile.open (patternscrap, ios::in | ios::out | IOS_NOCREATE);
  templatefile.open (templatescrap, ios::in | ios::out | IOS_NOCREATE);
#endif

  if (!patternfile || !templatefile)
    {
      if (!(!patternfile))
        patternfile.close (); 
      if (!(!templatefile))
        templatefile.close ();
      patternfile.clear ();
#ifdef HAS_IOS_BINARY
      patternfile.open (patternscrap, ios::in | ios::out | ios::trunc | ios::binary);
#else
      patternfile.open (patternscrap, ios::in | ios::out | ios::trunc);
#endif
      if (!patternfile)
        {
          s->write_botlog ("error creating patternfile %s: %s", (c_char)patternscrap, strerror (errno));
          return;
        }
      templatefile.clear ();
#ifdef HAS_IOS_BINARY
      templatefile.open (templatescrap, ios::in | ios::out | ios::trunc | ios::binary);
#else
      templatefile.open (templatescrap, ios::in | ios::out | ios::trunc);
#endif
      if (!templatefile)
        {
          s->write_botlog ("error creating templatefile %s: %s", (c_char)templatescrap, strerror (errno));
          return;
        } 
      if (!prepare (aimlscrap))
        return;
      patternfile.clear ();
      patternfile.seekg (0);
      templatefile.clear ();
      templatefile.seekg(0);
    }
  else
    {
      // ok, this part really should be saved to a file upon preparation
      char last = 0;
      while (1)
        {
          long line;
          line = patternfile.tellg ();
          if (!patternfile.getline (pbuffer, MAX_LINE_SIZE))
            break;
          char first = pbuffer[0];
          if ((first < 'A') || (first > 'Z'))
            first = 'Z' + 1;
          if (first != last)
            {
              last = first;
              if ((last < 'A') || (last > 'Z'))
                last = 'Z' + 1;
              chunk[last - 'A'] = line;
            }
        }
    }
  loadvars (vars);
  initialized = 1;
}

ModAlice::~ModAlice (void)
{
  savevars (vars);
  var_array *varsnext;
  while (root_var != NULL)
    {
      varsnext = root_var->next;
      delete root_var;
      root_var = varsnext;
    }
}

int
ModAlice::setvar (bool protect, c_char var, c_char value)
{
  String varname (var, MAX_VARNAME_SIZE);
  varname.lower ();
  if (root_var == NULL)	// the variable list is empty, create the first
    {
      if (value[0] == '*')
        root_var = new var_array (protect, 1, varname, value+1, NULL);
      else
        root_var = new var_array (protect, 0, varname, value, NULL);
      if (root_var == NULL)
        return 0;
      if (root_var->def == 0)
        return 1;
      return 2;
    } 

  // we search through the list for the name
  var_array *conductor = root_var;
  while (conductor != NULL)
    {
      if (conductor->varname == varname)
        {
          if (protect == 1 || conductor->protect == protect)
            conductor->value = value;
          else
            return 0;
          if (conductor->def == 0)
            return 1;
          return 2;
        }
      conductor = conductor->next;
    }
  // if we get here, the variable doesn't exist, create a new entry
  if (value[0] == '*')
    conductor = new var_array (protect, 1, varname, value+1, root_var);
  else
    conductor = new var_array (protect, 0, varname, value, root_var);
  if (conductor == NULL)
    return 0;
  root_var = conductor;
  if (conductor->def == 0)
    return 1;
  return 2;     
}  

/*
  return <var>'s text. if <var> doesn't exist, return <vardefault>, and if
  it's not NULL, also create <var> with it
*/
c_char
ModAlice::getvar (c_char var, c_char vardefault)
{
  if (strlen (var) > MAX_VARNAME_SIZE)
    return NULL;
  String varname (var, MAX_VARNAME_SIZE);
  varname.lower ();

  // we begin searching through the linked list
  var_array *conductor = root_var;
  while (conductor != NULL)
    {
      if (conductor->varname == varname)
        return conductor->value;
      conductor = conductor->next;
    }  
  // if we made it to here, the variable doesn't exist
  if (vardefault == NULL)
    return NULL;
  // create a new with the default value
  setvar (0, varname, vardefault);
  return vardefault;
}

char *
ModAlice::respond (char *text)
{
  /* Thanks to Dr. Wallace for writing this into the CGI,
     it was stripped out of there, modified slightly, 
     then added to here. */
  char scrap[MAX_LINE_SIZE+1];
  char *ltimestr, *uip, *strtmp;
  struct tm *ltime;
  time_t clock;

  time (&clock);
  ltime = localtime (&clock); 
  ltimestr = asctime (ltime);

  srai_num = 0;
  my_strncpy (scrap, text, MAX_LINE_SIZE);
  substitute (substitutefile, scrap);
  /* perhaps some code to deal with remaining periods that are not
     sentence breaks.  (Like 5.02 or H.I.P.P.i.e.) */
  while (replace (scrap, "?", "."));
  while (replace (scrap, "!", "."));
  while (replace (scrap, "\n", ""));
  // we need to make sure our sentence ends in a period
  if (scrap[strlen (scrap)-2] != '.')
    strncat (scrap, ".", MAX_LINE_SIZE-strlen (scrap));
  uip = scrap;
  char oneline[MAX_LINE_SIZE+1];
  char output[MAX_LINE_SIZE+1];
  
  output[0] = 0;
  do
    {
      memset (oneline, 0, MAX_LINE_SIZE);
      // skip white space:
      while (uip[0] == ' ')
        uip++;
      my_strncpy (oneline, uip, MAX_LINE_SIZE);
      if (strstr (uip, ".") > 0)
        {
          // advance to next sentence:
          uip = strstr (uip, ".")+1;
          // strip out first sentence:
          strtmp = strstr (oneline, ".");
          if (strtmp != NULL)
            strtmp[0] = 0;
        }   
      if (oneline != NULL && strlen (oneline) > 0)
        {
          // Robot Respose:
          cleaner (oneline);
          strncat (output, respond2 (oneline), MAX_LINE_SIZE-strlen (output));
          if (logging && log != NULL)
            {
              *log << CMD[0] << ": " << oneline << EOL;
              *log << s->nick << ": " << output << EOL;
            }
          strncat (output, "  ", MAX_LINE_SIZE-strlen (output));
        }
    }
  while (strstr (uip, ".") > 0); // any more sentences?
  strcpy (pbuffer, output);
  print (pbuffer);
  return pbuffer;
}

void
ModAlice::savevars (c_char varfile)
{
  ofstream f (varfile, ios::out | ios::trunc);
  // first we print out that
  f << that << '\n';
  // we print out each variable
  var_array *conductor = root_var;
  while (conductor != NULL)
    {
      if (conductor->def == 1)
        f << conductor->protect << ' ' << conductor->varname << "=*" << conductor->value << '\n';
      else
        f << conductor->protect << ' ' << conductor->varname << '=' << conductor->value << '\n';
      conductor = conductor->next;
    }
}

void
ModAlice::loadvars (c_char varfile)
{
  // first wipe out our existing variables
  var_array *conductor = root_var;
  while (conductor != NULL)
    {
      conductor = conductor->next;
      delete root_var;
      root_var = conductor;
    }
  // next we see if varfile exists
  char scrap[MAX_LINE_SIZE+1], scrap2[MAX_LINE_SIZE+1];
#ifdef HAS_IOS_BINARY
  ifstream f (varfile, ios::in | ios::binary | IOS_NOCREATE);
#else
  ifstream f (varfile, ios::in | IOS_NOCREATE);
#endif
  if (!f)
    {
      // if not, we load up defvars.txt into our varfile, and into memory
      loadvars (datafile);
      savevars (varfile);
    }
  else
    {
      // if so, we load up variables from it into memory
      // first we grab that
      f.getline (that, MAX_LINE_SIZE);
      // now we load up our vars
      while (f.getline (scrap, MAX_LINE_SIZE))
        {
          if (!(strstr (scrap, "=")))
            continue;
          my_strncpy (scrap2, scrap + 2, MAX_LINE_SIZE);
          scrap[1] = 0;
          int protect = atoi (scrap);
          my_strncpy (scrap, scrap2, MAX_LINE_SIZE);
          *strchr (scrap, '=') = 0;
          char *varname = scrap;
          char *value = strstr (scrap2, "=")+1;
          setvar (protect, varname, value);
        }
    }
}

void
ModAlice::replacer (char *line)
{
  char *pos;
  
  replace (line, "<alice>", "");
  replace (line, "<category>", "");
  replace (line, "</category>", "");
  replace (line, "</alice>", "");
  if ((pos = strstr (line, "<topic name=\"")))
    {
      my_strncpy (topic, pos, MAX_VARVAL_SIZE);
      *strstr (topic, "\"") = 0;
    }
  if (strstr (line, "</topic>"))
    topic[0] = 0;
  strremove (line, "<topic", ">");
  replace (line, "</topic>", "");
  while (replace (line, "<getversion/>", "<getvar name=\"version\"/>"));
  while (replace (line, "<set_animagent/>", "<setvar name=\"animagent\">on</setvar>"));
  while (replace (line, "<person/>", "<person><star/></person>"));
  while (replace (line, "<person2/>", "<person2><star/></person2>"));
  while (replace (line, "<personf/>", "<personf><star/></personf>"));
  while (replace (line, "<setname/>", "<setvar name=\"name\"/><star/></setvar>"));
  while (replace (line, "<birthday/>", "<getvar name=\"botbirthday\"/>"));
  while (replace (line, "<birthplace/>", "<getvar name=\"botbirthplace\"/>"));
  while (replace (line, "<botasmter/>", "<getvar name=\"botasmter\"/>"));
  while (replace (line, "<botmaster/>", "<getvar name=\"botmaster\"/>"));
  while (replace (line, "<boyfriend/>", "<getvar name=\"botboyfriend\"/>"));
  while (replace (line, "<favorite_band/>", "<getvar name=\"botband\"/>"));
  while (replace (line, "<favorite_book/>", "<getvar name=\"botbook\"/>"));
  while (replace (line, "<favorite_color/>", "<getvar name=\"botcolor\"/>"));
  while (replace (line, "<favorite_food/>", "<getvar name=\"botfood\"/>"));
  while (replace (line, "<favorite_movie/>", "<getvar name=\"botmovie\"/>"));
  while (replace (line, "<favorite_song/>", "<getvar name=\"botsong\"/>"));
  while (replace (line, "<for_fun/>", "<getvar name=\"botfun\"/>"));
  while (replace (line, "<friends/>", "<getvar name=\"botfriends\"/>"));
  while (replace (line, "<gender/>", "<getvar name=\"botgender\"/>"));
  while (replace (line, "<getname/>", "<getvar name=\"name\"/>"));
  while (replace (line, "<get_does/>", "<getvar name=\"does\"/>"));
  while (replace (line, "<get_gender/>", "<getvar name=\"gender\"/>"));
  while (replace (line, "<getsize/>", "<getvar name=\"botsize\"/>"));
  while (replace (line, "<gettopic/>", "<getvar name=\"topic\"/>"));
  while (replace (line, "<girlfriend/>", "<getvar name=\"botgirlfriend\"/>"));
  while (replace (line, "<location/>", "<getvar name=\"botlocation\"/>"));
  while (replace (line, "<look_like/>", "<getvar name=\"botlooks\"/>"));
  while (replace (line, "<name/>", "<getvar name=\"botname\"/>"));
  while (replace (line, "<kind_music/>", "<getvar name=\"botmusic\"/>"));
  while (replace (line, "<question/>", "<getvar name=\"question\"/>"));
  while (replace (line, "<sign/>", "<getvar name=\"botsign\"/>"));
  while (replace (line, "<talk_about/>", "<getvar name=\"bottalk\"/>"));
  while (replace (line, "<they/>", "<getvar name=\"they\"/>"));
  while (replace (line, "<wear/>", "<getvar name=\"botwear\"/>"));
  while (replace (line, "<setname>", "<setvar name=\"name\">"));
  while (replace (line, "</setname>", "</setvar>"));
  while (replace (line, "<settopic>", "<setvar name=\"topic\">"));
  while (replace (line, "</settopic>", "</setvar>"));
  while (replace (line, "<set_does>", "<setvar name=\"does\">"));
  while (replace (line, "<set_it>", "<setvar name=\"it\">"));
  while (replace (line, "</set_it>", "</setvar>"));
  while (replace (line, "<set_personality>", "<setvar name=\"personality\">"));
  while (replace (line, "</set_personality>", "</setvar>"));
  while (replace (line, "<set_female/>","<setvar name=\"gender\">she</setvar>"));
  while (replace (line, "<set_male/>","<setvar name=\"gender\">he</setvar>"));
  // need code to handle get_??? and set_???
  while ((pos = strstr(line, "<get_")))
    {
      replace (pos, "<get_", "<getvar name=\"");
      replace (pos, "/>", "\"/>");
    }
  while ((pos = strstr (line, "<set_")))
    {
      replace (pos, "<set_", "<setvar name=\"");
      replace (pos, ">", "\">");
      if (replace (pos, "</se", "</setvar>"))
        {
          pos = strstr (pos, "</setvar>") + 9;
          strremove (pos, "t_", ">");
        }
    }
  while (replace (line, "<sr/>", "<srai><star/></srai>"));
  while (replace (line, "\n", ""));
}  

int
ModAlice::checkline (char *line)
{
/* this performs syntax checking on the AIML.

ok, what we've got is a global int called cc.  If we have a
tag we add the appropiate value.  If we have a /tag we subtract that
value.
The values go as follows.
<alice> = ba = +1  </alice> = ea = -1
<topic> = bt = 2
category = bc = 4
pattern = bp = 8
that = bh = 16
template = be = 32
*/
  int ba = 0, ea = 0; // <alice> begin and end
  int bt = 0, et = 0; // <topic>
  int bc = 0, ec = 0; // <category>
  int bp = 0, ep = 0; // <pattern>
  int bh = 0, eh = 0; // <that>
  int be = 0, ee = 0; // <template>
  if (strstr (line, "<alice>"))
    ba = 1;
  if (strstr (line, "</alice>"))
    ea = 1;
  if (strstr (line, "<topic"))
    bt = 1;
  if (strstr (line, "</topic>"))
    et = 1;
  if (strstr (line, "<category>"))
    bc = 1;
  if (strstr (line, "</category>"))
    ec = 1;
  if (strstr (line, "<pattern>"))
    bp = 1;
  if (strstr (line, "</pattern>"))
    ep = 1;
  if (strstr (line, "<that>"))
    bh = 1;
  if (strstr (line, "</that>"))
    eh = 1;
  if (strstr (line, "<template>"))
    be = 1;
  if (strstr (line, "</template>"))
    ee = 1;
  if (ba)
    {
      if (cc != 0)
        return 0;
      cc += 1;
    }
  if (bt)
    {
      if (cc != 1)
        return 0;
      cc += 2;
    }
  if (bc)
    {
      if (cc !=1 && cc != 3)
        return 0;
      cc += 4;
    }
  if (bp)
    {
      if (!(cc & 1) || !(cc & 4))
        return 0;
      cc += 8;
    }
  if (ep)
    {
      if (!(cc & 1) || !(cc & 4) || hh || he || !(cc & 8))
        return 0;
      cc -= 8;
      hp = 1;
    }
  if (bh)
    {
      if (!(cc & 1) || !(cc & 4) || !(hp) || he)
        return 0;
      cc += 16;
    }
  if (eh)
    {
      if (!(cc & 1) || !(cc & 4) || !(hp) || he || !(cc & 16))
        return 0;
      cc -= 16;
      hh = 1;
    }
  if (be)
    {
      if (!(cc & 1) || !(cc & 4) || !hp)
        return 0;
      cc += 32;
    }
  if (ee)
    {
      if (!(cc & 1) || !(cc & 4) || !(hp) || !(cc & 32))
        return 0;
      cc -= 32;
      he = 1;
    }
  if (ec)
    {
      if (!(cc & 1) || !(cc & 4) || !hp || !he)
        return 0;
      cc -= 4;
      hp = 0;
      hh = 0;
      he = 0;
    }
  if (et)
    {
      if (cc != 3)
        return 0;
      cc -= 2;
    }
  if (ea)
    cc -= 1;
  return 1;
}

int
ModAlice::splitaiml (c_char src, fstream &unsorted)
{
  cc = 0;
  // it now prepares the temp file into the template and pattern files
  int patterns = 0, templates = 0, bytecount = 0;

  ifstream aiml (src, ios::in | IOS_NOCREATE);
  if (!aiml)
    {
      s->write_botlog ("skipping file %s: %s", src, strerror (errno));
      return 1;
    }

  int linenr = -1;
  char line[MAX_LINE_SIZE+1], next[MAX_LINE_SIZE+1], buffer[MAX_LINE_SIZE+1];
  aiml.getline (line, MAX_LINE_SIZE);
  if (!checkline (line))
    {
      s->write_botlog ("error %i encountered in file %s, at line %i", cc, src, linenr);
      return 0;
    }
  memset (buffer, 0, MAX_LINE_SIZE);
  /* we now make the temp.txt into unsorted.txt and templates.txt
     also we exchange some tags for shorthand tags. */
  while (aiml.getline (next, MAX_LINE_SIZE))
    {
      linenr++;
      if (!checkline (next))
        {
          s->write_botlog ("error %i encountered in file %s, at line %i", cc, src, linenr);
          return 0;
        }
      replacer (line);
      if (strstr (line, "<pattern>") && strstr (line, "<that>") || strstr (next, "<that>"))
        {
          while (!strstr (line, "</that>"))
            {
              if (strlen (line) > MAX_LINE_SIZE)
                {
                  s->write_botlog ("line too long: %s", line);
                  break;
                }
              strcpy (line + strlen (line), next);
              replacer (line);
              linenr++;
              aiml.getline (next, MAX_LINE_SIZE);
              if (!checkline (next))
                {
                  s->write_botlog ("error %i encountered in file %s, at line %i", cc, src, linenr);
                  return 0;
                }
            }
          patterns++;
          // what we need to print out.. is line till that
          strcpy (buffer, strstr (line, "</that>") + 7);
          *(strstr (line,"</that>") + 7) = 0;
          replace (line, "<pattern>", "");
          replace (line, "</pattern>", ""); 
          unsorted << line;
          bytecount += strlen (line) - 3;
        }
      else if (strstr (line, "<pattern>" ))
        {
          while (!strstr (line, "</pattern>"))
            {
              if (strlen (line) > MAX_LINE_SIZE)
                {
                  s->write_botlog ("line too long: %s", line);
                  break;
                }
              strcpy (line + strlen (line), next);
              replacer (line);
              linenr++;
              aiml.getline (next, MAX_LINE_SIZE);
              if (!checkline (next))
                {
                  s->write_botlog ("error %i encountered in file %s, at line %i", cc, src, linenr);
                  return 0;
                }
            }
          patterns++;
          strcpy (buffer, strstr (line, "</pattern>") + 10); 
          *(strstr (line,"</pattern>") + 10) = 0;
          replace (line, "<pattern>", "");
          replace (line, "</pattern>", "");
          unsorted << line;
          bytecount += strlen (line) - 3;
        }
      if (strstr (line, "<template>"))
        {
          while (!strstr (line, "</template>"))
            {
              if (strlen (line) > MAX_LINE_SIZE)
                {
                  s->write_botlog ("line too long: %s", line);
                  break;
                }
              strcpy (line + strlen (line), next);
              replacer (line);
              linenr++;
              aiml.getline (next, MAX_LINE_SIZE);
              if (!checkline (next))
                {
                  s->write_botlog ("error %i encountered in file %s, at line %i", cc, src, linenr);
                  return 0;
                }
            }
          long tpos = templatefile.tellp ();
          templates++;
          if (templates != patterns)
            {
              s->write_botlog ("pattern/template count mismatch");
              mbot_exit ();
            }
          strcpy (buffer, strstr (line, "</template>") + 11);
          *(strstr (line, "</template>") + 11) = 0;
          replace (line, "<template>", "");
          templatefile << line << '\n';
          unsorted << "<tpos=" << tpos << ">\n";
          strcpy (buffer, strstr (line, "</template>") + 11);
          bytecount += strlen (line) - 3;
        }
      strcpy (line, buffer);
      strcpy (line + strlen (line), next);
    }
  // this shouldn't be needed
  templatefile << "<end of file>\n";
  return 1;
}

int
ModAlice::prepare (c_char file)
{
  // routine looks through C.aiml for what files to load
  fstreamtmp unsorted;
  if (!unsorted.is_open ())
    {
      s->b->write_botlog ("error creating temporary file: %s", strerror (errno));
      return 0;
    }
  char aline[256+1];
  ifstream aimlfile (file, ios::in | IOS_NOCREATE);
  if (!aimlfile)
    {
      s->write_botlog ("error opening aimlfile %s: %s", file, strerror (errno));
      return 0;
    }
  while (aimlfile.getline (aline, 256))
    if (!splitaiml (aline, unsorted))
      return 0;
  /* now we alphabetize unsorted.txt by the first letter.  
     we put this in patterns.txt
     we also set chunk, which will tell us where the first letter
     is in patterns.txt */
  unsorted.seekg (0);
  char buffer[MAX_LINE_SIZE+1];
  for (int i = 'A'; i <= ('Z' + 1); i++)
    {
      chunk[i - 'A'] = patternfile.tellp ();
      buffer[0] = i;
      buffer[1] = 0;
      unsorted.seekg (0);
      while (unsorted.getline (buffer, MAX_LINE_SIZE))
        {
          if (i <= 'Z')
            {
              if (buffer[0] == i)
                patternfile << buffer << '\n';
            }
          else
            if ((buffer[0] < 'A') || (buffer[0] > 'Z'))
              patternfile << buffer << '\n';
        }
      unsorted.clear ();
    } 
  return 1;
}

void
ModAlice::substitute (Text &subst, char *text)
{
  char spaces[MAX_LINE_SIZE+1], string1[MAX_LINE_SIZE+1],
       string2[MAX_LINE_SIZE+1], line[MAX_LINE_SIZE+1], *strtmp;
  long long pos1, pos2, pos3, pos4, l;
  subst.rewind_text ();
  upper (text);
  // replace "'" with "`"
  size_t i;
  for (i = 0; i < strlen (text); i++)
    if (text[i] == '\'')
      text[i] = '`';
  my_strncpy (spaces + 1, text, MAX_LINE_SIZE-1);
  spaces[0] = ' ';
  l = strlen (spaces);
  spaces[l] = ' ';
  spaces[l + 1] = 0;
  // do replacing from our filename
  for (i = subst.line_num; i > 0; i--)
    {
      my_strncpy (line, subst.get_line (), MAX_LINE_SIZE);
      // allow comments
      strtmp = strstr (line, "#");
      if (strtmp != NULL)
        strtmp[0] = 0;
      if ((pos1 = strstr (line, "'") - line) < 0
          || (char *)0-line == (long long)pos1)
        continue;
      if ((pos2 = strstr (line + pos1 + 1, "'") - line) < 0
          || (char *)0-line == (long long)pos2)
        continue;
      if ((pos3 = strstr (line + pos2 + 1, "'") - line) < 0
          || (char *)0-line == (long long)pos3)
        continue;
      if ((pos4 = strstr (line + pos3 + 1, "'") - line) < 0
          || (char *)0-line == (long long)pos4)
        continue;
      my_strncpy (string1, line + pos1 + 1, MAX_LINE_SIZE);
      string1[pos2 - pos1] = 0;
      upper (string1);
      my_strncpy (string2, line + pos3 + 1, MAX_LINE_SIZE);
      string2[pos4 - pos3] = 0;
      string1[pos2 - pos1 - 1] = string2[pos4 - pos3 - 1] = 0;
      while (replace (spaces, string1, string2));
    }
  strcpy (text, spaces + 1);
}

int
ModAlice::match (c_char text, c_char pattern, int final)
{
  long long starpos, pos2;
  // if it's a perfect match, return with a YIPPE!
  if (!strcmp (pattern, text))
    return 1;
  // if we just have a '*'
  if ((pattern[0] == '*') && (!pattern[1]))
    {
      if (final)
        strcpy (star, text);
      return 1;
    }
  // check to see if we have a star.. if we don't return false
  if (((starpos = strstr( pattern, "*" ) - pattern) < 0) &&
      ((starpos = strstr( pattern, "_" ) - pattern) < 0))
    return 0;
  // make sure the star isn't in the front
  if (starpos > 0)
    {
      // if the stuff in the front of the star doesn't match, return false
      if (strncmp( pattern, text, starpos ))
        return 0;
      // if we have nothing after the star, return true
      if (!pattern[starpos + 1])
        {
          if (final)
            strcpy (star, text + starpos);
          return 1; 
        }
    }
  /* at this point, we either have a star in the front and text
     * HELLO
     or a star in the middle and the text in front matches.
     HELLO * COMPUTER
     this code should be simplified using strcspn()'s and strspn()
     actually.. a better idea..  we reverse both of them
     then we do the same thing we did to begin with.
     nonetheless.. for now.. this code works.. :) */
  char pluseoln[MAX_LINE_SIZE+1], texteoln[MAX_LINE_SIZE+1];
  my_strncpy (texteoln, text, MAX_LINE_SIZE);
  int len = strlen (text);
  texteoln[len] = 10;
  texteoln[len + 1] = 0;
  my_strncpy (pluseoln, pattern + starpos + 1, MAX_LINE_SIZE);
  len = strlen (pluseoln);
  pluseoln[len] = 10;
  pluseoln[len + 1] = 0;
  if ((pos2 = strstr (texteoln, pluseoln) - texteoln) > -1
      && (char *)0-texteoln != pos2)
    {
      if (text[pos2 + strlen (pattern + starpos + 1)] == 0)
        {
          if (final)
            {
              strcpy (star, text + starpos);
              star[pos2 - starpos] = 0;
            }
          return 1;
        }
    }
  return 0;
}

void
ModAlice::print (char *line)
{
  int start = 1, space = 0, bpos = 0;
  char *pos = pbuffer, *scrap;
  while (*pos)
    {
      if (*pos == ' ')
        space = 1;
      else
        {
          /* this capitilizes I and I'm */
          if ((space) && ((*pos == 'i') || (*pos == 'I')) && 
              ((*(pos + 1) == ' ') || (*(pos + 1) == '\'')))
            {
              line[bpos++] = ' ';
              line[bpos++] = 'I';
              space = 0;
            }	
          /* capitilizes the first word in a sentence. */
          else if ((start) && (*pos >= 'a') && (*pos <= 'z'))
            {
              if (space)
                line[bpos++] = ' ';
              line[bpos++] += 'A' - 'a';
              start = space = 0;
            } 
          else 
            {
              if ((space) && (*(pos +1) != '.'))
                line[bpos++] = ' ';
              line[bpos++] = *pos;
              space = 0;
            }
          start = 0;
        }
      if ((*pos == '.') && (*pos+1 == ' '))
        start = 1;
      pos++;
    }
  // Capitalize current user name
  scrap = strstr (line, getvar ("name", "Human"));
  if ((scrap) && (*scrap >= 'a') && (*scrap <= 'z'))
    *scrap += 'A' - 'a';
  line[bpos] = 0;
  int last = strlen( line ) - 1;
  while ((line[last] == ' ') && (last >= 0))
    line[last--] = 0;
  if ((last >= 0) && (line[last] >= 'a') && (line[last] <= 'z'))
    {
      line[last + 1] = '.';
      line[last + 2] = 0;
    }
}

void
ModAlice::randomize (char *text)
{
  // this function handles our random tags
  long long pos1;
  int list[MAX_RANDLIST_SIZE];
  char rstring[MAX_LINE_SIZE+1], buffer[MAX_LINE_SIZE+1], *strtmp;

  while ((long long)(pos1 = (char *)strstr (text, "<random>") - (char *)text) > -1
         && (char *)0-text != (long long)pos1)
    {
      my_strncpy (rstring, text + pos1 + 8, MAX_LINE_SIZE);
      strtmp = strstr (rstring, "</random>");
      if (strtmp != NULL)
        {
          strtmp[0] = 0;
          int items = 0;
          long long pos, last = -1;
          while ((pos = (long long)(strstr (rstring + last + 1, "<li>") - rstring)) > -1
                 && (char *)0-rstring != (long long)pos)
            {
              list[items++] = pos + 4;
              last = pos;
            }
          int rnd = random_num (items);
          my_strncpy (buffer, text, MAX_LINE_SIZE);
          strcpy (buffer + pos1, rstring + list[rnd]);
          if (strstr (buffer, "</li>"))
            *strstr (buffer, "</li>") = 0;
          strcpy (buffer + strlen (buffer), " ");
          strcpy (buffer + strlen (buffer), strstr (text, "</random>") + 9);
          strcpy (text, buffer);
        }
      else
        {
          // Handle erroneous randoms
          my_strncpy (rstring, text + pos1 + 8, MAX_LINE_SIZE);
          strcpy (text, rstring);
        }
    }
}

void
ModAlice::reevaluate (char *line)
{
  char varname[MAX_VARNAME_SIZE+1];
  char value[MAX_VARVAL_SIZE+1];
  char scrap[MAX_LINE_SIZE+1];
  char *pos;
  lower (star);
  randomize (line);

  while ((pos = strstr (line, "<think>")))
    {
      my_strncpy (scrap, pos + 7, MAX_LINE_SIZE);
      *strstr (scrap, "</think>") = 0;
      reevaluate (scrap);
      strremove (line, "<think>", "</think>");
    }

  while (replace (line, "<star/>", star));
  while (replace (line, "<that/>", that));

  while ((pos = strstr(line,"<person>")))
    {
      my_strncpy (scrap, pos + 8, MAX_LINE_SIZE);
      *strstr (scrap, "</person>") = 0;
      substitute (personfile, scrap);
      lower (scrap);
      strremove (line, "son>", "</person>"); 
      replace (line, "<per", scrap);
    }

  while ((pos = strstr (line, "<person2>")))
    {
      my_strncpy (scrap, pos + 9, MAX_LINE_SIZE);
      *strstr (scrap, "</person2>") = 0;
      substitute (person2file, scrap);
      lower (scrap);
      strremove (line, "son2>", "</person2>"); 
      replace (line, "<per", scrap);
    }

  while ((pos = strstr (line, "<personf>")))
    {
      my_strncpy (scrap, pos + 9, MAX_LINE_SIZE);
      *strstr (scrap, "</personf>") = 0;
      substitute (personffile, scrap);
      lower (scrap);
      strremove (line, "sonf>", "</personf>"); 
      replace (line, "<per", scrap);
    }

  while ((pos = strstr (line, "<getvar")))
    {
      if (!(pos =strstr (pos + 8, "name")))
        {
          strremove (line, "<getvar", "/>");
          continue;
        }
      pos = strstr (pos, "\"") + 1;
      // ok.. now pos = foo" default="bar" /> or... pos = foo"
      my_strncpy (scrap, pos, MAX_LINE_SIZE);
      char *lix = strstr (scrap, "\"");
      if (lix != NULL)
        lix[0] = 0;
      my_strncpy (varname, scrap, MAX_VARNAME_SIZE);
      pos = strstr (line, "<getvar");
      // this is if we don't have a default
      if (!(strstr (pos + 8, "default")))
        {
          my_strncpy (scrap, pos, MAX_LINE_SIZE);
          *strstr (line, "<getvar") = 0;
          strremove (scrap, "<getvar","/>");
          strncat (line, getvar (varname, "weird"), MAX_LINE_SIZE-strlen (line));
          strncat (line, scrap, MAX_LINE_SIZE-strlen (line));
          continue;
        }
      // insert code here to deal with defaults
    }

  while ((pos = strlstr (line, "<setvar")))
    {
      char *holder = pos;
      int setreturn;
      pos = strstr (pos, "name=\"") + 6;
      my_strncpy (scrap, pos, MAX_LINE_SIZE);
      *strstr (scrap, "\"") = 0;
      my_strncpy (varname, scrap, MAX_VARNAME_SIZE);
      pos = strstr (pos, ">") +1;
      strcpy (scrap, pos);
      char *lix = strstr (scrap, "</setvar>");
      if (lix != NULL)
        *lix = 0;
      my_strncpy (value, scrap, MAX_VARVAL_SIZE);
      setreturn = setvar (0, varname, value);
      if (setreturn == 1)
        {
          // print name
          strremove (holder, "<setvar", "\"");
          strremove (holder, "\"", "</setvar>");
        }
      if (setreturn == 2)
        {
          strremove (holder, "<setvar", ">");
          replace (holder, "</setvar>", "");
        }
    }

  while ((pos = strstr (line, "<srai>")))
    {
      if (++srai_num > MAX_SRAI)
        break;
      my_strncpy (scrap, pos + 6, MAX_LINE_SIZE);
      *strstr (scrap, "</srai>") = 0;
      upper (scrap);
      my_strncpy (scrap, respond2 (scrap), MAX_LINE_SIZE);
      // is this too cheap?
      strremove (line, "ai>", "</srai>"); 
      replace (line, "<sr", scrap);
    }

  while ((pos = strstr (line, "<system>")))
    {
      my_strncpy (scrap, pos + 8, MAX_LINE_SIZE);
      *strstr (scrap, "</system>") = 0;
      system (scrap);
      strremove (line, "<system>", "</system>");
    }

  while ((pos = strstr (line, "<seen>")))
    {
      my_strncpy (scrap, pos + 6, MAX_LINE_SIZE);
      *strstr (scrap, "</seen>") = 0;

      // if it's in a channel with the bot
      for (int i = 0; i < s->channel_num; i++)
        if (CHANNELS[i]->user_index (scrap) != -1)
          {
            snprintf (line, MAX_LINE_SIZE, "%s is currently online.", scrap);
            return;
          }

      // else get the seen module and ask it
      time_t t = -1;
      Module *m;
      s->b->modules.rewind ();
      while ((m = (Module *)s->b->modules.next ()) != NULL)
        if (strcmp (*m, "seen") == 0)
          break;
      s->b->modules.restore_current ();
      if (m != NULL)
        {
          time_t (*server2seentime)(NetServer *, c_char) = (time_t(*)(NetServer *, c_char))m->getsym ("server2seentime");
          if (server2seentime != NULL)
            t = server2seentime (s, scrap);
        }
      if (t == -1)
        switch (random_num (2))
          {
            case 0: snprintf (line, MAX_LINE_SIZE, "Humm, I don't remember %s.", scrap); break;
            case 1: snprintf (line, MAX_LINE_SIZE, "I don't remember %s... Tell %s to buy me more RAM.", scrap, getvar ("botmaster", "mirage")); break;
          }
      else
        {
          char timestr[MSG_SIZE+1];
          my_strncpy (timestr, asctime (localtime (&t)), MSG_SIZE);
          timestr[strlen (timestr) - 1] = 0;
          snprintf (line, MAX_LINE_SIZE, "I last saw %s on %s.", scrap, timestr);
        }
    }

  while ((pos = strstr (line, "<word>")))
    {
      my_strncpy (scrap, pos+6, MAX_LINE_SIZE);
      *strstr (scrap, "</word>") = 0;
      Module *m;
      s->b->modules.rewind ();
      while ((m = (Module *)s->b->modules.next ()) != NULL)
        {
          if (strcmp (*m, "word") == 0)
            break;
        }
      s->b->modules.restore_current ();
      char text[MSG_SIZE+1];
      text[0] = 0;
      if (m != NULL)
        {
          void (*server2wordtext)(NetServer *, c_char, char *, size_t) = (void(*)(NetServer *, c_char, char *, size_t))m->getsym ("server2wordtext");
          if (server2wordtext != NULL)
            server2wordtext (s, scrap, text, MSG_SIZE);
        }
      if (text[0] == 0)
        snprintf (line, MAX_LINE_SIZE, "Not defined... lousy word anyway :D");
      else
        {
          switch (random_num (5))
            {
              case 0: snprintf (line, MAX_LINE_SIZE, "I heard that %s is %s", scrap, text); break;
              case 1: snprintf (line, MAX_LINE_SIZE, "Someone said %s is %s", scrap, text); break;
              case 2: snprintf (line, MAX_LINE_SIZE, "%s is %s", scrap, text); break;
              case 3: snprintf (line, MAX_LINE_SIZE, "I know that %s is %s", scrap, text); break;
              case 4: snprintf (line, MAX_LINE_SIZE, "It's public that %s is %s", scrap, text); break;
            }
        }
    }
  strcpy (that, line);
}

char *
ModAlice::respond2 (char *text)
{
  char *pos, line[MAX_LINE_SIZE+1], buffer[MAX_LINE_SIZE+1],
       capthat[MAX_LINE_SIZE+1];
  long tpos, bestpos = 0;
  int alpha, thatused;
  star[0] = pbuffer[0] = 0;
  best = "";
  alpha = ((text[0] >= 'A') && (text[0] <= 'Z'));
  long long tval;
  int bestline = -1, linenr = -1;
  patternfile.clear ();
  if (alpha) 
    patternfile.seekg (chunk[text[0] - 'A']);
  else 
    patternfile.seekg (chunk[26]);
  while (!(!patternfile))
    {
      linenr++;
      patternfile.getline (line, MAX_LINE_SIZE);
      /* this gets fired if Alice didn't find an exact
         match in it's own letter */
      if ((alpha) && (line[0] != text[0]))
        {
          alpha = 0;
          patternfile.clear ();
          patternfile.seekg (chunk[26]);
          continue;
        }
      /* this strips the position out of the end of the pattern.
         if the line doesn't contain a <tops get the next line. */
      if ((tval = strstr (line, "<tpos=") - line) < 0
          || (char *)0-line == (long long)tval)
        continue;
      my_strncpy (buffer, line + tval + 6, MAX_LINE_SIZE);
      if (strstr (buffer, ">"))
        *strstr (buffer, ">") = 0;
      line[tval] = 0;
      tpos = atol (buffer);
      thatused = 0;
      while (replace (line, "<getvar name=\"botname\"/>", getvar ("botname", "DarkSun")));
      // check to see if <that> was used
      if ((pos = strstr (line, "<that>")))
        {
          // we copy that into buffer
          my_strncpy (buffer, pos + 6, MAX_LINE_SIZE);
          // we chop that off our line
          *strstr (line, "<that>") = 0;
          if (strstr (buffer, "</that>"))
            *strstr (buffer, "</that>") = 0;
          /* now buffer holds our that from the pattern
             that holds our last statement. */
          my_strncpy (capthat, that, MAX_LINE_SIZE);
          upper (capthat);
          cleaner (capthat);
          if (!match (capthat, buffer, 0 ))
            continue;
          thatused = 1;
        }
      spacetrim (line);

      if (!match (text, line, 0))
        continue;
      if ((strcmp (text, line) == 0) || (thatused))
        {
          best = line;
          bestline = linenr;
          bestpos = tpos;
          break;
        }
      if ((strcmp (line, best) < 0) && (bestline > -1))
        continue;
      best = line;
      bestline = linenr;
      bestpos = tpos;
    }
  match (text, best, 1);
  templatefile.clear ();
  templatefile.seekg (bestpos);
  templatefile.getline (line, MAX_LINE_SIZE);
  buffer[0] = 10;
  buffer[1] = 0;
  if (strstr (line, "</template>"))
    *strstr (line, "</template>") = 0;
  reevaluate (line);
  strcpy (pbuffer, line);
  return pbuffer;
}

////////////////////
// module managing
////////////////////

ModAlice *
server2alice (NetServer *s)
{
  ModAlice *a;
  alice_list->rewind ();
  while ((a = (ModAlice *)alice_list->next ()) != NULL)
    if (a->s == s)
      return a;
  return NULL;
}

// events watcher to parse privmsg
void
alice_event (NetServer *s)
{
  if (EVENT != EVENT_PRIVMSG
      || CMD[3][0] == ''				// ctcp
      || (CMD[2][0] >= '0' && CMD[2][0] <= '9'))	// dcc chat
    return;

  char *alans;
  ModAlice *a = server2alice (s);
  if (a == NULL)
    return;

  // if it's a channel
  if ((CMD[2][0] == '#') || ((CMD[2][0] == '@') && (CMD[2][1] == '#')))
    {
      if (strncasecmp (CMD[3], s->nick, s->nick.getlen ()) == 0)
        {
          char *buf = strstr (CMD[3], " ");
          if (buf == NULL)
            return;
          alans = a->respond (buf);
          stripurl (alans);
          strip_crlf (alans);
          SEND_TEXT (DEST, "%s: %s", SOURCE, alans);
        }
      return;
    }

  // pvt with the bot
  char buf[MSG_SIZE+1];
  my_strncpy (buf, CMD[3], MSG_SIZE);
  strtok (buf, " ");
  // only respond if it's not a mbot command
  if (s->script.cmd_get (buf) != NULL)
    return;
  alans = a->respond (CMD[3]);
  stripurl (alans);
  strip_crlf (alans);
  SEND_TEXT (DEST, "%s", alans);
}

// configuration file's local parser
void
alice_conf (NetServer *s, c_char bufread)
{
  char buf[5][MSG_SIZE+1];
  strsplit (bufread, buf, 4);

  if (strcasecmp (buf[0], "alice") == 0)
    {
      if (buf[2][0] == 0)
        s->b->conf_error ("sintax error: use \"alice initfile varsfile [loghandle]\"");
      Log *log = NULL;
      if (buf[3][0] != 0)
        {
          log = s->b->log_get (buf[3]);
          if (log == NULL)
            {
              snprintf (buf[0], MSG_SIZE, "inexistant loghandle: %s", buf[3]);
              s->b->conf_error (buf[0]);
            }
        }
      ModAlice *a = server2alice (s);
      if (a != NULL)
        s->b->conf_error ("alice already defined in this server");
      a = new ModAlice (s, buf[1], buf[2], log);
      if (!a->initialized)
        s->b->conf_error ("error initializing alice");
      alice_list->add ((void *)a);
      s->script.events.add ((void *)alice_event);
    }
}

void
alice_stop (void)
{
  ModAlice *a;
  alice_list->rewind ();
  while ((a = (ModAlice *)alice_list->next ()) != NULL)
    {
      a->s->script.events.del ((void *)alice_event);
      delete a;
    }
  delete alice_list;
}

// module initialization
void
alice_start (void)
{
  alice_list = new List ();
}

