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

#ifndef MSID_SEARCH_IFACE_H
#define MSID_SEARCH_IFACE_H

#include <string.h>
#include <glib.h>

#include "curling.h"
#include "debug.h"

/*
 * search_for_sid returns a single-linked list
 * of msid_search_entries, variable uri has the
 * complete uri where sid file is located
 */
typedef struct _msid_search_entry
{
  char *uri;
  char *file_name;
  char *hvsc_path;   /* shown in the player ui */

} msid_search_entry;

/*
 * interface for creating search plugins
 * no pre-initialization, just search function
 *
 */
class msid_search_plugin
{
 public :
  msid_search_plugin() {;}
  virtual ~msid_search_plugin() {;}

  virtual unsigned int init() = 0;
  virtual GSList *search_for_sid (const char *needle, gpointer data) = 0;

 private :
};

typedef msid_search_plugin* create_search_plugin();
typedef void destroy_search_plugin(msid_search_plugin*);

#endif
