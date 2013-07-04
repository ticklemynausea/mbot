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

#define LEVEL_CALC 0

#define HELP_CALC "!calc expression - Evaluate a mathematical <expression> and output the result. Use !calch/calcb/calco/calcr for results returned in hex, bin, octal and roman respectively, but only for unsigned integers."

#define DEST s->script.dest
#define BUF s->script.buf
#define SEND_TEXT s->script.send_text

#define PI 3.14159265358979323846

#define VARS_MAX 100

/////////////////////
// class definition
/////////////////////

class ModCalc {
public:
  ModCalc (NetServer *server) : s (server), ans (0) {}
  ~ModCalc (void);

  double calc (c_char);

  NetServer *s;
  int error_pos;

  // WINDOZE wants this to be public
  enum s_type { S_NUM, S_OP, S_ID, S_BRACE_L, S_BRACE_R, S_EOF };
  enum op_type { OP_ASSIGN, OP_PLUS, OP_MINUS, OP_TIMES, OP_DIV, OP_MOD,
                 OP_POW, OP_SHIFT_L, OP_SHIFT_R, OP_AND, OP_OR };

private:

  struct symbol_type {
    int pos;
    s_type type;
    op_type op;		// used if type is S_OP
    char priority;	// used if type is S_OP
    double value;	// used if type is S_NUM
    String id;		// used if type is S_ID
    symbol_type *next;
    symbol_type (int p, double v) : pos (p), type (ModCalc::S_NUM), value (v) {}
    symbol_type (int p, op_type o, char pr) : pos (p), type (S_OP), op (o), priority (pr) {}
    symbol_type (int p, c_char i) : pos (p), type (S_ID), id (i, MSG_SIZE) {}
    symbol_type (int p, s_type t) : pos (p), type (t) {}
  } *top, *bottom;

  void scan_symbols (c_char);

  double parse_explist (void);
  bool parse_expsimple (void);
  bool parse_exp_id (symbol_type *);

  struct var_type {
    String name;
    double value;
    time_t time;
    var_type (c_char n, double v) : name (n, MSG_SIZE), value (v),
      time (get_time ()) {}
  };
  List vars;

  int braces;
  double ans;		// holds the last valid answer

  void delete_symbols (void);
  void push_symbol_bottom (symbol_type *);
  void push_symbol (symbol_type *);
  symbol_type *pop_symbol (void);

};

List *calc_list;

///////////////
// prototypes
///////////////

static char error[MSG_SIZE+1];

struct const_type {
  c_char name;
  double value;
};
static const const_type consts[] = {
  { "c", 2.997925E8 },		// velocidade de propagação da luz no vazio
  { "e", 2.7182818284590452354 },
  { "E0", 8.854188E-12 },	// permitividade do vazio
  { "F", 9.65E4 },		// constante de Faraday
  { "g", 9.8 },			// força da gravidade na Terra
  { "G", 6.67E-11 },		// constante gravitacional
  { "h", 6.63E-34 },		// constante de Planck
  { "k0", 8.99E9 },		// constante de Coulomb
  { "me", 9.109390E-31 },	// massa própria do electrão
  { "mp", 1.672623E-27 },	// massa própria do protão
  { "mn", 1.674929E-27 },	// massa própria do neutrão
  { "mol", 6.022137E23 },	// constante de Avogadro
  { "N", 6.022137E23 },		// idem
  { "NA", 6.022137E23 },	// idem
  { "pi", PI },
  { "u", 1.66E-27 },		// unidade de massa atómica
  { "u0", 1.26E-6 },		// permeabilidade no vazio
  { "R", 8.31 },		// constante (molar) de gás ideal
  { "RH", 1.10E7 },		// constante de Rydberg
  { NULL, 0 }
};

enum func_type {
  FUNC_SQRT,
  FUNC_FABS,
  FUNC_CEIL,
  FUNC_FLOOR,
  FUNC_INT,
  FUNC_EXP,
  FUNC_LOG,
  FUNC_LOG10,
  FUNC_SINH, FUNC_SENH,
  FUNC_COSH,
  FUNC_TANH,
  FUNC_COSIN, FUNC_COSEN,
  FUNC_COCOS,
  FUNC_COTAN,
  FUNC_SIN, FUNC_SEN,
  FUNC_COS,
  FUNC_TAN,
  FUNC_ASIN, FUNC_ARSIN, FUNC_ARCSIN, FUNC_ASEN, FUNC_ARSEN, FUNC_ARCSEN,
  FUNC_ACOS, FUNC_ARCOS, FUNC_ARCCOS,
  FUNC_ATAN, FUNC_ARTAN, FUNC_ARCTAN,
  FUNC_RAND,
  FUNC_FACT,
  FUNC_RAD,
  FUNC_DEG,
  FUNC_NOT
};

static const c_char funcs[] = {
  "sqrt",
  "fabs",
  "ceil",
  "floor",
  "int",
  "exp",
  "log",
  "log10",
  "sinh", "senh",
  "cosh",
  "tanh",
  "cosin", "cosen",
  "cocos",
  "cotan",
  "sin", "sen",
  "cos",
  "tan",
  "asin", "arsin", "arcsin", "asen", "arsen", "arcsen",
  "acos", "arcos", "arccos",
  "atan", "artan", "arctan",
  "rand",
  "fact",
  "rad",
  "deg",
  "not",
  NULL
};

static void calc_cmd_calc_aux (NetServer *);
static void calc_cmd_calc (NetServer *);
static void calc_cmd_calch (NetServer *);
static void calc_cmd_calcb (NetServer *);
static void calc_cmd_calco (NetServer *);
static void calc_cmd_calcr (NetServer *);

static ModCalc *server2calc (NetServer *);
static void calc_add (NetServer *);
static void calc_conf (NetServer *, c_char);
static void calc_stop (void);
static void calc_start (void);

extern "C" {

EXPORT Module::module_type module = {
  MODULE_VERSION,
  "calc",
  calc_start,
  calc_stop,
  calc_conf,
  NULL
};

}

////////////////////
// class functions
////////////////////

ModCalc::~ModCalc (void)
{
  var_type *v;
  vars.rewind ();
  while ((v = (var_type *)vars.next ()) != NULL)
    delete v;
}

double
ModCalc::calc (c_char exp)
{
  error[0] = 0;
  braces = 0;
  top = bottom = NULL;
  scan_symbols (exp);
  if (error[0] != 0)
    return 0;
  double result = parse_explist ();
  if (error[0] == 0)
    ans = result;
  delete_symbols ();
  return result;
}

/*
  call parse_expsimple() repeatedly until the entire symbol stack is reduced
  to a single numeric symbol.
*/
double
ModCalc::parse_explist (void)
{
  while (error[0] == 0)
    if (parse_expsimple ())
      {
        symbol_type *symbol = pop_symbol ();
        if (symbol == NULL || symbol->type != S_NUM)
          {
            snprintf (error, MSG_SIZE, "FUNCTION PARAMETER ERROR!");
            break;
          }
        return symbol->value;
      }
  return 0;
}

/*
  parse a simple expression (<num><operation><num>) from the symbol stack
  and push the result. expands <num> to identifiers (constants, variables or
  functions), and to another expression inside braces.
  return 1 if S_EOF is found, 0 otherwise.
*/
bool
ModCalc::parse_expsimple (void)
{
  double d = 0;
  bool lvalue = 0;
  symbol_type *symbol, *op = NULL, *var = NULL;

  while (1)
    {
      symbol = pop_symbol ();
      if (symbol == NULL)
        {
          snprintf (error, MSG_SIZE, "STACK ERROR!");
          error_pos = 0;
          return 0;
        }
      error_pos = symbol->pos;

      if (symbol->type == S_EOF)
        {
          if (!lvalue || op != NULL || braces != 0)
            {
              snprintf (error, MSG_SIZE, "unexpected end of expression");
              break;
            }
          push_symbol (new symbol_type (symbol->pos, d));
          delete symbol;
          return 1;
        }

      else if (symbol->type == S_OP)
        {
          if (symbol->op == OP_ASSIGN)
            {
              if (var == NULL)
                {
                  snprintf (error, MSG_SIZE, "non-variable lvalue in assignment");
                  break;
                }
            }
          else 
            {
              if (op != NULL ||
                  (!lvalue && symbol->op != OP_PLUS && symbol->op != OP_MINUS))
                {
                  if (op != NULL)
                    snprintf (error, MSG_SIZE, "missing rvalue for operator");
                  else
                    snprintf (error, MSG_SIZE, "missing lvalue for operator");
                  break;
                }
              if (!lvalue && (symbol->op == OP_PLUS || symbol->op == OP_MINUS))
                {
                  d = 0;
                  lvalue = 1;
                }
            }
          op = symbol;
        }

      else if (symbol->type == S_NUM)
        {
          if (op == NULL)
            {
              if (lvalue)
                {
                  snprintf (error, MSG_SIZE, "two consecutive values");
                  break;
                }
              d = symbol->value;
              lvalue = 1;
              delete symbol;
              continue;
            }
          if (top != NULL && top->type == S_OP && top->priority > op->priority)
            {
              push_symbol (new symbol_type (symbol->pos, symbol->value));
              parse_expsimple ();
              if (error[0] != 0)
                break;
              delete symbol;
              continue;
            }
          switch (op->op)
            {
              case OP_ASSIGN:
                d = symbol->value;
                var_type *v;
                vars.rewind ();
                while ((v = (var_type *)vars.next ()) != NULL)
                  if (var->id == v->name)
                    break;
                if (v != NULL)
                  {
                    v->value = d;
                    v->time = get_time ();
                  }
                else
                  {
                    vars.add ((void *)new var_type (var->id, d));
                    if (vars.count () == VARS_MAX)
                      {
                        time_t t_old = -1;
                        var_type *v_old = NULL;
                        vars.rewind ();
                        while ((v = (var_type *)vars.next ()) != NULL)
                          if (v->time < t_old)
                            {
                              t_old = v->time;
                              v_old = v;
                            }
                        vars.del ((void *)v_old);
                      }
                  }
                delete var;
                var = NULL;
                break;
              case OP_PLUS:
                d += symbol->value;
                break;
              case OP_MINUS:
                d -= symbol->value;
                break;
              case OP_TIMES:
                d *= symbol->value;
                break;
              case OP_DIV:
                d /= symbol->value;
                break;
              case OP_MOD:
                d = fmod (d, symbol->value);
                break;
              case OP_POW:
                d = pow (d, symbol->value);
                break;
              case OP_SHIFT_L:
                d = ((unsigned long int)d) << ((unsigned long int)symbol->value);
                break;
              case OP_SHIFT_R:
                d = ((unsigned long int)d) >> ((unsigned long int)symbol->value);
                break;
              case OP_AND:
                d = ((unsigned long int)d) & ((unsigned long int)symbol->value);
                break;
              case OP_OR:
                d = ((unsigned long int)d) | ((unsigned long int)symbol->value);
                break;
            }
          push_symbol (new symbol_type (symbol->pos, d));
          delete op;
          delete symbol;
          return 0;
        }
      
      else if (symbol->type == S_BRACE_L)
        {
          int old_braces = braces;
          braces++;
          while (braces != old_braces)
            {
              parse_expsimple ();
              if (error[0] != 0)
                break;
            }
          delete symbol;
        }

      else if (symbol->type == S_BRACE_R)
        {
          if (!lvalue)
            snprintf (error, MSG_SIZE, "missing value in braces");
          else if (op != NULL)
            snprintf (error, MSG_SIZE, "missing rvalue for operator");
          else if (braces == 0)
            snprintf (error, MSG_SIZE, "extra ')' used");
          else
            {
              braces--;
              push_symbol (new symbol_type (symbol->pos, d));
              delete symbol;
              if (op != NULL)
                delete op;
              return 0;
            }
          break;
        }

      else if (symbol->type == S_ID)
        {
          if (parse_exp_id (symbol))
            {
              if (lvalue)
                {
                  snprintf (error, MSG_SIZE, "invalid lvalue in assignment");
                  break;
                }
              var = symbol;
            }
          if (error[0] != 0)
            break;
        }
    }

  // only reached in case of error
  if (symbol != NULL)
    delete symbol;
  if (op != NULL)
    delete op;
  if (var != NULL)
    delete var;
  return 0;
}

/*
  push the value of <symbol> into the stack, unless it's a variable in an
  assignment operation. <symbol> can be a defined variable, a constant or a
  function name.
  return 1 if it's a variable and the next symbol in the stack is an assign
  operator, 0 otherwise.
*/
bool
ModCalc::parse_exp_id (symbol_type *symbol)
{
  // check if it's a function
  bool is_function = 0;
  size_t i;
  for (i = 0; funcs[i] != NULL; i++)
    if (symbol->id == funcs[i])
      {
        is_function = 1;
        break;
      }
  if (!is_function && top != NULL && top->type == S_BRACE_L)
    {
      snprintf (error, MSG_SIZE, "unknown function '%s'", (c_char)symbol->id);
      return 0;
    }
  if (is_function)
    {
      if (top == NULL || top->type != S_BRACE_L)
        {
          snprintf (error, MSG_SIZE, "missing parameter for function '%s'", (c_char)symbol->id);
          return 0;
        }
      delete pop_symbol ();
      int old_braces = braces;
      braces++;
      while (braces != old_braces)
        {
          parse_expsimple ();
          if (error[0] != 0)
            return 0;
        }
      symbol_type *s = pop_symbol ();
      if (s == NULL || s->type != S_NUM)
        {
          snprintf (error, MSG_SIZE, "FUNCTION PARAMETER ERROR!");
          return 0;
        }
      double parameter = s->value, result;
      if (symbol->id == funcs[FUNC_SQRT])
        result = sqrt (parameter);
      else if (symbol->id == funcs[FUNC_FABS])
        result = fabs (parameter);
      else if (symbol->id == funcs[FUNC_CEIL])
        result = ceil (parameter);
      else if (symbol->id == funcs[FUNC_FLOOR])
        result = floor (parameter);
      else if (symbol->id == funcs[FUNC_INT])
        result = (int)parameter;
      else if (symbol->id == funcs[FUNC_EXP])
        result = exp (parameter);
      else if (symbol->id == funcs[FUNC_LOG])
        result = log (parameter);
      else if (symbol->id == funcs[FUNC_LOG10])
        result = log10 (parameter);
      else if (symbol->id == funcs[FUNC_SINH]
               || symbol->id == funcs[FUNC_SENH])
        result = sinh (parameter);
      else if (symbol->id == funcs[FUNC_COSH])
        result = cosh (parameter);
      else if (symbol->id == funcs[FUNC_TANH])
        result = tanh (parameter);
      else if (symbol->id == funcs[FUNC_COSIN]
               || symbol->id == funcs[FUNC_COSEN])
        result = pow (sin (parameter), -1);
      else if (symbol->id == funcs[FUNC_COCOS])
        result = pow (cos (parameter), -1);
      else if (symbol->id == funcs[FUNC_COTAN])
        result = pow (tan (parameter), -1);
      else if (symbol->id == funcs[FUNC_SIN]
               || symbol->id == funcs[FUNC_SEN])
        result = sin (parameter);
      else if (symbol->id == funcs[FUNC_COS])
        result = cos (parameter);
      else if (symbol->id == funcs[FUNC_TAN])
        result = tan (parameter);
      else if (symbol->id == funcs[FUNC_ASIN]
               || symbol->id == funcs[FUNC_ARSIN]
               || symbol->id == funcs[FUNC_ARCSIN]
               || symbol->id == funcs[FUNC_ASEN]
               || symbol->id == funcs[FUNC_ARSEN]
               || symbol->id == funcs[FUNC_ARCSEN])
        result = asin (parameter);
      else if (symbol->id == funcs[FUNC_ACOS]
               || symbol->id == funcs[FUNC_ARCOS]
               || symbol->id == funcs[FUNC_ARCCOS])
        result = acos (parameter);
      else if (symbol->id == funcs[FUNC_ATAN]
               || symbol->id == funcs[FUNC_ARTAN]
               || symbol->id == funcs[FUNC_ARCTAN])
        result = atan (parameter);
      else if (symbol->id == funcs[FUNC_RAND])
        {
          if (parameter >= pow ((double)2, 32)/2 || parameter < 0)
            {
              snprintf (error, MSG_SIZE, "parameter out of bounds in rand()");
              return 0;
            }
          result = random_num ((long int)parameter);
        }
      else if (symbol->id == funcs[FUNC_FACT])
        {
          if (parameter < 0)
            {
              snprintf (error, MSG_SIZE, "negative parameter in fact()");
              return 0;
            }
          result = 1;
          unsigned short int i;
          for (i = (unsigned short int)parameter; i != 0; i--)
            result *= i;
          if (result == 1 && parameter != 0 && parameter != 1)
            result = HUGE_VAL;		// overflowed
        }
      else if (symbol->id == funcs[FUNC_RAD])
        result = parameter*PI/180;
      else if (symbol->id == funcs[FUNC_DEG])
        result = parameter*180/PI;
      else if (symbol->id == funcs[FUNC_NOT])
        result = ~((unsigned long int)parameter);
      else
        {
          error_pos = symbol->pos;
          snprintf (error, MSG_SIZE, "FUNCTION LOOKUP ERROR!");
          return 0;
        }
      push_symbol (new symbol_type (symbol->pos, result));
      return 0;
    }

  // ans holds the last valid answer
  if (symbol->id == "ans")
    {
      push_symbol (new symbol_type (symbol->pos, ans));
      return 0;
    }

  // check if it's a constant
  for (i = 0; consts[i].name != NULL; i++)
    if (symbol->id == consts[i].name)
      {
        push_symbol (new symbol_type (symbol->pos, consts[i].value));
        return 0;
      }

  // if we got here, it must be a variable
  if (top != NULL && top->type == S_OP && top->op == OP_ASSIGN)
    return 1;
  var_type *v;
  vars.rewind ();
  while ((v = (var_type *)vars.next ()) != NULL)
    if (symbol->id == v->name)
      {
        push_symbol (new symbol_type (symbol->pos, v->value));
        return 0;
      }
  snprintf (error, MSG_SIZE, "unknown identifier");
  return 0;
}

// split <e> into symbols and push them into the symbol stack
void
ModCalc::scan_symbols (c_char e)
{
  char token[MSG_SIZE+1];
  c_char e_orig = e;
  size_t pos;

  while (1)
    {
      e += num_spaces (e);
      error_pos = e - e_orig;
      if (isdigit (e[0]) || e[0] == '.' || e[0] == ',')
        {
          pos = 0;

          // check if it's a binary in the form "101010b"
          bool binary = 0;
          while (isdigit (e[pos]))
            {
              if (e[pos+1] == 'b' || e[pos+1] == 'B')
                binary = 1;
              pos++;
            }
          if (binary)
            {
              if (pos > 64)
                {
                  snprintf (error, MSG_SIZE, "number too large in binary mode");
                  return;
                }
              unsigned long long num = 0;
              for (int i = pos-1; i != -1; i--)
                {
                  error_pos = e - e_orig;
                  if (e[0] != '0' && e[0] != '1')
                    {
                      snprintf (error, MSG_SIZE, "invalid number in binary mode: %c", e[0]);
                      return;
                    }
                  if (e[0] == '1')
                    num += (unsigned long long)pow ((double)2, (double)i);
                  e++;
                }
              e++;
              push_symbol_bottom (new symbol_type (error_pos+pos, (double)num));
              continue;
            }

          // check if it's a hexadecimal in the form "0x1af3"
          pos = 0;
          if (e[0] == '0' && e[1] == 'x')
            {
              e += 2;
              while (isxdigit (e[pos]))
                pos++;
              error_pos = e - e_orig;
              if (pos == 0)
                {
                  snprintf (error, MSG_SIZE, "missing number in hex mode");
                  return;
                }
              if (pos > 16)
                {
                  snprintf (error, MSG_SIZE, "number too large in hex mode");
                  return;
                }

              unsigned long long num = 0;
              for (int i = pos-1; i != -1; i--)
                {
                  error_pos = e - e_orig;
                  if (e[0] >= '0' && e[0] <= '9')
                    num += (e[0]-48) * (unsigned long long)pow ((double)16, (double)i);
                  else
                    {
                      char c = tolower (e[0]);
                      if (c >= 'a' && c <= 'f')
                        num += (c-87) * (unsigned long long)pow ((double)16, (double)i);
                    }
                  e++;
                }
              push_symbol_bottom (new symbol_type (error_pos+pos, (double)num));
              continue;
            }

          // nope, carry on with float mode
          pos = 0;
          bool dot = 0, exp = 0, expsign = 0;
          while ((isdigit (e[pos]) || e[pos] == '.' || e[pos] == ','
                  || e[pos] == '-' || e[pos] == '+' || tolower (e[pos]) == 'e')
                 && pos < MSG_SIZE)
            {
              error_pos = e - e_orig + pos;
              if (e[pos] == '.' || e[pos] == ',')
                {
                  if (dot)
                    {
                      snprintf (error, MSG_SIZE, "multiple dots in number");
                      return;
                    }
                  dot = 1;
                }
              if (tolower (e[pos]) == 'e')
                {
                  if (exp)
                    {
                      snprintf (error, MSG_SIZE, "multiple 'E's in number");
                      return;
                    }
                  exp = 1;
                }
              if (e[pos] == '-' || e[pos] == '+')
                {
                  if (!expsign && exp)
                    expsign = 1;
                  else
                    break;
                }
              if (e[pos] == ',')
                token[pos] = '.';
              else
                token[pos] = e[pos];
              pos++;
            }
          token[pos] = 0;
          e += pos;
          push_symbol_bottom (new symbol_type (error_pos, atof (token)));
        }
      else if (isalpha (e[0]))
        {
          pos = 0;
          while (isalnum (e[pos]))
            {
              token[pos] = e[pos];
              pos++;
            }
          token[pos] = 0;
          e += pos;
          push_symbol_bottom (new symbol_type (error_pos, token));
        }
      else if (e[0] == '=')
        {
          e++;
          push_symbol_bottom (new symbol_type (error_pos, OP_ASSIGN, 0));
        }
      else if (e[0] == '+')
        {
          e++;
          push_symbol_bottom (new symbol_type (error_pos, OP_PLUS, 1));
        }
      else if (e[0] == '-')
        {
          e++;
          push_symbol_bottom (new symbol_type (error_pos, OP_MINUS, 1));
        }
      else if (e[0] == '/')
        {
          e++;
          push_symbol_bottom (new symbol_type (error_pos, OP_DIV, 2));
        }
      else if (e[0] == '%')
        {
          e++;
          push_symbol_bottom (new symbol_type (error_pos, OP_MOD, 2));
        }
      else if (e[0] == '^')
        {
          e++;
          push_symbol_bottom (new symbol_type (error_pos, OP_POW, 3));
        }
      else if (e[0] == '*' && e[1] == '*')
        {
          e += 2;
          push_symbol_bottom (new symbol_type (error_pos, OP_POW, 3));
        }
      else if (e[0] == '<' && e[1] == '<')
        {
          e += 2;
          push_symbol_bottom (new symbol_type (error_pos, OP_SHIFT_L, 4));
        }
      else if (e[0] == '>' && e[1] == '>')
        {
          e += 2;
          push_symbol_bottom (new symbol_type (error_pos, OP_SHIFT_R, 4));
        }
      else if (e[0] == '&')
        {
          e++;
          push_symbol_bottom (new symbol_type (error_pos, OP_AND, 4));
        }
      else if (e[0] == '|')
        {
          e++;
          push_symbol_bottom (new symbol_type (error_pos, OP_OR, 4));
        }
      else if (e[0] == '*')
        {
          e++;
          push_symbol_bottom (new symbol_type (error_pos, OP_TIMES, 2));
        }
      else if (e[0] == '(')
        {
          e++;
          push_symbol_bottom (new symbol_type (error_pos, S_BRACE_L));
        }
      else if (e[0] == ')')
        {
          e++;
          push_symbol_bottom (new symbol_type (error_pos, S_BRACE_R));
        }
      else if (e[0] == 0 || e[0] == '#')
        {
          push_symbol_bottom (new symbol_type (error_pos, S_EOF));
          break;
        }
      else
        {
          snprintf (error, MSG_SIZE, "unmatched symbol '%c'", e[0]);
          break;
        }
    }
}

void
ModCalc::delete_symbols (void)
{
  symbol_type *symbol;
  while (top != NULL)
    {
      symbol = top->next;
      delete top;
      top = symbol;
    }
}

void
ModCalc::push_symbol_bottom (symbol_type *symbol)
{
  if (top == NULL)
    {
      push_symbol (symbol);
      bottom = top;
    }
  else
    {
      symbol->next = NULL;
      bottom->next = symbol;
      bottom = symbol;
    }
}

void
ModCalc::push_symbol (symbol_type *symbol)
{
  symbol->next = top;
  top = symbol;
}

ModCalc::symbol_type *
ModCalc::pop_symbol (void)
{
  if (top == NULL)
    return NULL;
  symbol_type *symbol = top;
  top = top->next;
  return symbol;
}

/////////////
// commands
/////////////

// !calc/calch/calcb/calco/calcr auxiliary function
// return 1 if "result" was already sent, 0 otherwise
static bool
calc_cmd_calc_aux (NetServer *s, double *result)
{
  ModCalc *calc = server2calc (s);
  if (calc == NULL)
    return 1;
  strsplit (CMD[3], BUF, 1);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_CALC);
      return 1;
    }
  if (strcasecmp (BUF[1], "help") == 0)
    {
      SEND_TEXT (DEST, "Use the source, Luke...");
      return 1;
    }
  *result = calc->calc (BUF[1]);
  if (error[0] != 0)
    SEND_TEXT (DEST, "Error at %d: %s.", calc->error_pos+1, error);
  else if (*result == HUGE_VAL)
    SEND_TEXT (DEST, "Result too large.");
  else if (*result == -HUGE_VAL)
    SEND_TEXT (DEST, "Result too small.");
  else if (isnan (*result))
    SEND_TEXT (DEST, "Not a number.");
  else
    return 0;
  return 1;
}

// !calc expression
static void
calc_cmd_calc (NetServer *s)
{
  double result;
  if (calc_cmd_calc_aux (s, &result))
    return;

  char buf[MSG_SIZE+1];
  snprintf (buf, MSG_SIZE, "%.15f", result);
  if (strchr (buf, '.') != NULL)
    {
      size_t len = strchr (buf, '.')-buf;
      if (len > 15 || (1E-15 > result && result > 0))
        {
          SEND_TEXT (DEST, "%.15e", result);
          return;
        }
      len = strlen (buf);
      if (len > (size_t)(16 + (result < 0)))
        {
          buf[16 + (result < 0)] = 0;
          len = strlen (buf);
        }
      for (size_t i = len-1; buf[i] == '0' || buf[i] == '.'; i--)
        {
          if (buf[i] == '.')
            {
              buf[i] = 0;
              break;
            }
          buf[i] = 0;
        }
    }
  SEND_TEXT (DEST, "%s", buf);
}

// !calch expression
static void
calc_cmd_calch (NetServer *s)
{
  double result;
  if (calc_cmd_calc_aux (s, &result))
    return;

  char buf[MSG_SIZE+1];
  if (result >= pow (2.0, (double)(sizeof (unsigned long long) * 8)))
    snprintf (buf, MSG_SIZE, "Result too large.");
  else if (result == (unsigned long long)result)
    snprintf (buf, MSG_SIZE, "0x%llx", (unsigned long long)result);
  else
    snprintf (buf, MSG_SIZE, "0x%llx (rounded)", (unsigned long long)result);
  if ((unsigned long long)result >= (unsigned long long)1<<53)
    snprintf (buf+strlen (buf), MSG_SIZE-strlen (buf), " (possible loss of precision!)");
  SEND_TEXT (DEST, "%s", buf);
}

// !calcb expression
static void
calc_cmd_calcb (NetServer *s)
{
  double result;
  if (calc_cmd_calc_aux (s, &result))
    return;

  char buf[MSG_SIZE+1];
//  if (result >= pow (2.0, (double)(8*sizeof (unsigned long long))))
  if (result > 1.844674407370955e+19)
    snprintf (buf, MSG_SIZE, "Result too large.");
  else
    {
      unsigned long long llresult = (unsigned long long)result;
      int i, one = 0, bufpos = 0;
      for (i = 8*sizeof (unsigned long long)-1; i >= 0; i--)
        {
          if ((llresult & ((unsigned long long)1<<i)) != 0)
            one = 1;
          if (one)
            buf[bufpos++] = '0' + ((llresult & ((unsigned long long)1<<i)) != 0);
        }
      if (bufpos == 0)
        buf[bufpos++] = '0';
      buf[bufpos++] = 'b';
      buf[bufpos] = 0;
      if (result != llresult)
        snprintf (buf+strlen (buf), MSG_SIZE-strlen (buf), " (rounded)");
      if (llresult >= (unsigned long long)1<<53)
        snprintf (buf+strlen (buf), MSG_SIZE-strlen (buf), " (possible loss of precision!)");
    }
  SEND_TEXT (DEST, "%s", buf);
}

// !calco expression
static void
calc_cmd_calco (NetServer *s)
{
  double result;
  if (calc_cmd_calc_aux (s, &result))
    return;

  char buf[MSG_SIZE+1];
  if (result >= pow (2, (double)(sizeof (long long) * 8)))
    snprintf (buf, MSG_SIZE, "Result too large.");
  else if (result == (unsigned long long)result)
    snprintf (buf, MSG_SIZE, "0%llo", (unsigned long long)result);
  else
    snprintf (buf, MSG_SIZE, "0%llo (rounded)", (unsigned long long)result);
  SEND_TEXT (DEST, "%s", buf);
}


// !calcr expression
static void
calc_cmd_calcr (NetServer *s)
{
  double result;
  if (calc_cmd_calc_aux (s, &result))
    return;

  char buf[MSG_SIZE+1];
  if (abs ((int)result) >= 10000)
    snprintf (buf, MSG_SIZE, "Result too large.");
  else
    {
      int iresult = (int)result;
      int digits = 0;
      if (iresult != abs (iresult))
        snprintf (buf, MSG_SIZE, "-");
      else
        buf[0] = 0;
      iresult = abs (iresult);

      // count the digits of iresult
      while (iresult != 0)
        {
          iresult /= 10;
          digits++;
        }

      // convert each digit according to its position
      iresult = abs((int)result);
      while (digits != 0)  
        {
          int d = iresult / (int)(pow (10.0, digits-1));
          switch (digits)
            {
              case 1:
                switch (d)
                  {
                    case 1:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "I");
                      break;
                    case 2:
       	              snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "II");
                      break;
                    case 3:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "III");
                      break;
                    case 4:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "IV");
                      break;
                    case 5:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "V");
                      break;
                    case 6:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "VI");
                      break;
                    case 7:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "VII");
                      break;
                    case 8:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "VIII");
                      break;
                    case 9:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "IX");
                      break;
                  }
                break;
              case 2:
                switch (d)
		  {
                    case 1:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "X");
                      break;
                    case 2:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "XX");
                      break;
                    case 3:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "XXX");
                      break;
                    case 4:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "XL");
		      break;
                    case 5:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "L");
                      break;
                    case 6:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "LX");
                      break;
                    case 7:
     		      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "LXX");
                      break;
                    case 8:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "LXXX");
		      break;
                    case 9:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "XC");
                      break;
                  }
                break;
              case 3:
     		switch (d)
                  {
                    case 1:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "C");
                      break;
                    case 2:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "CC");
                      break;
                    case 3:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "CCC");
                      break;
                    case 4:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "CD");
                      break;
                    case 5:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "D");
		      break;
                    case 6:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "DC");
                      break;
                    case 7:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "DCC");
                      break;
                    case 8:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "DCCC");
                      break;
                    case 9:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "CM");  
                      break;
                  }
                break;
              case 4:
                switch (d)
                  {
                    case 1: 
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "M");
                      break;
                    case 2: 
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "MM");
                      break;
                    case 3: 
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "MMM");
                      break;
                    case 4:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "MMMM");
                      break;
                    case 5: 
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "MMMMM");
                      break;
                    case 6: 
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "MMMMMM");
                      break;
                    case 7: 
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "MMMMMMM");
                      break;
                    case 8:
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "MMMMMMMM");
                      break;
                    case 9: 
                      snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "MMMMMMMMM");
     		      break;
                  }
                break;
              default:
                snprintf (buf+strlen(buf), MSG_SIZE-strlen(buf), "(ERROR)");
                break;
            }
          iresult %= (int)(pow (10.0, digits-1));
          digits--;
        }

      if (buf[0] == 0)
        snprintf (buf, MSG_SIZE, "(zero)");
      if (result != (int)result)
        snprintf (buf+strlen (buf), MSG_SIZE-strlen (buf), " (rounded)");
    }
  SEND_TEXT (DEST, "%s", buf);
}

////////////////////
// module managing
////////////////////

// returns the calc object for a given server, NULL if nonexistant
static ModCalc *
server2calc (NetServer *s)
{
  ModCalc *calc;
  calc_list->rewind ();
  while ((calc = (ModCalc *)calc_list->next ()) != NULL)
    if (calc->s == s)
      return calc;
  return NULL;
}

// adds a calc with <server> to the list
static void
calc_add (NetServer *s)
{
  calc_list->add ((void *)new ModCalc (s));
}

// configuration file's local parser
static void
calc_conf (NetServer *s, c_char bufread)
{
  char buf[2][MSG_SIZE+1];

  strsplit (bufread, buf, 1);

  if (strcasecmp (buf[0], "bot") == 0)
    {
      calc_add (s);
      s->script.cmd_bind (calc_cmd_calc, LEVEL_CALC, "!calc", module.mod, HELP_CALC);
      s->script.cmd_bind (calc_cmd_calch, LEVEL_CALC, "!calch", module.mod, NULL);
      s->script.cmd_bind (calc_cmd_calcb, LEVEL_CALC, "!calcb", module.mod, NULL);
      s->script.cmd_bind (calc_cmd_calco, LEVEL_CALC, "!calco", module.mod, NULL);
      s->script.cmd_bind (calc_cmd_calcr, LEVEL_CALC, "!calcr", module.mod, NULL);
    }
}

// module termination
static void
calc_stop (void)
{
  ModCalc *calc;
  calc_list->rewind ();
  while ((calc = (ModCalc *)calc_list->next ()) != NULL)
    delete calc;
  delete calc_list;
}

// module initialization
static void
calc_start (void)
{
  calc_list = new List ();
}

