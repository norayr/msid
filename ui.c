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

#include "ui.h"

gchar*
ask_for_directory (GtkWidget *window)
{
  GtkWidget *selector;
  gint result;
  char *dir =NULL;

#ifndef HILDON
  selector = gtk_file_chooser_dialog_new ("Please select HVSC directory",
					  GTK_WINDOW(window),
					  GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  GTK_STOCK_OK, GTK_RESPONSE_OK,
					  NULL);
#else
  selector = hildon_file_chooser_dialog_new  (GTK_WINDOW(window),
					      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
#endif

  result = gtk_dialog_run (GTK_DIALOG (selector));

  if ((result == GTK_RESPONSE_OK) || (result == GTK_RESPONSE_ACCEPT))
  {
    dir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (selector));
  }

  gtk_widget_destroy (selector);
  return dir;
}


#ifdef HANDHELD_UI
static GtkWidget *
create_big_button (const char *txt, int w, int h)
{
  GtkWidget *button, *box, *label;

  button = gtk_button_new();
  box    = gtk_event_box_new();
  label  = gtk_label_new(txt);

  gtk_event_box_set_visible_window(GTK_EVENT_BOX(box),
                                   FALSE);
  gtk_container_add (GTK_CONTAINER (box), label);
  gtk_widget_set_size_request(box, w, h);
  gtk_container_add (GTK_CONTAINER (button), box);

  return button;
}

static GtkWidget *
create_big_toggle_button (const char *txt, int w, int h)
{
  GtkWidget *button, *box, *label;
  
  button = gtk_toggle_button_new();
  box    = gtk_event_box_new();
  label  = gtk_label_new(txt);

  gtk_event_box_set_visible_window(GTK_EVENT_BOX(box),
                                   FALSE);
  gtk_container_add (GTK_CONTAINER (box), label);
  gtk_widget_set_size_request(box, w, h);
  gtk_container_add (GTK_CONTAINER (button), box);

  return button;
}
#endif

GtkWidget *
create_ui_button (const char *txt)
{
#ifdef HANDHELD_UI
  return create_big_button (txt, 64, 60);
#else
  return gtk_button_new_with_label (txt);
#endif
}

GtkWidget *
create_ui_toggle_button (const char *txt)
{
#ifdef HANDHELD_UI
  return create_big_toggle_button (txt, 64, 60);
#else
  return gtk_toggle_button_new_with_label (txt);
#endif
}

GtkWidget *
create_huge_button (const char *txt)
{
#ifdef HANDHELD_UI
  return create_big_button (txt, 100, 60);
#else
  return gtk_button_new_with_label (txt);
#endif
}
