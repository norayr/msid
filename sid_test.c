
#include "sid_test.h"

static unsigned int mkdir(const char *path)
{
  if (g_mkdir (path, 0777) == 0)
    return 1;
  else
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

unsigned int sidpath_ok()
{
  char path[256];

  snprintf (path, 256, "%s/sidmusic", getenv("HOME"));

  if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
    return 1;
  }

  /* does not exist, let's try to create one */
  return mkdir(path);
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
