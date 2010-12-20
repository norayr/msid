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

#include "driver_wav.h"

short wav_driver::initialize(void *format, int freq, int chn)
{
  this->set_bsize(1024);

  memset(&sfinfo, 0, sizeof(SF_INFO));
  sfinfo.samplerate = 44100;
  sfinfo.channels = chn;
  sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

  if (sf_format_check(&sfinfo) == true) {

    snd = sf_open("dump.wav", SFM_WRITE, &sfinfo);

  } else {
    return 0;
  }

  return 1;
}

short wav_driver::play_stream(unsigned char *buffer, int size)
{
  sf_write_raw(snd, buffer, size);

  return 1;
}

void wav_driver::stop()
{
}

void wav_driver::close()
{
  sf_write_sync(snd);
  sf_close(snd);
}

void wav_driver::set_config(emuConfig *cfg)
{
  cfg->frequency     = 44100;
  cfg->channels      = 2;

  cfg->bitsPerSample = 16;
  cfg->sampleFormat  = SIDEMU_SIGNED_PCM;
}
