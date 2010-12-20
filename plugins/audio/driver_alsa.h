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

#ifndef MSID_ALSA_DRIVER
#define MSID_ALSA_DRIVER

#include <alsa/asoundlib.h>

#include "../driver_iface.h"

class alsa_driver : public msid_driver
{
  public :

    alsa_driver()
    : playback_handle(NULL) {}

  short initialize(void *format, int freq, int chn);
  short play_stream(unsigned char *buffer, int size);
  void stop();
  void close();

  void set_config (emuConfig *cfg);

  private :
    snd_pcm_t *playback_handle;
    snd_pcm_hw_params_t *hw_params;
};

// class factories

extern "C" void* create() {
  DEBUG ("create ALSA driver\n");
  return new alsa_driver;
}

extern "C" void destroy (void *driver) {
  DEBUG ("destroy ALSA driver\n");
  delete (alsa_driver *) driver;
}

#endif
