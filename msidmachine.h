
#ifndef MSID_MACHINE_H
#define MSID_MACHINE_H

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib-2.0/glib.h>

#include <sidplay/player.h>
#include <sidplay/fformat.h>
#include <sidplay/myendian.h>

#include "plugins/driver_iface.h"

#include "paths.h"

//#define MSID_DEBUG 1

// states for msidMachine
enum {
  READY = 0,
  PLAY,
  STOP,
  BUSY
};


// these settings are used by default *only* if everything else fails
#define DEFAULT_AUDIO_PLUGIN "libmsid_pulse.so"
#define DEFAULT_SEARCH_PLUGIN "libmsid_c64org.so"
static const gchar default_config[] =
  "[msid_config]\naudio_plugin = " DEFAULT_AUDIO_PLUGIN "\n search_plugin = " DEFAULT_SEARCH_PLUGIN "\n";


// interface for audio and search plugin management
typedef struct _msid_plugin_iface
{
  void *dl_handle;
  void *plugin_object;

  create_t* create_plugin;
  destroy_t* destroy_plugin;

} msid_plugin_iface;


typedef struct playback_t
{
  emuEngine engine;
  msid_driver *driver;
  char *song;
  uword song_no;

  unsigned int loop;
  unsigned int secs;
  unsigned int channels;
  
} playback_data;


class msidMachine {

 public  :

  msidMachine();
  ~msidMachine();

  void play();
  unsigned int stop();
  void pause();

  // temporary functions to help migration
  msid_plugin_iface *audioPlugin() { return &audio_iface; }
  msid_plugin_iface *searchPlugin() { return &search_iface; }

  unsigned int setAudioPlugin(const char *plugin);
  unsigned int setSearchPlugin(const char *plugin);

  const char *audioPluginName() { return current_audio_plugin; }
  const char *searchPluginName() { return current_search_plugin; }

  void setSong(const char *path, guint subsong);
  void setSubSong(guint subsong);
  void setStereo(unsigned int toggle);
  void setLength(unsigned int secs);
  void setLoop(unsigned int loop);

  unsigned int length() { return play_data.secs; }
  unsigned int loop() { return play_data.loop; }
  unsigned int isPaused() { return paused; }

  unsigned int isPlaying();

 protected :

  unsigned int initDefaultPlugins();
  void closePlugins();

  unsigned int initAudioPlugin(const char *plugin);
  unsigned int initSearchPlugin(const char *plugin);

  void closeAudioPlugin();
  void closeSearchPlugin();

 private :

  char *current_audio_plugin;
  char *current_search_plugin;

  msid_plugin_iface audio_iface;
  msid_plugin_iface search_iface;

  playback_data play_data;

  unsigned int paused;

};


#endif
