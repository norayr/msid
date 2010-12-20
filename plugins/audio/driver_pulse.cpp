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

#include "driver_pulse.h"

short pulse_driver::initialize(void *format, int freq, int chn)
{
  this -> set_bsize (1024);

  s = NULL;

  ss.format = PA_SAMPLE_S16NE;
  ss.channels = chn;
  ss.rate = 44100;

  s = pa_simple_new(NULL,               // Use the default server.
		    "msid",             // Our application's name.
		    PA_STREAM_PLAYBACK,
		    NULL,               // Use the default device.
		    "Music",            // Description of our stream.
		    &ss,                // Our sample format.
		    NULL,               // Use default channel map
		    NULL,               // Use default buffering attributes.
		    NULL                // Ignore error code.
		    );

  if (!s) return 0;
  return 1;
}

short
pulse_driver::play_stream(unsigned char *buffer, int size)
{
  int error;
  pa_simple_write(s, buffer, size, &error);
  return 1;
}

void
pulse_driver::stop()
{
  int error;
  pa_simple_flush(s, &error);
}

void
pulse_driver::close()
{
  pa_simple_free(s);
}

void
pulse_driver::set_config (emuConfig *cfg)
{
  cfg->frequency     = ss.rate;
  cfg->channels      = ss.channels;
  
  cfg->bitsPerSample = 16;
  cfg->sampleFormat  = SIDEMU_SIGNED_PCM;
}
