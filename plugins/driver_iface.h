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


#ifndef MSID_DRIVER_IFACE_H
#define MSID_DRIVER_IFACE_H

#include <sidplay/player.h>
#include <sidplay/fformat.h>
#include <sidplay/myendian.h>

#include "debug.h"

class msid_driver
{
  public :
    msid_driver() {}
    virtual ~msid_driver() {}

    virtual short initialize(void *format, int freq, int chn) {return 0;}
    virtual short play_stream(unsigned char *buffer, int size) {return 0;}
    virtual void stop() {}
    virtual void close() {}

    virtual void set_config (emuConfig *config) {}

    // default implementation does nothing, plugins may use this
    virtual void set_path(const char *path) { return; }

    inline unsigned int bsize() { return buffer_size; }
    inline void set_bsize(unsigned int bsize) { buffer_size = bsize; }

 private :

    int buffer_size;

};

typedef msid_driver* create_t();
typedef void destroy_t (void*);

#endif
