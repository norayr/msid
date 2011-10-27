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

#include "loader_tab.h"

typedef struct _search_widgets
{
  GtkWidget *entry;
  GtkWidget *song_list;
} search_widgets;

static search_widgets s_widgets;

static GtkWidget *progress_bar;

G_LOCK_DEFINE_STATIC(search_active);
static volatile gboolean search_active = FALSE;
G_LOCK_DEFINE_STATIC(download_active);
static volatile gboolean download_active = FALSE;

extern gboolean
add_song_to_playlist (const gchar *path);

void (*set_status_text) (const char*, const char*) = NULL;

typedef struct search_args
{
  GtkWidget *list;
  char *needle;
} loader_search_args;

typedef struct download_args
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  char *uri;
  char *file_path;
} loader_download_args;

loader_search_args search_args;
loader_download_args dload_args;

static GtkTreeModel*
create_and_fill_model (void)
{
  return GTK_TREE_MODEL
    (gtk_list_store_new(LOADER_LIST_NUM_COLS,
			G_TYPE_STRING,
			G_TYPE_STRING,
			G_TYPE_STRING));
}

static GtkWidget*
create_view_and_model (void)
{
  GtkCellRenderer     *renderer;
  GtkTreeModel        *model;
  GtkWidget           *view;

  view = gtk_tree_view_new();

  renderer = gtk_cell_renderer_text_new();

#ifdef HANDHELD_UI
  ;//g_object_set (G_OBJECT (renderer), "height", 32, NULL);
#endif

  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW (view),
					      -1, "Song", renderer,
					      "text", LOADER_LIST_SONG_NAME, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW (view),
					      -1, "URI", renderer,
					      "text", LOADER_LIST_SONG_URI, NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW (view),
					      -1, "HVSC PATH", renderer,
					      "text", LOADER_LIST_SONG_SHORT_URI, NULL);

  gtk_tree_view_column_set_visible(gtk_tree_view_get_column
				   (GTK_TREE_VIEW(view), LOADER_LIST_SONG_NAME), TRUE);
  gtk_tree_view_column_set_visible(gtk_tree_view_get_column
				   (GTK_TREE_VIEW(view), LOADER_LIST_SONG_URI), FALSE);

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view), TRUE);
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)),
                              GTK_SELECTION_SINGLE);

  model = create_and_fill_model();

  gtk_tree_view_set_model(GTK_TREE_VIEW (view), model);
  g_object_unref(model); /* destroy model automatically with view */

  return view;
}


static void
free_entry (gpointer entry, gpointer data)
{
  free ((msid_search_entry*) entry);
}


static gboolean
show_progress (gpointer data)
{
  G_LOCK (search_active);
  if (!search_active)
  {
    G_UNLOCK (search_active);

    // reset progress bar
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar), 0.0);

    return FALSE;
  }
  G_UNLOCK (search_active);
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (progress_bar));
  return TRUE;
}


void * download_thread(void *arg)
{
  loader_download_args *args;

  char complete_uri[512];
  char tmp[256];

  unsigned int mirror = 0;
  unsigned int download_ok = 0;

  args = (loader_download_args *) arg;

  //
  // FIXME - this list should be configurable and not hardcoded
  //
  const char *mirrors[] = {
    "http://www.prg.dtu.dk/HVSC/C64Music",
    "http://hvsc.perff.dk",
    "http://www.tld-crew.de/c64music",
    "http://hvsc.pixolut.com/C64Music",
    NULL
  };

  while (mirrors[mirror] && !download_ok) {

    // current mirror
    snprintf(complete_uri, 512, "%s%s", mirrors[mirror], args->uri);

    DEBUG("trying out HVSC mirror [%s]\n", mirrors[mirror]);
    DEBUG("download [%s] -> [%s]\n", complete_uri, args->file_path);

    snprintf(tmp, 256, "LOADING FROM MIRROR %d", mirror+1);

    gdk_threads_enter();
    set_status_text(tmp, NULL);
    gdk_threads_leave();

    fetch_data_to_file(complete_uri, args->file_path);

    // if file does not exist, try another mirror?
    if (g_file_test(args->file_path, G_FILE_TEST_EXISTS)) {
      download_ok = 1;
    }

    DEBUG("download status [%s] (mirror %d)", download_ok ? "ok" : "not ok", mirror);
    mirror++;

  } // while download not ok


  gdk_threads_enter();

  // remove entry anyway, is either downloaded or 'bad'
  gtk_list_store_remove(GTK_LIST_STORE(args->model), &args->iter);


  if (download_ok) {

    if (sidtune_is_ok(args->file_path)) {

      add_song_to_playlist(args->file_path);

      set_status_text("SONG WAS DOWNLOADED", NULL);
    } else {
      // download succeeded but file is not ok for msid
      set_status_text("TUNE NOT SUPPORTED", NULL);
    }
  } else {
    set_status_text("SONG WAS NOT FOUND", NULL);
  }


  gdk_threads_leave();

  g_free (args->uri);
  g_free (args->file_path);

  G_LOCK (search_active);
  search_active = FALSE;
  G_UNLOCK (search_active);

  return NULL;
}


static void
download_and_remove(GtkTreeView       *tree_view,
		    GtkTreePath       *path,
		    GtkTreeViewColumn *column,
		    gpointer           user_data)
{
  GtkTreeModel *model=NULL;
  GtkTreeIter iter;
  gchar *uri, *name;
  gchar file_path[256];
  char msid_path[256];
  GDir *dir;
  GError *error =NULL;
  gboolean success=FALSE;

  snprintf (msid_path, 256, "%s/sidmusic", getenv("HOME"));

  // TODO - do not duplicate this code, it's already in player
  static const char *path_list[] = {
    "/home/user/MyDocs/sidmusic",
    "/media/mmc2/sidmusic",
    msid_path,
    NULL
  };
  const char **p = path_list;

  G_LOCK (search_active);
  if (search_active == TRUE) {
    G_UNLOCK (search_active);
    return;
  }
  search_active = TRUE;
  G_UNLOCK (search_active);

  /*
   * TODO - make directory to mmc-card if does not exist
   */
#ifdef HILDON
#define HILDON_MMC1 "/home/user/MyDocs/sidmusic"
#define HILDON_MMC2 "/media/mmc2/sidmusic"
  if (!g_file_test(HILDON_MMC1, G_FILE_TEST_IS_DIR) &&
      !g_file_test(HILDON_MMC2, G_FILE_TEST_IS_DIR)) {
    set_status_text("INSERT MMC CARD", NULL);
  }
#endif

  model = gtk_tree_view_get_model(tree_view);
  gtk_tree_model_get_iter(model, &iter, path);

  gtk_tree_model_get(model, &iter,
		     LOADER_LIST_SONG_NAME, &name,
		     LOADER_LIST_SONG_URI,  &uri,
		     -1);

  /*
   * TODO - get path from settings ...
   */

  while (*p) {

    error =NULL;
    dir = g_dir_open(*p, 0, &error);

    if (dir) {

      g_dir_close(dir);

      snprintf(file_path, 256, "%s/%s", *p, name);

      /* FIXME - check first internet connection availability */

      dload_args.model = model;
      dload_args.iter = iter;

      dload_args.uri = g_strdup(uri);
      dload_args.file_path = g_strdup(file_path);

      g_timeout_add(250, show_progress, NULL);

      g_thread_create(download_thread, (void*) &dload_args, FALSE, &error);

      break;
    }
    p++;
  }

  if (success) {
#ifdef HANDHELD_UI
    ; // TODO - show infobanner
#endif
  }
}




void *search_thread (void *arg)
{
  GSList *list, *tmp;
  msid_search_entry* entry;

  GtkTreeModel *model=NULL;
  GtkTreeIter iter;

  GtkWidget *song_list;
  loader_search_args *args;
  extern msid_search_plugin *msid_active_search_plugin;
 
  args = (loader_search_args *) arg;

  song_list = args->list;

  //
  // FIXME - plugin has to be protected by a G_LOCK (mutex) ?
  //

  // blocks and can take a lot of time
  list = msid_active_search_plugin->search_for_sid(args->needle, NULL);

  gdk_threads_enter();

  // TODO - add to entry completion only if there is some search results?

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(song_list));
  gtk_tree_model_get_iter_first(model, &iter);

  if (list) {

    for (tmp = list; tmp; tmp = g_slist_next(tmp)) {

      entry = (msid_search_entry*) tmp->data;

      gtk_list_store_append(GTK_LIST_STORE(model), &iter);
      gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			 LOADER_LIST_SONG_NAME, g_strdup(entry->file_name),
			 LOADER_LIST_SONG_URI,  g_strdup(entry->uri),
			 LOADER_LIST_SONG_SHORT_URI, g_strdup(entry->hvsc_path),
			 -1);
    }
    g_slist_foreach(list, (GFunc) free_entry, NULL);
    g_slist_free(list);
  }

  gdk_threads_leave();

  g_free(args->needle);

  G_LOCK(search_active);
  search_active = FALSE;
  G_UNLOCK(search_active);

  return NULL;
}



static void
make_search (GtkWidget *song_list, const char *needle)
{
  GError *error=NULL;
  unsigned int k;

  // FIXME - stop current search!
  G_LOCK (search_active);
  if (search_active) {
    G_UNLOCK (search_active);
    return;
  }
  G_UNLOCK (search_active);

  search_args.list = song_list;
  search_args.needle = g_strdup(needle);

  // replace spaces with '_'
  // (usual format in sid files)
  for (k=0; k<strlen(search_args.needle); k++) {
    if (search_args.needle[k] == ' ') {
      search_args.needle[k] = '_';
    }
  }

  G_LOCK (search_active);
  search_active = TRUE;
  G_UNLOCK (search_active);

  set_status_text ("LOADING...", NULL);

  g_timeout_add (250, show_progress, NULL);
  g_thread_create (search_thread, (void*) &search_args, FALSE, &error);
}

static void
append_entry_completion (gpointer data)
{
  GtkTreeModel *model=NULL;
  GtkTreeIter iter;

  search_widgets *o = (search_widgets *) data;

  model = gtk_entry_completion_get_model(gtk_entry_get_completion((GtkEntry*)o->entry));

  gtk_tree_model_get_iter_first(model, &iter);
  gtk_list_store_append(GTK_LIST_STORE(model), &iter);
  gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		     0,
		     g_strdup
		     (gtk_entry_get_text((GtkEntry*)o->entry)),
		     -1);
}


static void
search_button_clicked (GtkButton *button,
		       gpointer data)
{
  GtkTreeModel *model=NULL;
  GtkTreeIter iter;
  search_widgets *o = (search_widgets *) data;

  append_entry_completion (data);

  const gchar *needle = gtk_entry_get_text((GtkEntry*) o->entry);

  // clear previous results
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(o->song_list));
  while (gtk_tree_model_get_iter_first(model, &iter)) {
    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
  }

  make_search(o->song_list, needle);
}


static void
clear_loader_list (GtkButton *button,
		   gpointer data)
{
  GtkTreeModel *model=NULL;
  GtkTreeIter iter;

  search_widgets *o = (search_widgets *) data;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(o->song_list));
  while (gtk_tree_model_get_iter_first(model, &iter)) {
    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
  }
}


static void
clear_search_entry (GtkButton *button,
		    gpointer data)
{
  gtk_entry_set_text (GTK_ENTRY(data), "");
  gtk_widget_grab_focus (GTK_WIDGET(data));
}


static void
append_to_vbox (GtkWidget *box, GtkWidget *w,
		gboolean a, gboolean b)
{
  GtkWidget *sep = gtk_vseparator_new();
  gtk_box_pack_start(GTK_BOX(box), sep, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), w, a, b, 0);
}

    
static void
run_search (GtkEntry *entry,
	    gpointer data)
{
  G_LOCK (search_active);
  if (search_active)
  {
    G_UNLOCK (search_active);
    return;
  }
  G_UNLOCK (search_active);

  search_button_clicked(NULL, data);
}



GtkWidget *
create_loader_page (msid_search_plugin *plugin,
		    void (*status_text_func) (const char*, const char*))
{
  GtkWidget

    *scroll_win,
    *loader_main_box,
    *find_box,
    *song_list,
    *song_box;

  GtkWidget

    *search_button,
    *search_entry;

  GtkEntryCompletion *entry_completion;

  set_status_text = status_text_func;

  /* create containers */

  loader_main_box = gtk_vbox_new (FALSE, 0);

  find_box = gtk_hbox_new(FALSE, 0);
  song_box = gtk_vbox_new(FALSE, 0);

  /* create widgets */

#ifdef MAEMO5
  scroll_win = hildon_pannable_area_new();
#else
  scroll_win = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
#endif

  search_button = create_ui_button("search");
  search_entry  = gtk_entry_new();

  entry_completion = gtk_entry_completion_new();

  progress_bar = gtk_progress_bar_new();

#ifdef HANDHELD_UI
  ; //  gtk_widget_set_size_request (search_entry, -1, 80);
#endif

  song_list = create_view_and_model();

  s_widgets.entry     = search_entry;
  s_widgets.song_list = song_list;

  gtk_entry_completion_set_inline_completion(entry_completion, TRUE);
  gtk_entry_completion_set_popup_completion(entry_completion, TRUE);
#ifdef HILDON
  gtk_entry_completion_set_popup_completion(entry_completion, TRUE);
#else
  gtk_entry_completion_set_inline_selection(entry_completion, TRUE);
#endif


  gtk_entry_set_completion(GTK_ENTRY(search_entry), entry_completion);

  GtkTreeModel* completion_model =
    GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING));

  gtk_entry_completion_set_model(entry_completion,
				 GTK_TREE_MODEL (completion_model));
  gtk_entry_completion_set_text_column(entry_completion, 0);



  /* connect to signals */

  g_signal_connect(G_OBJECT(search_entry), "activate",
		   G_CALLBACK(run_search), &s_widgets);

  g_signal_connect(G_OBJECT(search_button), "clicked",
		   G_CALLBACK(run_search), &s_widgets);

  g_signal_connect(G_OBJECT(song_list),
		   "row-activated",
		   G_CALLBACK(download_and_remove),
		   NULL);

  /* pack widgets */

  gtk_box_pack_start(GTK_BOX(find_box), search_entry,  TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(find_box), search_button, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER (scroll_win), song_list);

  /* pack containers */

  gtk_box_pack_start(GTK_BOX(loader_main_box), find_box, FALSE, FALSE, 0);

  append_to_vbox(loader_main_box, song_box, FALSE, FALSE);
  append_to_vbox(loader_main_box, scroll_win, TRUE, TRUE);
  append_to_vbox(loader_main_box, progress_bar, FALSE, FALSE);

  return loader_main_box;
}
