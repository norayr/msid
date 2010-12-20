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

#include "search_plugin_tp2b.h"
#include "../curling.c"

#define TMP_FILE "/tmp/sidweb_tmp.txt"
#define SID_SERVER "http://hvsc.tp2.be/"
#define BUFSIZE 256

static unsigned int
parse_uri_from_sidsearch (const char *line, char*uri)
{
  char *tmp, *end;

  uri[0] = '\0';

  if ((strstr(line, "&path=")) &&
      (strstr(line, ".sid")))
  {
    // we have a hit
    tmp = strstr (line, "&path=");
    tmp += 6;

    end = strstr (tmp, ".sid");
    end[4] = '\0';

    snprintf (uri, BUFSIZE, "%s%s", SID_SERVER, tmp);

    return 1;
  }
  return 0;
}

static GSList*
parse_uris_from_http_page (char *file_path)
{
  FILE *in;
  GSList *list=NULL;
  char buffer[BUFSIZE];
  gchar uri[BUFSIZE];

  in = fopen (file_path, "r");
  
  if (in)
  {
    do
    {
      fgets (buffer, 256, in);

      if (parse_uri_from_sidsearch (buffer, uri))
      {
	list = g_slist_prepend (list, g_strdup (uri));
      }

    } while (!feof(in));
    fclose (in);
  }
  return list;
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
#define HVSC_ROOT "C64Music/"
  char *p = strstr (uri, HVSC_ROOT);
  if (p)
  {
    p += strlen (HVSC_ROOT + 1);
    return g_strdup (p);
  }
  return NULL;
}


GSList *
tp2b_search::search_for_sid (const char *needle,
			     gpointer data)
{
  char buffer[256];
  GSList *uri_list, *tmp;
  GSList *entry_list = NULL;

  DEBUG ("searching for [%s] (%s)\n", needle, SID_SERVER);

  snprintf (buffer, 256, "%s?q=%s", SID_SERVER, needle);

  fetch_data_to_file (buffer, TMP_FILE);

  uri_list = parse_uris_from_http_page (TMP_FILE);

  for (tmp=uri_list; tmp; tmp=g_slist_next(tmp))
    {
      msid_search_entry *entry = (msid_search_entry *) malloc (sizeof (msid_search_entry));
    
      entry -> uri = g_strdup((char *) tmp->data);
      entry -> file_name = get_filename_from_uri ((char *) entry->uri);
      entry -> hvsc_path = make_hvsc_path (entry->uri);

      entry_list = g_slist_prepend (entry_list, entry);
    }

  g_slist_foreach (uri_list, (GFunc) g_free, NULL);
  g_slist_free (uri_list);

  unlink (TMP_FILE);

  return entry_list;
}
