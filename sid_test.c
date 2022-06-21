#include <unistd.h>
#include "sid_test.h"

static unsigned int mkdir(const char *path)
{
  if (g_mkdir (path, 0777) == 0)
    return 1;
  else
    return 0;
}

typedef struct fileinfo_t
{
  gchar *path;
  unsigned int size;
  time_t access;
} file_info;

static void free_fileinfo(gpointer data, gpointer user_data)
{
  file_info *info = (file_info *) data;
  g_free(info->path);
  free(info);
}


static GSList *directory_contents(const gchar *path, unsigned int *size)
{
  const gchar *fname =NULL;
  struct stat stat_info;
  char buffer[256];
  GError *error =NULL;
  GSList *list = NULL;
  unsigned int total = 0;
  GDir *dir;

  dir = g_dir_open(path, 0, &error);

  if (dir) {
    fname = g_dir_read_name(dir);

    while (fname != NULL) {

      snprintf(buffer, 256, "%s/%s", path, fname);
      if(stat(buffer, &stat_info) == 0) {
	total += stat_info.st_size;
      }

      file_info *data = (file_info*) malloc (sizeof(file_info));

      data->path   = strdup(buffer);
      data->size   = stat_info.st_size;
      data->access = stat_info.st_atime; // how hot?

      list = g_slist_prepend(list, data);
      fname = g_dir_read_name(dir);
    }
    g_dir_close(dir);
  }

  if (size) {
    *size = total;
  }

  return list;
}

unsigned int directory_size(const gchar *path)
{
  const gchar *fname =NULL;
  struct stat stat_info;
  unsigned int result = 0;
  char buffer[256];
  GError *error =NULL;
  GDir *dir;

  dir = g_dir_open(path, 0, &error);

  if (dir) {
    fname = g_dir_read_name(dir);

    while (fname != NULL) {

      snprintf(buffer, 256, "%s/%s", path, fname);
      if(stat(buffer, &stat_info) == 0) {
	result += stat_info.st_size;
      }
      fname = g_dir_read_name(dir);
    }
    g_dir_close(dir);
  }

  return result;
}

static gint sort_by_access(gconstpointer a, gconstpointer b)
{
  file_info *ai = (file_info *) a;
  file_info *bi = (file_info *) b;

  return ai->access > bi->access ? 1 : -1;
}

static void print_fileinfo(gpointer data, gpointer user_data)
{
  file_info *info = (file_info *) data;
  printf("%s\n", info->path);
}


unsigned int truncate_cache_dir(const gchar *path, unsigned int limit)
{
  unsigned int total_size = 0;
  unsigned int trash_size = 0;
  GSList *contents = directory_contents(path, &total_size);

  if (total_size < limit) {
    DEBUG("thumbnail cache is small enough\n");
    return 0;
  }

  // sort items by access field to remove cold items, leave hot ones
  contents = g_slist_sort(contents, sort_by_access);

  DEBUG("cache too big! %d vs %d, need to chop off %d bytes\n", total_size, limit, total_size - limit);

  if (contents && total_size) {

    GSList *p = contents;
    while (trash_size < total_size - limit) {

      if (p) {

	file_info *info = (file_info *) p->data;

	// unlink file
	DEBUG("unlink [%s] .. [%d] bytes - access [%d]\n", info->path, info->size, info->access);

	unlink(info->path);

	trash_size += info->size;

      } else {
	break;
      }

      p = g_slist_next(p);
    }

    g_slist_foreach(contents, (GFunc)free_fileinfo, NULL);
    g_slist_free(contents);

    return 1;
  }

  return 0;
}

unsigned int create_sid_path(const char *path)
{
  /* path exists, yippee ... */
  if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
    return 1;
  }

  /* does not exist, let's try to create one */
  return mkdir(path);
}

#ifdef HILDON
unsigned int mmc_ok()
{
  char path[] = "/home/user/MyDocs/sidmusic";

  if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
    return 1;
  }

  /* does not exist, let's try to create one */
  return mkdir(path);
}
#endif

static unsigned int path_ok(const char *path)
{
  if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
    return 1;
  }

  /* does not exist, let's try to create one */
  return mkdir(path);
}

unsigned int sidpath_ok()
{
  char path[256];

  snprintf(path, 256, "%s/sidmusic", getenv("HOME"));

  return path_ok(path);
}

unsigned int thumbpath_ok()
{
  char path[256];

  snprintf(path, 256, "%s/sidthumbs", getenv("HOME"));

  return path_ok(path);
}


unsigned int sidtune_is_ok(const char*path)
{
  struct sidTuneInfo info;
  struct stat stat_info;

  if (!strstr(path, ".sid")) {
    return 0;
  }

  /* if does not exist, return */
  if(stat(path, &stat_info) != 0) {
    return 0;
  }

  // check filesize - maybe a download has failed
  if (stat_info.st_size < 1) {
    if (g_file_test(path, G_FILE_TEST_IS_REGULAR) &&
        !g_file_test(path, G_FILE_TEST_IS_SYMLINK)) {
      unlink (path);
    }
    return 0;
  }

  sidTune tune(path);

  memset(&info, 0, sizeof(sidTuneInfo));

  if (tune.getInfo(info) != true) {
    return 0;
  }

  if (info.authorString == 0) {
    return 0;
  }

  if (info.nameString == 0) {
    return 0;
  }

  if (info.songs <= 0) {
    return 0;
  }

  if (g_ascii_strcasecmp(info.nameString, "") == 0) {
    return 0;
  }

  return 1;
}
