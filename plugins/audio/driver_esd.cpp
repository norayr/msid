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

#include <stdio.h>
#include "driver_esd.h"

short esd_driver::initialize (void *format, int freq, int chn)
{
  esd_format_t fmt;

  // TODO - use parameters

  fmt = ESD_STREAM | ESD_PLAY | ESD_BITS16 | ESD_STEREO;

  DEBUG ("open esd socket\n");

  esd = esd_audio_open();
  esd = esd_play_stream (fmt, ESD_DEFAULT_RATE, NULL, "msid");

  if (!esd)
  {
    return 0;
  }

  this -> set_bsize (ESD_BUF_SIZE);

  return 1;
}

short
esd_driver::play_stream (unsigned char *buffer, int size)
{
  esd_audio_write ( (void*) buffer, size);
  return 1;
}

void
esd_driver::stop (void)
{
  ;
}

void
esd_driver::close (void)
{
  DEBUG ("close esd socket\n");
  esd_close (esd);
}

void
esd_driver::set_config (emuConfig *cfg)
{
  // 16-bit stereo sound, mjam!

  cfg->frequency     = ESD_DEFAULT_RATE;
  cfg->channels      = 2;
  cfg->bitsPerSample = 16;
  cfg->sampleFormat  = SIDEMU_SIGNED_PCM;
}
