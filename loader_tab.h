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

#ifndef MSID_SEARCH_TAB
#define MSID_SEARCH_TAB

#include <glib.h>
#include <glib/gstdio.h>

#include <stdlib.h>

#include "plugins/search_iface.h"
#include "ui.h"
#include "sid_test.h"

enum {
  LOADER_LIST_SONG_NAME = 0,
  LOADER_LIST_SONG_URI,
  LOADER_LIST_SONG_SHORT_URI,
  LOADER_LIST_NUM_COLS
};

GtkWidget *create_loader_page(msid_search_plugin *plugin,
			      void (*status_text_func) (const char*, const char*));

#endif
