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
     *  Block Iterator to handle Flash page boundaries.
     */

void flash_block(const FlashIO* io, void* obj, uint32_t addr, uint16_t bytes, uint8_t* data, flash_iter fn)
{
    //  calculate block / offset etc.
    while (bytes) {
        const uint16_t bsize = io->info->block_size;
        const uint16_t block = addr / bsize;
        const uint16_t offset = addr % bsize;
        const uint16_t size = min(bytes, bsize - offset);

        if (block >= io->info->blocks)
            return;

        fn(io, obj, block, offset, size, data);

        data += size;
        bytes -= size;
        addr += size;
    }
} 

    /*
     *  Save data to Flash
     */

static void saver(const FlashIO* io, void* obj, uint16_t block, uint16_t offset, uint16_t bytes, uint8_t* data)
{
    uint16_t* xfered = (uint16_t*) obj;

    io->save(block, offset, data, bytes);
    *xfered += bytes;
}

void flash_save(const FlashIO* io, uint16_t* xfered, uint32_t addr, uint16_t bytes, uint8_t* data)
{
    flash_block(io, xfered, addr, bytes, data, saver);
}

    /*
     *  Read data from Flash
     */

static void reader(const FlashIO* io, void* obj, uint16_t block, uint16_t offset, uint16_t bytes, uint8_t* data)
{
    uint16_t* xfered = (uint16_t*) obj;

    io->load(block, offset, data, bytes);
    *xfered += bytes;
}

void flash_read(const FlashIO* io, uint16_t* xfered, uint32_t addr, uint16_t bytes, uint8_t* data)
{
    flash_block(io, xfered, addr, bytes, data, reader);
}

// FIN
