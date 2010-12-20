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

#ifndef MSID_PULSE_DRIVER
#define MSID_PULSE_DRIVER

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#include "../driver_iface.h"

class pulse_driver : public msid_driver
{
  public :

  pulse_driver() {}

  short initialize(void *format, int freq, int chn);
  short play_stream(unsigned char *buffer, int size);
  void stop();
  void close();

  void set_config (emuConfig *cfg);

  private :

  pa_simple *s;
  pa_sample_spec ss;

};

// class factories

extern "C" void* create() {
  DEBUG ("create PULSE driver\n");
  return new pulse_driver;
}

extern "C" void destroy (void *driver) {
  DEBUG ("destroy PULSE driver\n");
  delete (pulse_driver *) driver;
}

#endif
