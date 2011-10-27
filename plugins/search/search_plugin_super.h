/*
 * Copyright (C) 2008 Tapani Pälli <lemody@c64.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef MSID_SUPER_SEARCH_PLUGIN
#define MSID_SUPER_SEARCH_PLUGIN

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "../search_iface.h"

class super_search : public msid_search_plugin
{
  public :

  super_search() { tree = NULL; }
  ~super_search();

  GSList *search_for_sid (const char *needle, gpointer data);
  unsigned int init(); // TODO - load local database here

  private :
  GTree *tree;
};

// class factories

extern "C" void* create() {
  return new super_search;
}

extern "C" void destroy (void *plug) {
  delete (super_search *) plug;
}

#ifdef __cplusplus
}
#endif

#endif
