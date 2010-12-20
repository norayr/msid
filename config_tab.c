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

#include "config_tab.h"

#define PLUGIN_PATH "./"

GtkWidget *config_audio_plugin_button;
GtkWidget *config_audio_plugin_combo;
GtkWidget *config_search_plugin_button;
GtkWidget *config_search_plugin_combo;

GtkWidget *hvsc_path_selection_button;
GtkWidget *hvsc_path_label;

GtkWidget *output_mode_toggle;

//
// TODO - make this all more generic and error-proof
//

static GtkWidget *
create_plugin_chooser(const char *title, const char *path)
{
  GtkWidget *cbox;
  GError *error =NULL;
  const gchar *fname =NULL;
  GDir *dir;

  cbox = gtk_combo_box_new_text();
  dir = g_dir_open(path, 0, &error);

  gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), title);

  if (dir) {
    fname = g_dir_read_name(dir);

    while (fname != NULL) {
      gtk_combo_box_append_text(GTK_COMBO_BOX(cbox), fname);
      fname = g_dir_read_name(dir);
    }

    g_dir_close(dir);
  }

  gtk_combo_box_set_active(GTK_COMBO_BOX(cbox), 0);
  return cbox;
}


GtkWidget* create_config_page(void)
{
  GtkWidget

    *config_main_box,

    *a_plugin_box,
    *s_plugin_box,
    *hvsc_box,

    *output_label,
    *label,
    *sep2,
    *sep,

    *plugin_label;

  char buffer[256];

  // FIXME - make this page look better

  //
  // TODO - include local HVSC path entry and button
  //

  // 'built-in' local HVSC support? not a *REAL* plugin?
  // if (active_search_plugin == NULL) then try local and only
  // after that - FAIL

  config_main_box = gtk_vbox_new(FALSE, 4);

  a_plugin_box = gtk_hbox_new(FALSE, 0);
  s_plugin_box = gtk_hbox_new(FALSE, 0);
  hvsc_box = gtk_hbox_new(FALSE, 0);

  label = gtk_label_new("Local HVSC path");
  output_label = gtk_label_new("Output mode");
  output_mode_toggle = gtk_toggle_button_new_with_label("Stereo output");
  hvsc_path_selection_button  = create_ui_button("not supported yet");

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_mode_toggle), TRUE);

  gtk_widget_set_sensitive(hvsc_path_selection_button, FALSE);

  plugin_label = gtk_label_new("Choose plugins");
  hvsc_path_label = gtk_label_new("");

  gtk_label_set_markup(GTK_LABEL(label), "<b>Local HVSC path</b>");
  gtk_label_set_markup(GTK_LABEL(output_label), "<b>Output mode</b>");
  gtk_label_set_markup(GTK_LABEL(plugin_label), "<b>Choose plugins</b>");
  gtk_label_set_markup(GTK_LABEL(hvsc_path_label), "<i>unspecified</i>");

  sep = gtk_hseparator_new();
  sep2 = gtk_hseparator_new();

  snprintf(buffer, 256, "%s/audio", PLUGIN_FILE_PATH);
  config_audio_plugin_combo = create_plugin_chooser("audio", buffer);

  snprintf(buffer, 256, "%s/search", PLUGIN_FILE_PATH);
  config_search_plugin_combo = create_plugin_chooser("search", buffer);

  config_audio_plugin_button  = gtk_button_new_with_label("load");
  config_search_plugin_button = gtk_button_new_with_label("load");

  gtk_box_pack_start(GTK_BOX(a_plugin_box), config_audio_plugin_combo,  TRUE,  TRUE,  2);
  gtk_box_pack_start(GTK_BOX(a_plugin_box), config_audio_plugin_button, FALSE, FALSE, 2);

  gtk_box_pack_start(GTK_BOX(s_plugin_box), config_search_plugin_combo,  TRUE,  TRUE,  2);
  gtk_box_pack_start(GTK_BOX(s_plugin_box), config_search_plugin_button, FALSE, FALSE, 2);


  /*
    TODO - add local HVSC support!
    gtk_box_pack_start (GTK_BOX(config_main_box), label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(hvsc_box), hvsc_path_selection_button, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX(config_main_box), hvsc_box, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(config_main_box), hvsc_path_label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX(config_main_box), sep, FALSE, FALSE, 0);
  */

  gtk_box_pack_start(GTK_BOX(config_main_box), output_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(config_main_box), output_mode_toggle, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(config_main_box), sep2, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(config_main_box), plugin_label, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(config_main_box), a_plugin_box, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(config_main_box), s_plugin_box, FALSE, FALSE, 0);

  return config_main_box;
}
