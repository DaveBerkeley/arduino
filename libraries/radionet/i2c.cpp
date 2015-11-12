/*
 Copyright (C) 2015 Dave Berkeley projects2@rotwang.co.uk

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

#include <stdint.h>

#include "i2c.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))

    /*
     *  I2C library.
     *
     *  Intended to be used in bootloader, so needs to be in C, not C++,
     *  and independent of the excellent jeelib library,
     *  from which it borrows.
     */


    /*
     *  I2C pin manipulation primitives
     */

void i2c_sda(I2C* i2c, bool data)
{
    // pull low, float hi
    pin_mode(i2c->sda, !data);
    pin_set(i2c->sda, data);
}

bool i2c_get(I2C* i2c)
{
    return pin_get(i2c->sda);
}

void i2c_scl(I2C* i2c, bool state)
{
    if (i2c->delay)
        i2c->delay();
    pin_set(i2c->scl, state);
}

    /*
     *  I2C Communication.
     */

void i2c_init(I2C* i2c)
{
    i2c_sda(i2c, true);
    pin_mode(i2c->scl, true);
    i2c_scl(i2c, true);
    if (i2c->trig) {
        pin_set(i2c->trig, true);
        pin_mode(i2c->trig, true);
    }

    i2c->retries = 100;
}

bool i2c_is_present(I2C* i2c)
{
    const bool ok = i2c_start(i2c, i2c->addr);
    i2c_stop(i2c);
    return ok;
}

    /*
     *  I2C Command primitives
     */

bool i2c_start(I2C* i2c, uint8_t addr)
{
    pin_pulse(i2c->trig);
    if (i2c->trig) delay_us(2);
    i2c_scl(i2c, true);
    i2c_sda(i2c, false);
    return i2c_write(i2c, addr);
}

void i2c_stop(I2C* i2c)
{
    i2c_sda(i2c, false);
    i2c_scl(i2c, true);
    i2c_sda(i2c, true);
}

bool i2c_write(I2C* i2c, uint8_t data)
{
    i2c_scl(i2c, false);
    for (uint8_t mask = 0x80; mask; mask >>= 1) {
        i2c_sda(i2c, data & mask);
        i2c_scl(i2c, true);
        i2c_scl(i2c, false);
    }
    i2c_sda(i2c, true);
    i2c_scl(i2c, true);
    const uint8_t ack = ! i2c_get(i2c);
    i2c_scl(i2c, false);
    return ack;
}

uint8_t i2c_read(I2C* i2c, bool last)
{
    uint8_t data = 0;
    for (uint8_t mask = 0x80; mask; mask >>= 1) {
        i2c_scl(i2c, true);
        if (i2c_get(i2c))
            data |= mask;
        i2c_scl(i2c, false);
    }
    i2c_sda(i2c, last);
    i2c_scl(i2c, true);
    i2c_scl(i2c, false);
    if (last)
        i2c_stop(i2c);
    else
        i2c_sda(i2c, true);
    return data;
}

    /*
     *
     */

static bool x_start(I2C* i2c, uint16_t page)
{
    page >>= 8;
    page &= 0x07;
    page <<= 1;
    const uint8_t sel = i2c->addr + page;
    //  EEPROM will return NAK while write is in progress.
    //
    //  Use polling sequence until device returns ack.
    for (int retry = 0; retry < i2c->retries; ++retry) {
        if (i2c_start(i2c, sel))
            return true;
    }
    return false;
}

    /*
     *  EEPROM load / save code
     */

bool i2c_load(I2C* i2c, uint16_t page, uint8_t offset, void* buff, int count)
{
    // Address Write
    const bool ok = x_start(i2c, page);
    i2c_write(i2c, page);
    i2c_write(i2c, offset);

    // Address Read
    i2c_start(i2c, 0x01 | i2c->addr);

    uint8_t* b = (uint8_t*) buff;
    while (--count >= 0) {
        *b++ = i2c_read(i2c, count == 0);
    }
    return ok;
}

bool i2c_save(I2C* i2c, uint16_t page, uint8_t offset, const void* buff, int count)
{
    // Address Write
    const bool ok  = x_start(i2c, page);
    i2c_write(i2c, page);
    i2c_write(i2c, offset);

    const uint8_t* b = (const uint8_t*) buff;
    while (count--) {
        i2c_write(i2c, *b++);
    }
    i2c_stop(i2c);
    return ok;
}

    /*
     *  Detect EEPROM
     */

static uint16_t probe(I2C* i2c, uint16_t lo, uint16_t hi)
{
    // do binary search on existence of EEPROM
    uint8_t c;
    const uint16_t mid = (lo + hi) / 2;
    if (mid == lo)
        return mid;
    if (i2c_load(i2c, mid, 0, & c, sizeof(c)))
        return probe(i2c, mid, hi);
    else
        return probe(i2c, lo, mid);
}

void i2c_probe(I2C* i2c, _FlashInfo* info)
{
    // reduce retries as we are using NAK fails to detect EEPROM
    const int16_t was = i2c->retries;
    i2c->retries = 1;
    info->page_size = 256;
    info->pages = probe(i2c, 0, (1024 * (1024 / 256)) - 1) + 1;
    // restore the retries count
    i2c->retries = was;
}

// FIN
