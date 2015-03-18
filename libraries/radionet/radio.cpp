
/*
 RF12 utilities.

 Copyright (C) 2012 Dave Berkeley projects@rotwang.co.uk

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 USA
*/

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include <pins_arduino.h>
#endif

#include <JeeLib.h>

#include <radio.h>

    /*
     *  Tidier interface to the RF12 library
     */

class RadioData
{
public:
    uint8_t node;
    uint8_t* msg;
    uint8_t size;

    uint8_t wait;
};

static RadioData msgs;

Radio::Radio()
{
}

bool Radio::init(uint8_t node, uint8_t group, Band band, uint8_t wait)
{
    msgs.wait = wait;

    // TODO : Support other bands
    if (band != MHz_868)
        return false;

    const uint8_t b = RF12_868MHZ;
    rf12_initialize(node, b, group);
    return true;
}

void Radio::poll()
{
    if (rf12_recvDone() && (rf12_crc == 0)) {
        on_rx(rf12_hdr & 0x1F, (uint8_t*) rf12_data, rf12_len);
    }

    // check for send if required
    if (!msgs.msg)
        return;

    if (!rf12_canSend())
        return;

    //  send the mesage
    on_tx(msgs.node);
    rf12_sendStart(msgs.node, msgs.msg, msgs.size);
    rf12_sendWait(msgs.wait);
    free(msgs.msg);
    msgs.msg = 0;
}

void Radio::send(uint8_t node, uint8_t* msg, uint8_t size)
{
    msgs.node = node;
    msgs.size = size;
    free(msgs.msg);
    msgs.msg = (uint8_t*) malloc(size);
    memcpy(msgs.msg, msg, size);
}

// FIN
