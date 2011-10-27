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

#ifndef MSID_PLAYER_TAB
#define MSID_PLAYER_TAB

#include <sidplay/player.h>
#include <sidplay/fformat.h>
#include <sidplay/myendian.h>

#include "sid_test.h"
#include "ui.h"

enum
  {
    PLAYER_LIST_SONG_NAME = 0,
    PLAYER_LIST_SONG_AUTHOR = 1,
    PLAYER_LIST_SONG_AMOUNT,
    PLAYER_LIST_SONG_PATH,
    PLAYER_LIST_NUM_COLS
  };

GtkWidget* create_player_page (void (*status_text_func) (const char*, const char*));

void
refresh_player_songlist(GtkWidget *widget,
                        gpointer data);

#endif
