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

#include "curling.h"

unsigned int
fetch_data_to_file (char *uri, char *file_path)
{
  struct stat stat_info;

  CURL *curl;
  CURLcode error_code;
  FILE *out;
  char buffer[256];
  int errors=0;

  DEBUG ("fetching [%s]\n", uri);

  out = fopen (file_path, "w");

  if (out) {
    curl = curl_easy_init();

    curl_easy_setopt (curl, CURLOPT_WRITEDATA, out);
    curl_easy_setopt (curl, CURLOPT_URL, uri);

    curl_easy_setopt (curl, CURLOPT_TIMEOUT, 60);

    error_code = curl_easy_perform (curl);

    if (error_code != 0) {
      fprintf (stderr, "ERROR : %s\n", curl_easy_strerror (error_code));
      errors=1;
    }

    curl_easy_cleanup(curl);
    fclose (out);

    /* got something ... let's analyze it a bit */
    if (!errors) {
      DEBUG ("got data, analyzing\n");

      // if does not exist, return
      if(stat(file_path, &stat_info) != 0) {
	errors=1;
	goto errors_found;
      }
      // check filesize - maybe a download has failed
      if (stat_info.st_size < 1) {
	errors=1;
	goto errors_found;
      }

      /* check for error_code */
      out = fopen (file_path, "r");
      if (out) {
	int k;
	for (k=0; k<20; k++) {
	  char *ret = fgets(buffer, 256, out);
	  if (ret && strstr(buffer, "404 Not Found")) {
	    errors=1;
	    break;
	  }
	}
	fclose (out);
      }
    }

  errors_found :

    if (errors) {
      DEBUG ("404 error, deleting data...\n");
      unlink (file_path);
    }

  }

  if (errors)
    return 0;
  return 1;
}
