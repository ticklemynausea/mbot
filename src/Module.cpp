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

#include "Module.h"

Module::Module (c_char file) : initialized (0), b (bot),
  filename (file, PATH_MAX), module (NULL), handle (NULL)
{
#ifdef WINDOZE
  handle = LoadLibrary (file);
#else		// !WINDOZE
  if (filename[0] == '/')		// if an absolute path was specified
    handle = dlopen (filename, RTLD_LAZY);
  else					// else build it
    {
      char *cwd = getcwd (NULL, PATH_MAX);
      if (cwd == NULL)
        return;
      size_t len = strlen (cwd) + filename.getlen () + 10;
      char *path = (char *)my_malloc (len+1);
      snprintf (path, len, "%s/%s", cwd, (c_char)filename);
      free (cwd);
      handle = dlopen (path, RTLD_LAZY);
      free (path);
    }
#endif		// !WINDOZE

  if (handle == NULL)
    {
      bot->write_botlog ("error loading module: %s", geterror ());
      return;
    }

  module = (struct module_type *)getsym ("module");
  if (module == NULL)
    {
      module = (struct module_type *)getsym ("_module");
      if (module == NULL)
        {
          bot->write_botlog ("error: getsym() returned: %s", geterror ());
          return;
        }
    }

  if (module->name == NULL)
    {
      bot->write_botlog ("error: invalid module");
      return;
    }
  if (bot->get_module (module->name) != NULL)
    {
      bot->write_botlog ("error: module already loaded");
      return;
    }
  if (module->version != MODULE_VERSION)
    {
      bot->write_botlog ("error: module was compiled with another version of mbot, recompile it");
      return;
    }

  module->mod = this;
  if (module->start != NULL)
    module->start ();

  initialized = 1;
}

Module::~Module (void)
{
  if (initialized && module != NULL)
    {
      if (module->stop != NULL)
        module->stop ();
      // unbind all commands which use this module
      Script::cmd_type *c;
      List *l;
      for (int i = 0; i < b->server_num; i++)
        {
          l = &b->servers[i]->script.cmds;
          l->rewind ();
          while ((c = (Script::cmd_type *)l->next ()) != NULL)
            {
              if (c->module != this)
                continue;
              delete c;
              l->del ((void *)c);
              l->rewind ();	// del() may invalidate/change our position
            }
        }
    }
  if (handle != NULL)
#ifdef WINDOZE
    FreeLibrary (handle);
#else		// !WINDOZE
    dlclose (handle);
#endif		// !WINDOZE
}

void *
Module::getsym (c_char sym) const
{
  if (handle == NULL)
    return NULL;
#ifdef WINDOZE
  return GetProcAddress (handle, sym);
#else
  return dlsym (handle, sym);
#endif
}

c_char
Module::geterror (void) const
{
#ifdef WINDOZE
  static char buf[MSG_SIZE+1];
  FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, GetLastError(),
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 buf, MSG_SIZE, NULL);
  return buf;
#else
  return dlerror ();
#endif
}

void
Module::conf (NetServer *s, c_char line) const
{
  if (module != NULL && module->conf != NULL)
    module->conf (s, line);
}

Module::operator c_char (void) const
{
  if (module == NULL)
    return NULL;
  return module->name;
}
