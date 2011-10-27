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

#include "search_plugin_super.h"
#include "../curling.c"

static GSList *search_results = NULL;

static gint
comparison_func (gconstpointer a,
		 gconstpointer b,
		 gpointer user_data)
{
  return g_ascii_strcasecmp ((gchar*) a,
                             (gchar*) b);
}

static void
destroy_string (gpointer data)
{
  g_free (data);
}

super_search::~super_search()
{
  if (tree)
    g_tree_destroy (tree);
}


/* -stolen from busybox, GPL ------------------------------------------------- */
/* this func Copyright (C) 2001 Larry Doolittle, <ldoolitt@recycle.lbl.gov>*/
char * last_char_is(const char *s, int c)
{
  char *sret;
  if (!s)
    return NULL;
  sret  = (char *)s+strlen(s)-1;
  if (sret>=s && *sret == c) {
    return sret;
  } else {
    return NULL;
  }
}

void chomp(char *s)
{
  char *lc = last_char_is(s, '\n');

  if(lc)
    *lc = 0;
}
/* -stolen from busybox, GPL ------------------------------------------------- */


static void
read_file_to_tree (GTree *tree, const gchar *file)
{
  FILE *in;
  char buffer[256];
  char *end;

  in = fopen (file, "r");
  if (in)
    {
    do
      {
	fgets (buffer, 256, in);

	/*
	 * 'key' is lowercased 'value', used in comparison
	 */

	if (buffer[0] == '/') /* STIL syntax 2.10.2007 */
	{
	  chomp(buffer);

	  // force-terminate the string, apparently
	  // STIL textfile is written with windows or some other
	  end = strstr (buffer, ".sid");
	  if (end)
	  {
	    end += 4; *end = '\0';
	  }

	  /*
	   * FIXME - strdown aint working on arm (?)
	   * neither does g_ascii_strdown ?!1
	   */

	  g_tree_insert (tree,
			 g_ascii_strdown (buffer, strlen(buffer)),
			 g_strdup (buffer));
	}

      } while (!feof(in));

    fclose (in);
    }
}

unsigned int
super_search::init(void)
{
  tree = g_tree_new_full((GCompareDataFunc)
			 comparison_func,
			 NULL,
			 destroy_string,
			 destroy_string);

  //
  // read STIL database to a binary tree (~1MB)
  //
  read_file_to_tree (tree, "/opt/msid/usr/share/msid/STIL.txt");

  return 1;
}

/*
 * called for each node in tree when searching
 * builds up a list with results and direct uri
 */
gboolean
traverse_func(gpointer key,
	      gpointer value,
	      gpointer needle)
{
  char buffer[256];

  if (strstr ((char*)key, (char*)needle))
  {
    snprintf(buffer, 256, "%s", (char*) value);
    search_results = g_slist_prepend(search_results, g_strdup(buffer));
  }

  /* continue search */
  return FALSE;
}

char*
get_filename_from_uri (char *uri)
{
  char *tmp = uri;
  while (strstr (tmp, "/"))
    tmp++;
  return tmp;
}

static gchar*
make_hvsc_path (gchar *uri)
{
#define HVSC_ROOT "HVSC/"
  char *p = strstr (uri, HVSC_ROOT);
  if (p)
  {
    p += strlen (HVSC_ROOT + 1);
    return g_strdup (p);
  }
  return NULL;
}

GSList *
super_search::search_for_sid(const char *needle,
			     gpointer data)
{
  GSList *tmp, *entry_list =NULL;
  gchar *needle_copy;

  /* error - previous search results exist - shouldn't */
  if (search_results)
  {
    return NULL;
  }

  needle_copy = g_ascii_strdown ((char*)needle, strlen((char*)needle));

  //DEBUG ("local search for [%s]\n", needle_copy);

  g_tree_foreach(tree, traverse_func, (gpointer) needle_copy);

  g_free(needle_copy);

  //DEBUG ("local search END.\n");

  if (!search_results)
  {
    return NULL;
  }

  for (tmp=search_results; tmp; tmp=g_slist_next(tmp))
  {
    msid_search_entry *entry = (msid_search_entry *) malloc (sizeof (msid_search_entry));

    entry->uri = g_strdup((char *) tmp->data);
    entry->file_name = get_filename_from_uri ((char *) entry->uri);
    entry->hvsc_path = make_hvsc_path (entry->uri);

    entry_list = g_slist_prepend (entry_list, entry);
  }

  g_slist_foreach (search_results, (GFunc) g_free, NULL);
  g_slist_free (search_results);
  search_results = NULL;

  return entry_list;
}
