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
#include <stdbool.h>

#include "i2c.h"

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

static void i2c_sda(I2C* i2c, bool data)
{
    // pull low, float hi
    pin_mode(i2c->sda, !data);
    pin_set(i2c->sda, data);
}

static bool i2c_get(I2C* i2c)
{
    return pin_get(i2c->sda);
}

static void i2c_scl(I2C* i2c, bool state)
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
    i2c->retries = 100;
}

    /*
     *  I2C Command primitives
     */

static void i2c_stop(I2C* i2c)
{
    i2c_sda(i2c, false);
    i2c_scl(i2c, true);
    i2c_sda(i2c, true);
}

static bool i2c_write(I2C* i2c, uint8_t data)
{
    uint8_t mask;

    i2c_scl(i2c, false);
    for (mask = 0x80; mask; mask >>= 1) {
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

static bool i2c_start(I2C* i2c, uint8_t addr)
{
    if (i2c->trig)
        i2c->trig();
    i2c_scl(i2c, true);
    i2c_sda(i2c, false);
    return i2c_write(i2c, addr);
}

static uint8_t i2c_read(I2C* i2c, bool last)
{
    uint8_t data = 0;
    uint8_t mask;

    for (mask = 0x80; mask; mask >>= 1) {
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
     *  Does the device respond to its address with an ACK?
     */

bool i2c_is_present(I2C* i2c)
{
    const bool ok = i2c_start(i2c, i2c->addr);
    i2c_stop(i2c);
    return ok;
}

    /*
     *  Repeat START,device_addr sequence until
     *  an ACK is returned.
     *
     *  EEPROM will return NAK while write is in progress.
     */

static bool x_start(I2C* i2c, uint16_t page)
{
    page >>= 8;
    page &= 0x07;
    page <<= 1;
    const uint8_t sel = i2c->addr + page;

    int retry;
    for (retry = 0; retry < i2c->retries; ++retry) {
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
     *  Block Iterator to handle Flash page boundaries.
     */

#define min(a, b) (((a) < (b)) ? (a) : (b))

void flash_block(const FlashIO* io, void* obj, uint32_t addr, uint16_t bytes, uint8_t* data, flash_iter fn)
{
    //  calculate block / offset etc.
    while (bytes) {
        const uint16_t page = addr / 256;
        const uint16_t offset = addr % 256;
        const uint16_t size = min(bytes, 256 - offset);

        if (page >= io->info.pages)
            return;

        fn(io, obj, page, offset, size, data);

        data += size;
        bytes -= size;
        addr += size;
    }
} 

    /*
     *  Read data from Flash
     */

static void reader(const FlashIO* io, void* obj, uint16_t block, uint16_t offset, uint16_t bytes, uint8_t* data)
{
    uint16_t* xfered = (uint16_t*) obj;

    if (i2c_load(io->i2c, block, offset, data, bytes))
        *xfered += bytes;
}

uint16_t flash_read(const FlashIO* io, uint32_t addr, uint16_t bytes, uint8_t* data)
{
    uint16_t xfered = 0;
    flash_block(io, & xfered, addr, bytes, data, reader);
    return xfered;
}

// FIN
