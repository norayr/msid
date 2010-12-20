
#include "msidmachine.h"

// mutex for state communication between main and player thread
G_LOCK_DEFINE_STATIC(msid_state);
static volatile unsigned int msid_state = READY;
// mutex for pausing the playback of player thread
G_LOCK_DEFINE_STATIC(msid_pause);
static volatile unsigned int msid_pause = 0;

msidMachine::msidMachine()
{
  paused  = 0;

  play_data.driver = 0;
  play_data.channels = 2;

  play_data.song_no = 0;
  play_data.song = 0;

  play_data.loop = 1;
  play_data.secs = 5;

  current_audio_plugin = 0;
  current_search_plugin = 0;

  initDefaultPlugins();

  G_LOCK(msid_state);
  msid_state = READY;
  G_UNLOCK(msid_state);
}

msidMachine::~msidMachine()
{
  stop();

  if (play_data.song) {
    free(play_data.song);
  }

  if (current_audio_plugin) {
    free(current_audio_plugin);
  }

  if (current_search_plugin) {
    free(current_search_plugin);
  }

  closePlugins();
}

void msidMachine::setSong(const char *path, guint subsong)
{
  if (play_data.song) {
    free(play_data.song);
  }

  play_data.song = strdup(path);
  play_data.song_no = subsong;
}

void msidMachine::setSubSong(guint subsong)
{
  play_data.song_no = subsong;
}

void msidMachine::setLength(unsigned int secs)
{
#ifdef MSID_DEBUG
  printf("%s set length [%d]\n", __func__, secs);
#endif

  play_data.secs = secs;
}

void msidMachine::setStereo(unsigned int toggle)
{
  toggle ? play_data.channels = 2 : play_data.channels = 1;
}

void msidMachine::setLoop(unsigned int loop)
{
  play_data.loop = loop;
}

void msidMachine::closePlugins()
{
  closeAudioPlugin();
  closeSearchPlugin();
}

unsigned int msidMachine::initDefaultPlugins()
{
  GKeyFile *config_file = 0;
  gchar *config_value = 0;
  GError *error = 0;

  unsigned int result = 1;

  config_file = g_key_file_new();

  error=NULL;
  g_key_file_load_from_file(config_file,
                            CONFIG_FILE_FULL_PATH,
                            G_KEY_FILE_NONE,
                            &error);
  // load default configuration
  if (error) {
    error=NULL;
    g_key_file_load_from_data(config_file,
                              default_config,
                              sizeof(default_config),
                              G_KEY_FILE_NONE, &error);
  }

  // AUDIO PLUGIN
  config_value = g_key_file_get_string
    (config_file, "msid_config", "audio_plugin", &error);

  if (config_value) {

    if (!initAudioPlugin(config_value)) {
      fprintf(stderr ,"audio plugin init failed!\n");
    }

    g_free(config_value);

  } else {
    fprintf (stderr, "no audio plugin available");
    result = 0;
    goto out;
  }

  // SEARCH PLUGIN
  config_value = g_key_file_get_string
    (config_file, "msid_config", "search_plugin", &error);

  if (config_value) {

    if (!initSearchPlugin(config_value)) {
      fprintf(stderr ,"search plugin init failed!\n");
    }

    g_free(config_value);

  } else {
    fprintf (stderr, "no search plugin available");
    result = 0;
    goto out;
  }


 out:

  g_key_file_free(config_file);

#ifdef MSID_DEBUG
  printf("msid init [%d]\n", result);
#endif

  return result;
}


// playback thread, reads data from libsidplay and feeds
// it to audio plugin, listens to state changes made by main thread.
void *msid_playback_thread(void *arg)
{
  unsigned int ready = 0;
  playback_data *m = (playback_data *) arg;

  ubyte *buffer = 0;
  sidTune *tune = 0;

#ifdef MSID_DEBUG
  printf("%s enter [%s] subsong %d\n", __func__, m->song, m->song_no);
#endif

  // final sanity check, this should never happen
  if (!m->song) {
    fprintf(stderr, "critical - no song?!1\n");
    goto exit_playback_thread;
  }


  buffer = new ubyte[m->driver->bsize()];
  tune = new sidTune(m->song);

  sidEmuInitializeSong(m->engine, *tune, m->song_no);

  m->engine.resetSecondsThisSong();

  while (!ready) {

    // exit thread if stopped
    G_LOCK(msid_state);
    if (msid_state == STOP) {
      ready = 1;
    }
    G_UNLOCK(msid_state);

    // pause here
    G_LOCK(msid_pause);
    G_UNLOCK(msid_pause);

    sidEmuFillBuffer(m->engine, *tune, buffer, m->driver->bsize());

    // stop playback in case of error
    if (!m->driver->play_stream(buffer, m->driver->bsize())) {

#ifdef MSID_DEBUG
    printf("%s driver->play_stream error\n", __func__);
#endif

      ready = 1;
    }

    if (!m->loop) {

      // finish when timeout
      if (m->engine.getSecondsThisSong() >= (int) (m->secs + 1)) {

#ifdef MSID_DEBUG
	printf("%s time is up [%d >= %d] secs\n", __func__, m->engine.getSecondsThisSong(), m->secs+1);
#endif
        ready = 1;
      }
    }
  }

  delete tune;
  delete [] buffer;

  m->driver->close();


 exit_playback_thread :

  G_LOCK(msid_state);
  msid_state = READY;
  G_UNLOCK(msid_state);

#ifdef MSID_DEBUG
  printf("%s exit\n", __func__);
#endif

  return NULL;
}

unsigned int msidMachine::isPlaying()
{
  unsigned int result = 0;

  G_LOCK(msid_state);
  if (msid_state == PLAY) {
    result = 1;
  }
  G_UNLOCK(msid_state);

  return result;
}

void msidMachine::play()
{
  GError *error = NULL;

  G_LOCK(msid_state);

  if (msid_state == READY) {
    msid_state = PLAY;
  } else {

#ifdef MSID_DEBUG
    printf("%s stop playback\n", __func__);
#endif

    stop();
  }

  G_UNLOCK(msid_state);


  struct emuConfig cfg;

  play_data.driver = (msid_driver *) audioPlugin()->plugin_object;

  // initialize driver plugin and configure libsidplay

  play_data.driver->initialize(NULL, 0, play_data.channels);

  play_data.engine.getConfig(cfg);
  play_data.driver->set_config(&cfg);
  play_data.engine.setConfig(cfg);

  g_thread_create(msid_playback_thread, (void*) &play_data, FALSE, &error);
}

unsigned int msidMachine::stop()
{
  unsigned int ready = 0, timer = 100, result = 1;

#ifdef MSID_DEBUG
    printf("%s called\n", __func__);
#endif

  G_LOCK(msid_state);

  // maybe already stopped?
  if (msid_state != PLAY && msid_state != BUSY) {
    G_UNLOCK(msid_state);
    return result;
  }

  msid_state = STOP;
  G_UNLOCK(msid_state);

  while (!ready) {
    usleep (10000);

    G_LOCK(msid_state);
    if (msid_state == READY) {
      ready=1;
    }
    G_UNLOCK(msid_state);

    // just in case something goes wrong - should not happen
    timer--;
    if (!timer) {
      fprintf(stderr, "timer run out, did not stop sid machine properly?\n");
      result = 0;
      break;
    }
  }

  return result;
}


void msidMachine::pause()
{

#ifdef MSID_DEBUG
  printf("%s pause toggle [%d -> %d]\n", __func__, paused, !paused);
#endif

  if (!paused) {
    G_LOCK(msid_pause);
  } else {
    G_UNLOCK(msid_pause);
  }

  paused = !paused;
}


/*
 * generic init functions to msid c++ plugin object
 * based on http://www.faqs.org/docs/Linux-mini/C++-dlopen.html
 */
static unsigned int
init_msid_plugin (msid_plugin_iface *iface,
                  const char *path)
{
  char *error;
  void *handle;

  handle = dlopen(path, RTLD_LAZY);

  if (!handle) {
    fputs(dlerror(), stderr);
    return 0;
  }

  dlerror();

  iface->create_plugin  =
    (create_t*) dlsym (handle, "create");

  if ((error = dlerror()) != NULL) {
    fprintf (stderr, "%s\n", error);
    return 0;
  }

  dlerror();

  iface->destroy_plugin =
    (destroy_t*) dlsym (handle, "destroy");

  if ((error = dlerror()) != NULL) {
    fprintf (stderr, "%s\n", error);
    return 0;
  }

  iface->plugin_object = iface->create_plugin();
  iface->dl_handle = handle;

  return 1;
}



unsigned int msidMachine::initAudioPlugin(const char *plugin)
{
  char buffer[256];
  snprintf(buffer, 256, "%s/audio/%s", PLUGIN_FILE_PATH, plugin);

#ifdef MSID_DEBUG
  printf("%s : [%s]\n", __func__, plugin);
#endif
 
  if(!g_file_test(buffer, G_FILE_TEST_EXISTS)) {
    fprintf(stderr, "file does not exist\n");
    return 0;
  }

  init_msid_plugin(&audio_iface, buffer);

  if (current_audio_plugin) {
    free(current_audio_plugin);
  }

  current_audio_plugin = strdup(plugin);

  return 1;
}

unsigned int msidMachine::initSearchPlugin(const char *plugin)
{
  char buffer[256];
  snprintf(buffer, 256, "%s/search/%s", PLUGIN_FILE_PATH, plugin);
 
#ifdef MSID_DEBUG
  printf("%s : [%s]\n", __func__, plugin);
#endif

  if(!g_file_test(buffer, G_FILE_TEST_EXISTS)) {
    fprintf(stderr, "file does not exist\n");
    return 0;
  }

  init_msid_plugin(&search_iface, buffer);

  if (current_search_plugin) {
    free(current_search_plugin);
  }

  current_search_plugin = strdup(plugin);

  return 1;
}

void msidMachine::closeAudioPlugin()
{
#ifdef MSID_DEBUG
  printf("%s\n", __func__);
#endif

  audio_iface.destroy_plugin(audio_iface.plugin_object);
  dlclose(audio_iface.dl_handle);
}

void msidMachine::closeSearchPlugin()
{
#ifdef MSID_DEBUG
  printf("%s\n", __func__);
#endif

  search_iface.destroy_plugin(search_iface.plugin_object);
  dlclose(search_iface.dl_handle);
}

unsigned int msidMachine::setAudioPlugin(const char *plugin)
{
#ifdef MSID_DEBUG
  printf("%s : [%s]\n", __func__, plugin);
#endif

  closeAudioPlugin();

  return initAudioPlugin(plugin);
}

unsigned int msidMachine::setSearchPlugin(const char *path)
{
#ifdef MSID_DEBUG
  printf("%s : [%s]\n", __func__, path);
#endif

  closeSearchPlugin();

  return initSearchPlugin(path);
}


