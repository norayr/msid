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

#include "driver_alsa.h"

short alsa_driver::initialize(void *format, int freq, int chn)
{
  int err;
  unsigned int frequency = 22050; 
  //unsigned int frequency = 44100;
  snd_pcm_format_t fmt;

  // fmt = *((snd_pcm_format_t*)format);

  fmt = SND_PCM_FORMAT_S8;
    //fmt = SND_PCM_FORMAT_S16_LE;

  DEBUG ("ALSA : open playback handle\n");

  if ((err = snd_pcm_open (&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0)
  {
    fprintf (stderr, "cannot open audio device (%s)\n",
             snd_strerror (err));
    return 0;
  }

  if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
    fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (err));
    return 0;
  }

  if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (err));
    return 0;
  }

  if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (err));
    return 0;
  }

  if ((err = snd_pcm_hw_params_set_format (playback_handle, hw_params, fmt)) < 0) {
    fprintf (stderr, "cannot set sample format (%s)\n",
             snd_strerror (err));
    return 0;
  }

  if ((err = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &frequency, 0)) < 0) {
    fprintf (stderr, "cannot set sample rate (%s)\n",
             snd_strerror (err));
    return 0;
  }

  if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, 1)) < 0) {
    fprintf (stderr, "cannot set channel count (%s)\n",
             snd_strerror (err));
    return 0;
  }

  if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot set parameters (%s)\n",
             snd_strerror (err));
    return 0;
  }

  snd_pcm_hw_params_free (hw_params);
  hw_params=NULL;

  if ((err = snd_pcm_prepare (playback_handle)) < 0) {
    fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
             snd_strerror (err));
    return 0;
  }

  /* FIXME : figure out good buffersize for ALSA */
  this -> set_bsize (256);

  return 1;
}

short
alsa_driver::play_stream(unsigned char *buffer, int size)
{
  int err;
  if ((err = snd_pcm_writei (playback_handle, buffer, size)) != size) {
    fprintf (stderr, "write to audio interface failed (%s)\n",
	     snd_strerror (err));
    return 0;
  }
  return 1;
}

void
alsa_driver::stop()
{
  ;
}

void
alsa_driver::close()
{
  if (playback_handle)
  {
    DEBUG ("ALSA : close playback handle\n");

    snd_pcm_drain (playback_handle);
    snd_pcm_close (playback_handle);

    playback_handle = NULL;
  }
}

void
alsa_driver::set_config (emuConfig *cfg)
{
  cfg->frequency     = 22050;
  cfg->channels      = 1;
  cfg->bitsPerSample = 8;
  cfg->sampleFormat  = SIDEMU_SIGNED_PCM;
}
