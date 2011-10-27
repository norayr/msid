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

#include "player_tab.h"

GtkWidget *player_song_list;
GtkWidget *player_play_button;
GtkWidget *player_pause_button;
GtkWidget *player_read_button;
GtkWidget *player_del_button;
GtkWidget *subsong_selector;

static void (*set_status_text) (const char*, const char*) = NULL;

static gint name_id = PLAYER_LIST_SONG_NAME;
static gint author_id = PLAYER_LIST_SONG_AUTHOR;

static GtkTreeModel*
create_and_fill_model (void)
{
  return GTK_TREE_MODEL
    (gtk_list_store_new (PLAYER_LIST_NUM_COLS,
                         G_TYPE_STRING,
                         G_TYPE_STRING,
			 G_TYPE_INT,
			 G_TYPE_STRING));
}

static void
sort (GtkTreeViewColumn *treeviewcolumn,
      gpointer           data)
{
  GtkTreeModel *sort_model;
  gint *id = (gint *) data;

  sort_model = gtk_tree_view_get_model(GTK_TREE_VIEW(player_song_list));
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE (sort_model),
				       id ? *id : 0, GTK_SORT_ASCENDING);
}

static GtkWidget*
create_view_and_model (void)
{
  GtkCellRenderer     *renderer;
  GtkTreeModel        *model;
  GtkWidget           *view;

  view = gtk_tree_view_new ();

  renderer = gtk_cell_renderer_text_new ();

  // g_object_set (G_OBJECT (renderer), "cell-background", "#FF8800", NULL);

  /*
  GValue *value = (GValue *) malloc (sizeof(GValue));
  g_value_init (value, G_TYPE_OBJECT);
  gtk_widget_style_get_property (view, "even-row-color", value);
  GdkColor *color = (GdkColor*) g_value_get_object (value);
  printf ("%d\n", color->red);
  g_free (value);
  */

#ifdef HANDHELD_UI
  g_object_set (G_OBJECT (renderer), "height", 42, NULL);
#endif

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                               -1, "Name", renderer,
                                               "text", PLAYER_LIST_SONG_NAME, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                               -1, "Author", renderer,
                                               "text", PLAYER_LIST_SONG_AUTHOR, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                               -1, "Amount", renderer,
                                               "text", PLAYER_LIST_SONG_AMOUNT, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
                                               -1, "Path", renderer,
                                               "text", PLAYER_LIST_SONG_PATH, NULL);

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), TRUE);
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view), TRUE);
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));

  gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(view),
							    PLAYER_LIST_SONG_PATH),
                                   FALSE);
  gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(view),
							    PLAYER_LIST_SONG_AMOUNT),
                                   FALSE);
  gtk_tree_view_column_set_clickable (gtk_tree_view_get_column(GTK_TREE_VIEW(view),
							       PLAYER_LIST_SONG_NAME),
                                      TRUE);
  gtk_tree_view_column_set_clickable (gtk_tree_view_get_column(GTK_TREE_VIEW(view),
							       PLAYER_LIST_SONG_AUTHOR),
                                      TRUE);

  g_signal_connect (G_OBJECT (gtk_tree_view_get_column(GTK_TREE_VIEW(view),PLAYER_LIST_SONG_NAME)),
                    "clicked", G_CALLBACK (sort), (gpointer) &name_id);
  g_signal_connect (G_OBJECT (gtk_tree_view_get_column(GTK_TREE_VIEW(view),PLAYER_LIST_SONG_AUTHOR)),
                    "clicked", G_CALLBACK (sort), (gpointer) &author_id);
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)),
                              GTK_SELECTION_SINGLE);
  
  model = create_and_fill_model ();

  gtk_tree_view_set_model (GTK_TREE_VIEW (view), model);
  g_object_unref (model); /* destroy model automatically with view */

  return view;
}


static gboolean
song_in_the_list (const gchar *fpath)
{
  GtkTreeModel *model=NULL;
  GtkTreeIter iter;
  gboolean value = FALSE;
  gchar *path=NULL;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(player_song_list));

  /* if empty check + set iter */
  if(!(gtk_tree_model_get_iter_first(model, &iter))) {
    return FALSE;
  }

  /* go through tree and check if song already in list */
  do {
    gtk_tree_model_get (model, &iter, PLAYER_LIST_SONG_PATH, &path, -1);

    if (path) {
      if (g_ascii_strcasecmp(fpath, path) == 0) {
	g_free(path);
	return TRUE;
      }
      g_free(path);
    }
  } while(gtk_tree_model_iter_next(model, &iter) != FALSE);

  return value;
}


gboolean
add_song_to_playlist (const gchar *path)
{
  struct sidTuneInfo info;
  GtkTreeModel *model;
  GtkTreeIter iter;

  gchar name_buf[256];
  gchar auth_buf[256];

  memset(&info, 0, sizeof(sidTuneInfo));

  if (!sidtune_is_ok (path)) {
    return FALSE;
  }

  sidTune tune (path);

  if (tune.getInfo(info) != true) {
    return FALSE;
  }

  if (!g_utf8_validate(info.authorString,
		       -1, NULL)) {
    // better this than nothing||crashes
    if (strlen (info.authorString) > strlen ("unknown"))
      sprintf (info.authorString, "unknown");
    else {
      return FALSE;
    }
  }

  if (g_ascii_strcasecmp(info.nameString, "<?>") == 0)
    sprintf(name_buf, "unknown");
  else
    sprintf(name_buf, "%s", info.nameString);

  if (g_ascii_strcasecmp(info.authorString, "<?>") == 0)
    sprintf(auth_buf, "unknown");
  else
    sprintf(auth_buf, "%s", info.authorString);

  if (info.songs <= 0)
  {
    return FALSE;
  }

  if (!song_in_the_list (path))
  {
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(player_song_list));

    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			PLAYER_LIST_SONG_NAME,   name_buf,
			PLAYER_LIST_SONG_AUTHOR, auth_buf,
			PLAYER_LIST_SONG_AMOUNT, info.songs,
			PLAYER_LIST_SONG_PATH, path,
			-1);
    return TRUE;
  }

  return FALSE;
}

void *read_filenames_thread (void *arg)
{
  GDir *dir;
  gchar buffer[256];
  GError *error =NULL;
  const gchar *fname =NULL;
  char msid_path[256];
  int amount = 0;

  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;

  snprintf (msid_path, 256, "%s/sidmusic", getenv("HOME"));

  static const char *path_list[] =
    {
      "/home/user/MyDocs/sidmusic",
      "/media/mmc2/sidmusic",
      msid_path,
      NULL
    };
  const char **p = path_list;

  gdk_threads_enter();
  gtk_widget_set_sensitive(GTK_WIDGET(arg), FALSE);
  gdk_flush();
  gdk_threads_leave();

  while (*p)
    {
      error =NULL;
      dir = g_dir_open (*p, 0, &error);

      if (dir)
      {
	fname = g_dir_read_name (dir);

	while (fname != NULL)
	{
	  sprintf (buffer, "%s/%s", *p, fname);

	  gdk_threads_enter();

	  if (add_song_to_playlist (buffer))
	  {
	    amount++;
	  }

	  gdk_flush();
	  gdk_threads_leave();

	  fname = g_dir_read_name(dir);
	}

	g_dir_close(dir);
      }
      p++;
    }

  gdk_threads_enter();
  gtk_widget_set_sensitive(GTK_WIDGET(arg), TRUE);

  // SORT

  int name_index = 0;

  sort (NULL, (gpointer) &name_index);

  // SELECT FIRST ITEM /////////////////////////////
  model = gtk_tree_view_get_model (GTK_TREE_VIEW(player_song_list));

  if (gtk_tree_model_get_iter_first (model, &iter))
  {
    path = gtk_tree_model_get_path (model, &iter);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW(player_song_list),
			      path, NULL, FALSE);
    gtk_tree_path_free (path);
  }
  /////////////////////////////////////////////////

  gdk_flush();
  gdk_threads_leave();

  return NULL;
}

void
refresh_player_songlist(GtkWidget *widget,
			gpointer data)
{
  GError *error=NULL;

  set_status_text ("LOADING...", C64_FG);

  g_thread_create (read_filenames_thread, (void*) widget, FALSE, &error);
}

GtkWidget *
create_subsong_selector(unsigned int amount)
{
  GtkWidget *x = NULL;
  unsigned int k;
  char str[8];

  x = gtk_combo_box_new_text();

  // title - should be taken in to account when modifying
  gtk_combo_box_append_text (GTK_COMBO_BOX(x), "  SONG");

  for (k=1; k<1+amount; k++)
  {
    sprintf(str, "%d", k);
    gtk_combo_box_append_text(GTK_COMBO_BOX(x), str);
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(x), 0);
#ifdef HANDHELD_UI
  gtk_widget_set_size_request(x, 64, 80);
#endif
  return x;
}


GtkWidget*
create_player_page (void (*status_text_func) (const char*, const char*))
{
  GtkWidget

    *player_main_box,
    *scroll_win,
    *ctrl_box,
    *ctrl_main_box,
    *subsong_box;

  set_status_text = status_text_func;

  /* --- widgets --- */

  player_main_box = gtk_vbox_new (FALSE, 0);

  ctrl_box      = gtk_hbox_new (TRUE, 0);
  ctrl_main_box = gtk_hbox_new (FALSE, 0);
  subsong_box = gtk_vbox_new (FALSE, 0);

  player_play_button = create_ui_button ("STOP");
  player_pause_button = create_ui_toggle_button ("PAUSE");
  player_read_button = create_ui_button ("REFRESH");
  player_del_button  = create_ui_button ("DEL");

  gtk_widget_set_sensitive (player_play_button, FALSE);
  gtk_widget_set_sensitive (player_pause_button, FALSE);

  subsong_selector = create_subsong_selector (1);

  player_song_list = create_view_and_model();

#ifdef MAEMO5
  scroll_win = hildon_pannable_area_new();
#else
  scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scroll_win),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
#endif

  /* --- signals --- */

  g_signal_connect (G_OBJECT (player_read_button), "clicked",
                    G_CALLBACK (refresh_player_songlist), NULL);

  /* --- packing --- */

  gtk_box_pack_start (GTK_BOX(ctrl_box), player_play_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(ctrl_box), player_pause_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(ctrl_box), player_read_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(ctrl_box), player_del_button, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(ctrl_box), subsong_selector, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (scroll_win), player_song_list);

  gtk_box_pack_start (GTK_BOX(ctrl_main_box), ctrl_box, TRUE, TRUE, 0);
  //  gtk_box_pack_start (GTK_BOX(ctrl_main_box), subsong_box, FALSE, FALSE, 2);

  gtk_box_pack_start (GTK_BOX(player_main_box), scroll_win, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(player_main_box), ctrl_main_box, FALSE, FALSE, 0);

  return player_main_box;
}
