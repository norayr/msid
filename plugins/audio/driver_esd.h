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

#ifndef MSID_ESD_DRIVER
#define MSID_ESD_DRIVER

#include <esd.h>
#include "../driver_iface.h"

class esd_driver : public msid_driver
{
  public :

  short initialize(void *format, int freq, int chn);
  short play_stream(unsigned char *buffer, int size);
  void stop();
  void close();

  void set_config (emuConfig *cfg);

  private :
    int esd;
};

// class factories

extern "C" void* create() {
  DEBUG ("create ESD driver\n");
  return new esd_driver;
}

extern "C" void destroy (void *driver) {
  DEBUG ("destroy ESD driver\n");
  delete (esd_driver *) driver;
}


#endif
