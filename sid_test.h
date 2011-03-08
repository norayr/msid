
#ifndef MSID_SID_TEST
#define MSID_SID_TEST

#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <sidplay/player.h>
#include <sidplay/fformat.h>
#include <sidplay/myendian.h>

#include <glib.h>
#include <glib/gstdio.h>

unsigned int
create_sid_path(const char *path);

unsigned int
sidtune_is_ok(const gchar *path);

#ifdef HILDON
unsigned int mmc_ok();
#endif

unsigned int sidpath_ok();
unsigned int thumbpath_ok();

#endif

