/*
 * Copyright (C) 2008 Tapani Pälli <lemody@gmail.com>
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "loader_tab.h"
#include "player_tab.h"
#include "config_tab.h"
#include "image_tab.h"

#include "msidmachine.h"

#ifdef HILDON
#include <hildon/hildon.h>
#endif

#define PROGRAM_VERSION "0.4"
#define PROGRAM_NAME "msid - " PROGRAM_VERSION

#define APP_WINDOW_HEIGHT 400 // ~hildonwindow workspace

static GtkWidget *app_window;

static GtkWidget *status_text=NULL;

extern GtkWidget *player_song_list;
extern GtkWidget *loader_song_list;
extern GtkWidget *player_play_button;
extern GtkWidget *player_pause_button;
extern GtkWidget *player_read_button;
extern GtkWidget *player_del_button;
extern GtkWidget *subsong_selector;

static GtkWidget *config_dialog = NULL;

extern GtkWidget *config_audio_plugin_button;
extern GtkWidget *config_search_plugin_button;
extern GtkWidget *config_audio_plugin_combo;
extern GtkWidget *config_search_plugin_combo;
extern GtkWidget *output_mode_toggle;

extern GtkWidget *hvsc_path_selection_button;
extern GtkWidget *hvsc_path_label;

extern GtkWidget *image_button;
extern GtkWidget *anim_control_button;

msid_search_plugin *msid_active_search_plugin = NULL;

static gboolean ignore_subsong_change = FALSE;

static guint image_page_num = 0;

static guint SUBSONG = 0;

char download_path[256];

static GtkTreePath *current_selection = NULL;


// TODO - handles all logic for search and playback
msidMachine msid;


// function declarations
static void select_song (GtkTreeView *treeview, gpointer data);
static void modify_subsong_selector();
void update_status_text (const char *text, const char *color);



// set fullscreen state for window (N770, N800, N810)
static gint
msid_on_key_release_event(GtkWidget *widget, GdkEventKey *event)
{
#ifdef HILDON
  static gboolean fullscreen=FALSE;
#endif
  switch (event->keyval) {
#ifdef HILDON
  case HILDON_HARDKEY_FULLSCREEN:
    if (!fullscreen)
      gtk_window_fullscreen(GTK_WINDOW (widget));
    else
      gtk_window_unfullscreen(GTK_WINDOW (widget));

    fullscreen = !fullscreen;

    return TRUE;
    break;
#endif
  default :
    break;
  }
  return FALSE;
}

// default text to show as status after specified timeout
static gboolean
set_ready_text_timeout(gpointer data)
{
  gdk_threads_enter();
  update_status_text("READY.", C64_FG);
  gdk_threads_leave();

  return FALSE;
}

// change status text
void
update_status_text(const char *text, const char *c)
{
  gchar buffer[256];
  const char *color;

  if (!c) {
    color = C64_FG;
  } else {
    color = c;
  }

  if (!strstr(text, "READY")) {
    g_timeout_add(2500, set_ready_text_timeout, NULL);
  }

  snprintf(buffer, 256, "<span foreground=\"%s\"><b>%s</b></span>", color, text);
  gtk_label_set_markup(GTK_LABEL(status_text), buffer);
}

// pack to vbox with a separator
static void
append_to_vbox (GtkWidget *box, GtkWidget *w,
		gboolean a, gboolean b)
{
  GtkWidget *sep = gtk_vseparator_new();

  gtk_box_pack_start(GTK_BOX(box), sep, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), w, a, b, 0);
}



static void
load_screenshot(GtkWidget *widget, gpointer data)
{
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  gchar *filepath=NULL;

  if (!current_selection) {
    return;
  }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(player_song_list));

  if (gtk_tree_model_get_iter(model, &iter, current_selection)) {
    gtk_tree_model_get(model, &iter, PLAYER_LIST_SONG_PATH, &filepath, -1);
  }

  if (!filepath) {
    return;
  }

  update_status_text("LOADING IMAGES", C64_FG);

  gtk_widget_set_sensitive(widget, FALSE);

  set_game_screenshot(filepath);
}

static void
select_hvsc_path (GtkWidget *widget,
		  gpointer data)
{
  gchar *dir = ask_for_directory(config_dialog);
  char *markup;

  if (dir) {
    g_print("path: %s\n", dir);

    markup = g_markup_printf_escaped("<span style=\"italic\">%s</span>", dir);
    gtk_label_set_markup(GTK_LABEL(hvsc_path_label), markup);
    g_free(markup);
    g_free(dir);
  }
}

static void
load_audio_plugin(GtkWidget *widget, gpointer data)
{
  char buffer[256];
  gchar *plugin=NULL;

  if ((gtk_combo_box_get_active(GTK_COMBO_BOX(data)) < 1))
    return;

  if (msid.isPlaying()) {
    update_status_text("cannot update plugin while playing", "#ffaaaa");
    return;
  }

  plugin = gtk_combo_box_get_active_text(GTK_COMBO_BOX(data));

  if (plugin) {

    if (msid.setAudioPlugin(plugin)) {
      snprintf(buffer, 256, "plugin [<b>%s</b>] loaded", plugin);
      update_status_text(buffer, "#aaffaa");
    } else {
      snprintf(buffer, 256, "error loading [<b>%s</b>]", plugin);
      update_status_text(buffer, "#ffaaaa");
    }
    g_free(plugin);
  }
}


static void
load_search_plugin(GtkWidget *widget, gpointer data)
{
  char buffer[256];
  gchar *plugin=NULL;

  // FIXME - cannot update plugin while searching!

  if ((gtk_combo_box_get_active(GTK_COMBO_BOX(data)) < 1))
    return;

  plugin = gtk_combo_box_get_active_text(GTK_COMBO_BOX(data));

  if (plugin) {

    if (msid.setSearchPlugin(plugin)) {
      snprintf(buffer, 256, "plugin [<b>%s</b>] loaded", plugin);
      update_status_text(buffer, "#aaffaa");
    } else {
      snprintf(buffer, 256, "error loading [<b>%s</b>]", plugin);
      update_status_text(buffer, "#ffaaaa");
    }
    g_free(plugin);

    msid_active_search_plugin = (msid_search_plugin *) msid.searchPlugin()->plugin_object;
    msid_active_search_plugin->init();
  }
}

// pause callback, sets mutex state according to pause button state
static void
pause_song(GtkWidget *widget, gpointer data)
{
  msid.pause();

  if (msid.isPaused()) {
    gtk_widget_set_sensitive(player_play_button, FALSE);
  } else {
    gtk_widget_set_sensitive(player_play_button, TRUE);
  }
}


static void
play_stop_song(GtkWidget *widget, gpointer data)
{

  if (!current_selection) {
    update_status_text("NO SONG SELECTED", "#ffaaaa");
    return;
  }

  if (msid.isPlaying() != 0) {

    msid.stop();

    update_status_text("READY.", "#aaaaff");
    gtk_widget_set_sensitive(player_play_button, FALSE);
    gtk_widget_set_sensitive(player_pause_button, FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(player_pause_button), FALSE);

    return;
  }

  if (msid.loop()) {

    gtk_widget_set_sensitive(player_play_button, TRUE);
    gtk_widget_set_sensitive(player_pause_button, TRUE);

  }

  msid.play();

  return;
}


static void
quit_msid(GtkMenuItem *menuitem, gpointer data)
{
  msid.stop();

  gtk_main_quit();
}


static void
play_selection(GtkTreeView       *tree_view,
	       GtkTreePath       *path,
	       GtkTreeViewColumn *column,
	       gpointer           user_data)
{
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  gchar *filepath=NULL;

  if (msid.isPlaying()) {

    msid.stop();
  }

  model = gtk_tree_view_get_model(tree_view);

  select_song(tree_view, path);

  // update subsong selector only when selection made,
  // not on cursor change
  modify_subsong_selector();

  if (gtk_tree_model_get_iter(model, &iter, path)) {

    gtk_tree_model_get(model, &iter, PLAYER_LIST_SONG_PATH, &filepath, -1);

    if(!filepath) {
      return;
    } else {

      msid.setSong(filepath, SUBSONG);

      g_free(filepath);

      play_stop_song(NULL, NULL);

      return;
    }

  }
}


static void
handle_selection (GtkTreeView       *tree_view,
		  GtkTreePath       *path,
		  GtkTreeViewColumn *column,
		  gpointer           user_data)
{
  GtkTreeModel     *model;

  model = gtk_tree_view_get_model(tree_view);
  play_selection(tree_view, path, column, user_data);
}


static unsigned int
get_subsong_amount()
{
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  unsigned int amount = 0;

  if (!current_selection) {
    return 0;
  }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(player_song_list));

  if (gtk_tree_model_get_iter (model, &iter, current_selection)) {
    gtk_tree_model_get (model, &iter, PLAYER_LIST_SONG_AMOUNT, &amount, -1);
  }

  return amount;
}


static void replay()
{
  if (msid.isPlaying()) {

    msid.stop();
    msid.setSubSong(SUBSONG);
    play_stop_song(NULL, NULL);
  }
}


static void
subsong_changed(GtkComboBox *widget, gpointer data)
{
  unsigned int amount;

  amount = get_subsong_amount();

  SUBSONG = gtk_combo_box_get_active (widget);
  if (SUBSONG < 0 || SUBSONG > amount) {
    SUBSONG = 1;
  }

  if (ignore_subsong_change) {
    ignore_subsong_change = FALSE;
    return;
  }

  // replay sid with different subsong
  replay();
}


static void
modify_subsong_selector()
{
  GtkTreeModel *model;
  guint new_amount;
  guint prev_amount;
  char str[8];

  model = gtk_combo_box_get_model (GTK_COMBO_BOX(subsong_selector));
  new_amount = get_subsong_amount();

  if (new_amount <= 0) {
    printf ("[%s] - no subsongs found\n", __func__);
    return;
  }
  // account for SONG
  prev_amount = gtk_tree_model_iter_n_children (model, NULL) - 1;
  //printf("%d %d\n", prev_amount, new_amount);

  if (prev_amount < new_amount) {
    // add new entries if needed
    while (prev_amount < new_amount) {
      prev_amount++;
      sprintf(str, "%d", prev_amount);
      gtk_combo_box_append_text(GTK_COMBO_BOX(subsong_selector), str);
    }

    // set same subsong as was selected before!
    gtk_combo_box_set_active (GTK_COMBO_BOX(subsong_selector), SUBSONG);
  }

  else if (prev_amount > new_amount) {
    // remove entries if needed
    while (prev_amount > new_amount) {
      gtk_combo_box_remove_text(GTK_COMBO_BOX(subsong_selector),
				prev_amount);
      prev_amount--;
    }

    // ignore this change as it's program making it
    ignore_subsong_change = TRUE;

    if (SUBSONG <= new_amount)
      gtk_combo_box_set_active (GTK_COMBO_BOX(subsong_selector), SUBSONG);
    else
      gtk_combo_box_set_active (GTK_COMBO_BOX(subsong_selector), 0);
  }

}

// delete current sid song with confirmation dialog
static void
delete_current_selection(void)
{
  GtkTreeModel     *model;
  GtkTreeIter       iter, next;
  gchar *filepath=NULL;
  gchar *name=NULL;
  char buffer[256];

  GtkWidget *dialog;
  GtkWidget *label;

  if (!current_selection) {
    return;
  }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(player_song_list));

  if (gtk_tree_model_get_iter(model, &iter, current_selection)) {
    gtk_tree_model_get(model, &iter, PLAYER_LIST_SONG_PATH, &filepath, -1);
    gtk_tree_model_get(model, &iter, PLAYER_LIST_SONG_NAME, &name, -1);
  }

  if (!filepath || !name) {
    return;
  }

  snprintf(buffer, 256, "Delete '%s'?", name);

  label = gtk_label_new(buffer);

#ifdef HILDON
    dialog = hildon_note_new_confirmation(GTK_WINDOW(app_window),
					  buffer);
#else
    dialog = gtk_dialog_new_with_buttons("Delete file",
					 GTK_WINDOW(app_window),
					 (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					 GTK_STOCK_OK,
					 GTK_RESPONSE_ACCEPT,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_REJECT,
					 NULL);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
#endif
    gtk_widget_show_all(dialog);

    gint result = gtk_dialog_run(GTK_DIALOG (dialog));
    gtk_widget_destroy(dialog);
    switch (result)
    {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:

      // if there is previous slot
      if (gtk_tree_path_prev(current_selection))
	gtk_tree_view_set_cursor(GTK_TREE_VIEW (player_song_list),
				 current_selection, NULL, FALSE);
      else {
	gtk_tree_path_next(current_selection);
	if (gtk_tree_model_get_iter(model, &next, current_selection))
	  gtk_tree_view_set_cursor(GTK_TREE_VIEW (player_song_list),
				   current_selection, NULL, FALSE);
      }

      gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

      unlink(filepath);

#ifdef HILDON
      hildon_banner_show_information(app_window, NULL, "File deleted");
#else
      update_status_text("FILE WAS DELETED", C64_FG);
#endif
    default :
      break;
    }
}

// delete button callback
static void
delete_song (GtkWidget *widget, gpointer data)
{
  delete_current_selection();
}

// keyboard callback
static gboolean
key_on_selection(GtkWidget   *widget,
		 GdkEventKey *event,
		 gpointer     user_data)
{
  switch (event->keyval)
  {
  case GDK_Delete:
  case GDK_BackSpace:
    delete_current_selection();
    break;

  default:
    break;
  }
  return FALSE;
}


/*
 * This function may seem horrible but I am trying to
 * get rid of dependency to gtktreeview selection since
 * hildon pannablearea does not support selection and I
 * hope to use pannablearea soon for lists.
 */
static void
select_song(GtkTreeView *treeview, gpointer data)
{
  GtkTreePath *path = (GtkTreePath*) (data);

  if (path == NULL) {
    // get cursor selection from treeview if possible
    GtkTreeSelection *selection;
    GtkTreeModel     *model;
    GtkTreeIter       iter;
    GtkTreePath *spath;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(player_song_list));
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
      spath = gtk_tree_model_get_path(model, &iter);
      if(!spath) {
	printf("no path :( \n");
      }
      current_selection = gtk_tree_path_copy(spath);
      gtk_tree_path_free(spath);
      goto have_selection;
    }
  }

  if (current_selection) {
    gtk_tree_path_free(current_selection);
  }
  current_selection = gtk_tree_path_copy(path);

  have_selection :

  /* since cursor changed - we can release image_button for the user again */
  gtk_widget_set_sensitive (image_button, TRUE);

  /* user cannot control animation anymore - song has changed! */
  gtk_widget_set_sensitive (anim_control_button, FALSE);

  // darken_current_animation_image();
}


// notebook page was changed, stop screenshot animation
static void
page_changed(GtkNotebook     *notebook,
	     GtkNotebookPage *new_page,
	     guint            page_num,
	     gpointer         user_data)
{
  if (page_num != image_page_num) {
    stop_screenshot_animation();
  }
}

void
program_is_destroyed(gpointer data, GObject *program)
{
  gtk_main_quit();
}


#ifdef PANEL_SUPPORT
static void
show_from_panel(GtkStatusIcon *status_icon,
		gpointer       user_data)
{
  /* no - do not let user remove icon if it's still blinking,  
   see timeout function below for why not */
  if (gtk_status_icon_get_blinking (status_icon))
    return;
  gtk_widget_show (app_window);
  g_object_unref (status_icon);
}

static gboolean
stop_blinking_timeout(gpointer icon)
{
  gdk_threads_enter();
  /* icon _has_ to still point to valid memory location ... */
  gtk_status_icon_set_blinking (GTK_STATUS_ICON(icon), FALSE);
  gdk_threads_leave();
  return FALSE;
}

static void
hide_to_panel(GtkMenuItem *menuitem, gpointer data)
{
  GtkStatusIcon *icon;
  char buffer[256];

  snprintf(buffer, 256, "%s/msid.png", ICON_FILE_PATH);

  icon = gtk_status_icon_new_from_file(buffer);

  g_signal_connect(G_OBJECT(icon), "activate",
		   G_CALLBACK(show_from_panel), NULL);

  gtk_status_icon_set_tooltip(icon, PROGRAM_NAME);
  gtk_status_icon_set_visible(icon, TRUE);
  gtk_status_icon_set_blinking(icon, TRUE);

  g_timeout_add(1500, stop_blinking_timeout, icon);

  gtk_widget_hide(app_window);
}
#endif

static inline void
free_args(gpointer data, gpointer user_data)
{
  g_free(data);
}

// FIXME - implement
static void
launch_browser(GtkLinkButton *button,
	       const gchar *link_,
	       gpointer user_data)
{
}


static GtkWidget *
append_sponsor_links(GtkWidget *dialog)
{
  GtkWidget *c64org = gtk_link_button_new_with_label("http://www.c64.org", "c64.org");
  GtkWidget *c64com = gtk_link_button_new_with_label("http://www.c64.com", "c64.com");

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), GTK_WIDGET(c64org));
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), GTK_WIDGET(c64com));

  return dialog;
}


static void
show_info_dialog(const char *title, const char *txt1, const char *txt2)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(app_window),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_MESSAGE_INFO,
					      GTK_BUTTONS_CLOSE,
					      NULL);

  gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), txt1);

  gtk_window_set_title(GTK_WINDOW(dialog), title);

  //  gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog), txt2);

#ifndef HILDON
  gtk_window_set_position (GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
#endif

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}


static void calculate_size(GtkRange *range, gpointer data)
{
  char buffer[256];
  int secs = gtk_range_get_value(range);
  guint channels = 1;

  GtkLabel *label = GTK_LABEL(data);

  // stereo output?
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(output_mode_toggle)) == TRUE) {
    channels++;
  }

  msid.setStereo(channels);

  snprintf(buffer, 256, "~%2.1fMB", (44.1 * channels * (16/8) * secs) / 1024);

  msid.setLength(secs);

  gtk_label_set_text(label, buffer);
}


static void
create_ringtone(GtkMenuItem *menuitem, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter  iter;

  gchar *filepath=NULL;
  gchar *name=NULL;
  char buffer[256];

#ifdef HILDON
  char conf_path[256];
  FILE *conf_file;
#endif

  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *sizelabel;
  GtkWidget *scale = gtk_hscale_new_with_range(1.0, 120.0, 1.0);

  char audioplugin[256];

  gtk_range_set_value(GTK_RANGE(scale), msid.length());

  // cannot playback while dumping
  if (msid.isPlaying()) {
    msid.stop();

    gtk_widget_set_sensitive(player_play_button, FALSE);
    gtk_widget_set_sensitive(player_pause_button, FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(player_pause_button), FALSE);

  }

  if (!current_selection) {
    show_info_dialog("No song selected", "You need to first select a song from the list.", "");
    return;
  }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(player_song_list));

  if (gtk_tree_model_get_iter(model, &iter, current_selection)) {
    gtk_tree_model_get(model, &iter, PLAYER_LIST_SONG_PATH, &filepath, -1);
    gtk_tree_model_get(model, &iter, PLAYER_LIST_SONG_NAME, &name, -1);
  }

  if (!filepath || !name) {
    return;
  }


  msid.setSong(filepath, SUBSONG);

  snprintf(buffer, 256, "Msid will create\n%s.wav\nLength in secs :", filepath);
  label = gtk_label_new("");

  gtk_label_set_markup (GTK_LABEL(label), buffer);
  gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);
  
  sizelabel = gtk_label_new("");

  calculate_size(GTK_RANGE(scale), sizelabel);
  
  g_signal_connect(G_OBJECT (scale), "value-changed", G_CALLBACK (calculate_size), sizelabel);

  dialog = gtk_dialog_new_with_buttons("Create ringtone",
				       GTK_WINDOW(app_window),
				       (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
				       GTK_STOCK_OK,
				       GTK_RESPONSE_ACCEPT,
				       GTK_STOCK_CANCEL,
				       GTK_RESPONSE_REJECT,
				       NULL);

  gtk_container_add(GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
  gtk_container_add(GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), scale);
  gtk_container_add(GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), sizelabel);

  gtk_widget_show_all(dialog);

  gint result = gtk_dialog_run(GTK_DIALOG (dialog));

  switch (result) {

  case GTK_RESPONSE_ACCEPT:
  case GTK_RESPONSE_OK:

    snprintf(audioplugin, 256, "%s", msid.audioPluginName());
    msid.setAudioPlugin(DEFAULT_DUMP_PLUGIN);

    msid.setLoop(0);

      // FIXME - BLOCK HERE UNTIL THREAD READY
      {
	unsigned int ready = 0;
	GtkWidget *progress = gtk_progress_bar_new();
	GtkWidget *pdialog = gtk_dialog_new_with_buttons("Creating tone, please wait!",
							 GTK_WINDOW(app_window),
							 (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
							 NULL);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.0);
	gtk_container_add(GTK_CONTAINER (GTK_DIALOG(pdialog)->vbox), progress);
	
	gtk_widget_show_all(pdialog);

	play_stop_song(NULL, data);
  
	while (!ready) {

	  usleep(20000);
	  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progress));

	  while (gtk_events_pending()) {
	    gtk_main_iteration();
	  }

	  if (!msid.isPlaying()) {
	    ready = 1;
	  }
	  
	}

	gtk_widget_hide(pdialog);
	gtk_widget_destroy(pdialog);

      }

      msid.setLoop(1);

#ifdef HILDON
      // set as current user ringtone
      snprintf(conf_path, 256, "%s/.user-ringtone", getenv("HOME"));
      conf_file = fopen(conf_path, "w");
      if (conf_file) {
	fprintf(conf_file, "%s.wav", filepath);
	fclose(conf_file);
      }
#endif

      update_status_text("RINGTONE CREATED", C64_FG);

      msid.setAudioPlugin(audioplugin);

      break;
    default :
      break;
    }


  gtk_widget_destroy(dialog);

}


void
show_about_dialog (GtkMenuItem *menuitem, gpointer data)
{
  GtkWidget *info;
  GtkWidget *hbox;
  GtkWidget *logo;

  char buffer[256];

  char txt[] = "MSID uses libsidplay1, a SID chip emulator by Michael Schwendt. MSID UI was programmed by lemody@gmail.com\n\nMany thanks and respect to the admins of c64.org and c64.com for letting MSID use their resources.";

  GtkWidget *dialog = gtk_dialog_new_with_buttons("About", GTK_WINDOW(app_window),
						  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), NULL);

  g_signal_connect_swapped(dialog, "response",
			   G_CALLBACK (gtk_widget_destroy),
			   dialog);

  hbox = gtk_hbox_new (FALSE, 8);

  snprintf(buffer, 256, "%s/msidlogo.png", CONFIG_FILE_PATH);

  logo = gtk_image_new_from_file(buffer);

  info = gtk_label_new("");
  gtk_label_set_markup(GTK_LABEL(info), txt);
  gtk_label_set_line_wrap(GTK_LABEL(info), TRUE);

  gtk_box_pack_start(GTK_BOX(hbox), logo, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), info, TRUE, TRUE, 0);

  gtk_container_add(GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), hbox);
  
  append_sponsor_links(dialog);

  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
}

void
show_help_dialog (GtkMenuItem *menuitem, gpointer data)
{
  GtkWidget *info;
  GtkWidget *hbox;

  char txt[] = "Use <span foreground=\"#aaaaff\">Search</span> tab to download sid files, when tapped they will appear on <span foreground=\"#aaaaff\">Player</span> tab. When a song is selected in player tab, use <span foreground=\"#aaaaff\">Screenshot</span> tab to display screenshots related to the song, works with most game sid tunes.\n\n<span foreground=\"#aaaaff\">Create ringtone</span> feature creates wav file to same directory as sidtune was found from.\n\nEnjoy!\n";

  GtkWidget *dialog = gtk_dialog_new_with_buttons("Help!", GTK_WINDOW(app_window),
						  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), NULL);

  g_signal_connect_swapped (dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);

  hbox = gtk_hbox_new(FALSE, 8);

  info = gtk_label_new("");
  gtk_label_set_markup(GTK_LABEL(info), txt);
  gtk_label_set_line_wrap(GTK_LABEL(info), TRUE);
  gtk_box_pack_start(GTK_BOX(hbox), info, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), hbox);

  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
}

void
show_config_dialog(GtkMenuItem *menuitem, gpointer data)
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons("Preferences", GTK_WINDOW(app_window),
						  (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), NULL);
#ifndef HILDON
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
#endif

  gtk_container_add(GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), GTK_WIDGET(data));
  gtk_widget_show_all(dialog);

  config_dialog = dialog;

  gtk_dialog_run(GTK_DIALOG(dialog));

  config_dialog = NULL;

  /* unpack config page */
  gtk_container_remove(GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), GTK_WIDGET(data));
  gtk_widget_destroy(dialog);
}


/*
 * very experimental code
 */
static void
colorize_widget(const char *widget)
{
  char rc_buffer[512];

  char BG_COLOR[]   = "\"#111111\"";
  char FG_COLOR[]   = "\"#999999\"";
  char SC_COLOR[]   = "\"#ffffff\"";
  char SCBG_COLOR[] = "\"#444444\"";
  char EVEN_COLOR[] = "\"#111111\"";
  char ODD_COLOR[]  = "\"#222222\"";

  /*
style "window"
  {
    engine "sapwood" {
      image {
       function = BOX
       file        = "../common/images/bg.xmp"
	 }
    }
  }
class "GtkWindow" style "window"
  */

  if (!strcmp (widget, "GtkTreeView"))
    {
#ifdef HILDON
      snprintf (rc_buffer, 512,
		"style \"%s_msid_colors\"\n{engine \"sapwood\"{\n\nbase[INSENSITIVE] = %s\nbase[NORMAL] = %s\nbase[ACTIVE] = %s\nbase[PRELIGHT] = %s\nbase[SELECTED] = %s\nGtkTreeView::even-row-color = %s\nGtkTreeView::odd-row-color  = %s\ntext[NORMAL]   = %s\ntext[ACTIVE]   = %s\ntext[SELECTED] = %s\ntext[INSENSITIVE] = %s\n}\n} class \"%s\" style \"%s_msid_colors\" ",
		widget,
		BG_COLOR, BG_COLOR, SCBG_COLOR, BG_COLOR, SCBG_COLOR,
		EVEN_COLOR, ODD_COLOR,
		FG_COLOR, SC_COLOR, SC_COLOR, BG_COLOR,
		widget, widget);
#else
      snprintf (rc_buffer, 512,
		"style \"%s_msid_colors\"\n{\n\nbase[INSENSITIVE] = %s\nbase[NORMAL] = %s\nbase[ACTIVE] = %s\nbase[PRELIGHT] = %s\nbase[SELECTED] = %s\nGtkTreeView::even-row-color = %s\nGtkTreeView::odd-row-color  = %s\ntext[NORMAL]   = %s\ntext[ACTIVE]   = %s\ntext[SELECTED] = %s\ntext[INSENSITIVE] = %s\n} class \"%s\" style \"%s_msid_colors\" ",
		widget,
		BG_COLOR, BG_COLOR, SCBG_COLOR, BG_COLOR, SCBG_COLOR,
		EVEN_COLOR, ODD_COLOR,
		FG_COLOR, SC_COLOR, SC_COLOR, BG_COLOR,
		widget, widget);
#endif
    }
  else if (!strcmp (widget, "GtkButton"))
    {
      /*
      snprintf (rc_buffer, 512,
		"style \"%s_msid_colors\"\n{\ntext[NORMAL]= %s\ntext[ACTIVE]   = %s\ntext[SELECTED] = %s\n} class \"%s\" style \"%s_msid_colors\" ",
		widget,
		SC_COLOR, SC_COLOR, SC_COLOR,
		widget, widget);
      */
    }

  gtk_rc_parse_string (rc_buffer);
}

// change between mono/stereo output
static void output_mode_changed(GtkToggleButton *togglebutton,
				gpointer user_data)
{
  // restart of the playback needed

  update_status_text("OUTPUT MODE CHANGED", C64_FG);

  replay();
}



int main(int argc, char **argv)
{
  GtkWidget

    *mother_container,
    *main_box,

    *menu_bar,
    *menu,

    *main_menu,
    *item1,
    *item2,
    *item3,
    *item4,
    *item5,

    *player_main_box,
    *status_background,

    *player_label,
    *loader_label,
    *image_label,

    *player_page,
    *loader_page,
    *config_page,
    *image_page;

  char buffer[256];

#ifdef HILDON
  HildonProgram *program;
#endif

  g_thread_init(NULL);
  gdk_threads_init();
  gdk_threads_enter();

  gtk_init(&argc, &argv);

  gtk_link_button_set_uri_hook(launch_browser, NULL, NULL);

  /* hildon theming overrides colorization ... buhuu. */
#ifndef HILDON
  colorize_widget("GtkTreeView");
  colorize_widget("GtkButton");
#endif

  msid_active_search_plugin = (msid_search_plugin *) msid.searchPlugin()->plugin_object;
  msid_active_search_plugin->init();


#ifdef HILDON
  program = hildon_program_get_instance();
  app_window = hildon_stackable_window_new();
  hildon_program_add_window (program, HILDON_WINDOW (app_window));
#else
  app_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_position (GTK_WINDOW(app_window), GTK_WIN_POS_CENTER);
#endif

  gtk_widget_set_size_request (app_window, -1, APP_WINDOW_HEIGHT);

  //  g_set_application_name (PROGRAM_NAME);

  snprintf (download_path, 256, "0");

#ifdef HILDON
  if (!create_sid_path("/home/user/MyDocs/sidmusic")) {
    if (!create_sid_path("/media/mmc2/sidmusic")) {
      fprintf(stderr, "error - problems with sid-path!\n");
    }
  }
#else
  snprintf(download_path, 256, "%s/sidmusic", getenv("HOME"));

  if (!create_sid_path (download_path)) {
    fprintf (stderr, "error - problems with sid-path!\n");
  }
#endif

  // create containers
  main_box = gtk_vbox_new (FALSE, 0);

  player_main_box = gtk_vbox_new (FALSE, 0);

  // create widgets
  player_label = gtk_label_new("Player");
  loader_label = gtk_label_new("Search");
  image_label  = gtk_label_new("Screenshot");

  status_background = gtk_event_box_new();
  status_text       = gtk_label_new("");

  // pack widgets
  gtk_container_add(GTK_CONTAINER(status_background), status_text);

  // pack containers
  loader_page = create_loader_page(msid_active_search_plugin, update_status_text);

  /*
   * these are just container boxes, so can be packed in anything.
   * makes it possible to create different UI layouts more easily ...
   */

  player_page = create_player_page(update_status_text);
  image_page  = create_image_page(update_status_text);
  config_page = create_config_page();

  /* leave reference to page so that it won't get destroyed
   * when unpacked from dialog 
   */
  g_object_ref (config_page);

#define NOTEBOOK
#ifdef NOTEBOOK
  mother_container = gtk_notebook_new();

  gtk_notebook_append_page(GTK_NOTEBOOK(mother_container),
			   player_page, player_label);
  gtk_notebook_append_page(GTK_NOTEBOOK(mother_container),
			   loader_page, loader_label);
  image_page_num = gtk_notebook_append_page(GTK_NOTEBOOK(mother_container),
					    image_page, image_label);

  g_signal_connect(G_OBJECT (mother_container), "switch-page",
		   G_CALLBACK (page_changed), image_page);
#endif


  main_menu = gtk_menu_item_new_with_label("Menu");

  item1 = gtk_menu_item_new_with_label("Help");
  item2 = gtk_menu_item_new_with_label("About");
  item3 = gtk_menu_item_new_with_label("Preferences");
  item4 = gtk_menu_item_new_with_label("Create Ringtone");

#ifndef __arm__
  item5 = gtk_menu_item_new_with_label("Quit");
#endif


  g_signal_connect(G_OBJECT (item1), "activate",
		   G_CALLBACK (show_help_dialog), NULL);
  g_signal_connect(G_OBJECT (item2), "activate",
		   G_CALLBACK (show_about_dialog), NULL);
  g_signal_connect(G_OBJECT (item3), "activate",
		   G_CALLBACK (show_config_dialog), config_page);
  g_signal_connect(G_OBJECT (item4), "activate",
		   G_CALLBACK (create_ringtone), NULL);

#ifndef __arm__
  g_signal_connect(G_OBJECT (item5), "activate",
		   G_CALLBACK (quit_msid), NULL);
#endif

#ifdef HILDON
  // use hildon menu stuff
  menu_bar = gtk_menu_new();
  gtk_menu_append(GTK_MENU (menu_bar), item1);
  gtk_menu_append(GTK_MENU (menu_bar), item2);
  gtk_menu_append(GTK_MENU (menu_bar), item3);
  gtk_menu_append(GTK_MENU (menu_bar), item4);

  hildon_program_set_common_menu(program, GTK_MENU (menu_bar));
#else
  // use gtkmenubar
  menu_bar = gtk_menu_bar_new();
  menu = gtk_menu_new();

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item1);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item2);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item3);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item4);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item5);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(main_menu), menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), main_menu);

  gtk_box_pack_start(GTK_BOX(main_box), menu_bar, FALSE, FALSE, 0);
#endif

  gtk_box_pack_start(GTK_BOX(main_box), mother_container, TRUE, TRUE, 0);
  append_to_vbox(main_box, status_background, FALSE, FALSE);

  gtk_container_add(GTK_CONTAINER(app_window), main_box);


  // signal-handlers for extern objects

  g_signal_connect(G_OBJECT (player_song_list), "key-release-event",
		   G_CALLBACK (key_on_selection), NULL);

  g_signal_connect(G_OBJECT (player_song_list), "row-activated",
		   G_CALLBACK (handle_selection), NULL);

  g_signal_connect(G_OBJECT (player_song_list), "cursor-changed",
		   G_CALLBACK (select_song), NULL);

  g_signal_connect(G_OBJECT (player_play_button), "clicked",
		   G_CALLBACK (play_stop_song), NULL);
  g_signal_connect(G_OBJECT (player_pause_button), "toggled",
		   G_CALLBACK (pause_song), NULL);
  g_signal_connect(G_OBJECT (player_del_button), "clicked",
		   G_CALLBACK (delete_song), NULL);

  g_signal_connect(G_OBJECT (subsong_selector), "changed",
		   G_CALLBACK (subsong_changed), NULL);

  g_signal_connect(G_OBJECT (config_audio_plugin_button), "clicked",
		   G_CALLBACK (load_audio_plugin), config_audio_plugin_combo);
  g_signal_connect(G_OBJECT (config_search_plugin_button), "clicked",
		   G_CALLBACK (load_search_plugin), config_search_plugin_combo);
  g_signal_connect(G_OBJECT (hvsc_path_selection_button), "clicked",
		   G_CALLBACK (select_hvsc_path), NULL);

  g_signal_connect(G_OBJECT(output_mode_toggle), "toggled",
		   G_CALLBACK (output_mode_changed), NULL);

  // FIXME - !!!
  g_signal_connect(G_OBJECT (image_button), "clicked",
		   G_CALLBACK (load_screenshot), NULL);

  g_signal_connect(G_OBJECT (app_window), "key-release-event",
		   G_CALLBACK (msid_on_key_release_event), NULL);

  gtk_widget_show_all(app_window);

#ifdef HILDON
  g_object_weak_ref(G_OBJECT (program),
		    (GWeakNotify) program_is_destroyed,
		    NULL);
  g_object_unref (program);
#else
  g_signal_connect(G_OBJECT (app_window), "delete_event",
		   G_CALLBACK (gtk_main_quit), NULL);
#endif

  /**************************************************/
  GdkColor bgcolor;
  gdk_color_parse(C64_BG, &bgcolor);

  gtk_widget_modify_bg(status_background, GTK_STATE_NORMAL, &bgcolor);
  /**************************************************/

  refresh_player_songlist(player_read_button, NULL);

  update_status_text("READY.", C64_FG);



  gtk_main();
  gdk_threads_leave();

  
  snprintf (buffer, 256, "%s/.msid", getenv("HOME"));

  // TODO - save current configuration

  /*
   * have to implement API to get plugin names from plugin objects
   */

  return 0;
}
