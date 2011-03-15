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

#include "image_tab.h"

GtkWidget *image_box=NULL;
GtkWidget *image_button=NULL;

GtkWidget *anim_control_button;

GtkWidget *img=NULL;
guint animation_timeout =0;
guint stop_animation =0;

static void (*set_status_text) (const char*, const char*) = NULL;

#define IM_W 320
#define IM_H 200

#define IM_SCALE_H 270
#define IM_SCALE_W (IM_W*IM_SCALE_H) / IM_H

typedef struct _anim
{
  GdkPixbuf *pix;
  gchar game_name[256];
  guint frame;
  guint frames_total;
} msid_animation;

msid_animation anim;

char*
get_filename_from_uri (char *uri)
{
  char *tmp = uri;
  while (strstr (tmp, "/"))
    tmp++;
  return tmp;
}

static void
darken_anim_image (GdkPixbuf *pixbuf)
{
  int width, height, rowstride, n_channels, x, y;
  guchar *pixels, *p;

  n_channels = gdk_pixbuf_get_n_channels (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  pixels = gdk_pixbuf_get_pixels (pixbuf);

  //  p = pixels + y * rowstride + x * n_channels;

  for (y=0; y<height; y++) {
    p = pixels + y * rowstride;

    for (x=0; x<width; x++) {
      p[0] /= 3;
      p[1] /= 3;
      p[2] /= 3;

      p += n_channels;
    }
  }
}

void
darken_current_animation_image ()
{
  if (anim.pix)
    darken_anim_image(anim.pix);
}


static void
draw_text (char *txt)
{
  PangoRenderer *renderer;
  //PangoMatrix matrix = PANGO_MATRIX_INIT;
  PangoContext *context;
  PangoLayout *layout;
  PangoFontDescription *desc;

  renderer = gdk_pango_renderer_get_default (gdk_screen_get_default());
  gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (renderer), img->window);

  gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (renderer), img->style->white_gc);

  /* Create a PangoLayout, set the font and text */
  context = gtk_widget_create_pango_context (img);
  layout = pango_layout_new (context);
  pango_layout_set_text (layout, txt, -1);
  desc = pango_font_description_from_string ("sans 20");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);


  gdk_draw_layout (GTK_WIDGET(img)->window,
		   img->style->white_gc,
		   0,
		   0,
		   layout);


  /*
  int w, h;
  w = h = 328;
  pango_layout_get_size (layout, &w, &h);
  pango_renderer_draw_layout (renderer, layout, 320/2 * PANGO_SCALE, 200/2 * PANGO_SCALE);
  */

  /* free the objects we created */
  g_object_unref (layout);
  g_object_unref (context);
}


static gboolean
change_picture(gpointer data)
{
  char tmp_fname[256];
  GError *error=NULL;
  msid_animation *anim = (msid_animation *) data;

  gdk_threads_enter();

  if (stop_animation) {
    animation_timeout=0;
    stop_animation=0;

    gdk_flush();
    gdk_threads_leave();
    return FALSE;
  }

  if (anim->frame +1 < 10)
    snprintf(tmp_fname, 256, "%s/sidthumbs/%s_0%d.gif", getenv("HOME"), anim->game_name, anim->frame++);
  else
    snprintf(tmp_fname, 256, "%s/sidthumbs/%s_%d.gif", getenv("HOME"), anim->game_name, anim->frame++);

  if (anim->pix)
    g_object_unref(anim->pix);

  anim->pix = gdk_pixbuf_new_from_file_at_scale(tmp_fname,
						IM_SCALE_W, -1,
						TRUE,
						&error);

  gtk_image_set_from_pixbuf(GTK_IMAGE(img), anim->pix);

  gdk_flush();
  gdk_threads_leave();

  if (anim->frame == anim->frames_total)
    anim->frame = 1;

  return TRUE;
}

void
stop_screenshot_animation()
{
  if (animation_timeout != 0) {
    gtk_button_set_label(GTK_BUTTON(anim_control_button), " PLAY ");
    stop_animation = 1;
  }
}

void
play_screenshot_animation()
{
  if (animation_timeout == 0 && anim.frames_total > 0) {
    gtk_widget_set_sensitive(anim_control_button, TRUE);
    gtk_button_set_label(GTK_BUTTON(anim_control_button), " PAUSE ");

    stop_animation = 0;
    animation_timeout = g_timeout_add(3000, (GSourceFunc) change_picture, &anim);
  }
}

void
toggle_screenshot_animation (GtkButton *button, gpointer data)
{
  /* if playing -> pause */
  if (animation_timeout != 0) {

    stop_screenshot_animation();

    /*
      darken (anim.pix);
      gtk_image_set_from_pixbuf (GTK_IMAGE(img), anim.pix);
      gdk_display_sync (gdk_display_get_default());
      draw_text ("PAUSE");
    */
  }
  /* if pause -> play */
  else {
    play_screenshot_animation();
  }
}


// Yahoo! image search
// http://developer.yahoo.com/search/image/V1/imageSearch.html
unsigned int yahoo_image_search(const char *name)
{
  struct stat stat_info;
  FILE *in;

  char buffer[1024];
  char tmp[256];

  char *startp; // start of uri 'imgurl\x3d'
  char *endp; // end of uri '\x26'
  char *p;

  char *blob;
  char *filename;

  unsigned int amount = 0;

  char needle[256];

  snprintf(needle, 256, "%s", name);
  p = strstr(needle, ".");
  sprintf(p, "+c64");

#define MSID_YAHOO_ID "o0yvGCjV34GMCmuam_eARZZ6dR0OL09dwlu1ZTczWstOWvVk2BDZuGUbXit78zBOVXS9wFVSLIdewqBt0FdLUtndjyPtCfc-"

  snprintf(buffer, 1024, "http://search.yahooapis.com/ImageSearchService/V1/imageSearch?appid=%s&query=%s&results=10", MSID_YAHOO_ID, needle);

  fetch_data_to_file(buffer, "/tmp/msid_yahoo.xml");

  // if does not exist, return
  if(stat("/tmp/msid_yahoo.xml", &stat_info) != 0) {
    return 0;
  }
  // check filesize - maybe a download has failed
  if (stat_info.st_size < 1) {
    return 0;
  }
  // ok fair enough, we have a file, read it in

  blob = (char*) malloc (stat_info.st_size);

  // traverse through html file searching for key 'imgurl\x3d' and fetch contents of url behind that until '\x26'

  p = blob;

  // read the whole buffer to memory
  in = fopen("/tmp/msid_yahoo.xml", "r");
  if (in) {
    do {
      fgets(p, 256, in);
      p += strlen(p);
    } while (!feof(in));
    fclose(in);
  } else {
    goto away;
  }

  p = strstr(blob, "totalResultsReturned");
  if (p) {

    p += 22;
    endp = strstr(p, "\"");

    // length of field is (endp - startp) + 1
    snprintf(buffer, (endp - p) + 1, "%s\\0", p);

    // check if no results ...
    if (atoi(buffer) <= 0) {
      goto away;
    }

  } else {
    goto away;
  }

#define YAHOO_START_KEY "Summary><Url>"

  // inspect file with strstr
  p = strstr(blob, YAHOO_START_KEY);
  while (p) {

    // set start and end pointers
    startp = p; startp += strlen(YAHOO_START_KEY);
    endp = strstr(startp, "</Url>");

    // length of required uri buffer is (endp - startp) + 1
    snprintf(buffer, (endp - startp) + 1, "%s\\0", startp);

    // TODO fetch these files to some tmp directory

    amount++;
    filename = get_filename_from_uri(buffer);
    snprintf(tmp, 256, "/tmp/%s", filename);

    //    fetch_data_to_file(buffer, tmp);

    p = strstr(endp, YAHOO_START_KEY);
  }

 away:

  free(blob);
  return amount;
}

// experimental google image search
// note - this cannot be really used due to terms defined by google
unsigned int google_image_search(const char *name)
{
  struct stat stat_info;
  FILE *in;

  char buffer[256];

  char *startp; // start of uri 'imgurl\x3d'
  char *endp; // end of uri '\x26'
  char *p;

  char *blob;

  unsigned int amount = 0;

  snprintf(buffer, 256, "http://www.google.com/images?hl=fi&source=hp&biw=1920&bih=975&q=%s&gbv=2&aq=f&aqi=g3&aql=&oq=", name);
  fetch_data_to_file(buffer, "/tmp/msid_google.html");

  // if does not exist, return
  if(stat("/tmp/msid_google.html", &stat_info) != 0) {
    return 0;
  }
  // check filesize - maybe a download has failed
  if (stat_info.st_size < 1) {
    return 0;
  }
  // ok fair enough, we have a file, read it in

  blob = (char*) malloc (stat_info.st_size);

  // traverse through html file searching for key 'imgurl\x3d' and fetch contents of url behind that until '\x26'

  p = blob;

  // read the whole buffer to memory
  in = fopen("/tmp/msid_google.html", "r");
  if (in) {
    do {
      fgets(p, 256, in);
      p += strlen(p);
    } while (!feof(in));
    fclose(in);
  } else {
    goto away;
  }

  // inspect file with strstr
  p = strstr(blob, "imgurl\\x3d");
  while (p) {

    // set start and end pointers
    startp = p; startp += 10;
    endp = strstr(startp, "\\x26");

    // length of required uri buffer is (endp - startp) + 1
    snprintf(buffer, (endp - startp) + 1, "%s\\0", startp);

    amount++;

    p = strstr(endp, "imgurl\\x3d");
  }

 away:

  free(blob);
  return amount;
}

void *fetch_screenshot(void *file_path)
{
  char uri[256];
  char *sid_fname;
  char pic_fname[256];
  char tmp_fname[256];
  char buffer[256];
  gchar *name;
  char *ext;
  int k;

  sid_fname = get_filename_from_uri((char *) file_path);
  name = g_ascii_strdown(sid_fname, strlen(sid_fname));

  yahoo_image_search(name);

  ext = strstr (name, ".sid");
  ext[0] = '\0';

  snprintf (anim.game_name, 256, "%s", name);

  // will fetch images until fails
#define FRAME_AMOUNT 20

  for (k=1; k<FRAME_AMOUNT+1; k++) {

    snprintf(pic_fname, 256, "%s_0%d.gif", name, k);

#define C64_ORG_SCREENSHOT_ROOT "http://www.c64.com/games/screenshots"

    // check that directory exists, if not create one
    if (!thumbpath_ok()) {
      return NULL;
    }

    snprintf(uri, 256, "%s/%c/%s", C64_ORG_SCREENSHOT_ROOT, name[0], pic_fname);
    snprintf(tmp_fname, 256, "%s/sidthumbs/%s", getenv("HOME"), pic_fname);

    if (g_file_test (tmp_fname, G_FILE_TEST_EXISTS)) {
      /* we have this pic already - no need to fetch */
      k++;
      continue;
    }

    fetch_data_to_file(uri, tmp_fname);

    if (!g_file_test (tmp_fname, G_FILE_TEST_EXISTS)) {
      /* no picture at all */
      if (k==1) {
	/* not a single image ... buhuu */
	gdk_threads_enter();
	snprintf (buffer, 256, "SORRY, NO IMAGES FOUND");
	set_status_text (buffer, NULL);
	gtk_widget_set_sensitive (image_button, FALSE);
	gtk_widget_set_sensitive (anim_control_button, FALSE);
	gdk_flush();
	gdk_threads_leave();

	return NULL;
      }
      /* picture count ends here */
      else {
	gdk_threads_enter();
	snprintf (buffer, 256, "%d IMAGES FOUND", k-1);
	set_status_text (buffer, NULL);
	gdk_flush();
	gdk_threads_leave();

	break;
      }
    }
  }

  anim.frame = 1;
  anim.frames_total = k;

  change_picture(&anim);

  if (!animation_timeout) {
    gdk_threads_enter();
    play_screenshot_animation();
    gdk_flush();
    gdk_threads_leave();
  }

  g_free (name);

  return NULL;
}

void set_game_screenshot(char *name)
{
  GError *error =NULL;

  g_thread_create(fetch_screenshot, name, FALSE, &error);
}

static gboolean
image_clicked(GtkWidget      *widget,
	      GdkEventButton *event,
	      gpointer        user_data)
{
  if (animation_timeout == 0 && anim.frames_total > 0) {
    play_screenshot_animation();
  }
  else if (animation_timeout != 0) {
    stop_screenshot_animation();
  }
  return TRUE;
}

GtkWidget* create_image_page (void (*status_text_func) (const char*, const char*))
{
  GtkWidget

    *image_main_box,
    *event_box,
    *ctrl_box;

  anim.pix = NULL;
  anim.frames_total=0;

  img = gtk_image_new();

  set_status_text = status_text_func;

  anim_control_button = gtk_button_new_with_label ("PAUSE");
  gtk_widget_set_sensitive (anim_control_button, FALSE);

  gtk_widget_show_all (img);

  image_main_box = gtk_vbox_new (FALSE, 0);
  image_box = gtk_vbox_new (FALSE, 0);
  ctrl_box = gtk_hbox_new (FALSE, 0);

  event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (event_box), img);

  /*
  g_signal_connect (G_OBJECT(event_box), "button-release-event",
		    G_CALLBACK(image_clicked), NULL);
  */

  g_signal_connect (G_OBJECT(anim_control_button), "clicked",
		    G_CALLBACK(toggle_screenshot_animation), NULL);

  image_button = gtk_button_new_with_label ("DOWNLOAD");

  gtk_box_pack_start (GTK_BOX(ctrl_box), image_button, TRUE, TRUE, 2);
  gtk_box_pack_start (GTK_BOX(ctrl_box), anim_control_button, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX(image_box), event_box, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX(image_main_box), image_box, FALSE, FALSE, 2);
  gtk_box_pack_start (GTK_BOX(image_main_box), ctrl_box, FALSE, FALSE, 2);
  
  return image_main_box;
}
