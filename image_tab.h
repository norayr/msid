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

#ifndef MSID_IMAGE_TAB
#define MSID_IMAGE_TAB

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#ifdef HILDON
#include <hildon/hildon.h>
#endif

#include "plugins/curling.h"

GtkWidget* create_image_page (void (*status_text_func) (const char*, const char*));

void darken_current_animation_image ();

void set_game_screenshot (char *file);

void play_screenshot_animation();
void stop_screenshot_animation();

#endif
