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

void block_iter(const _FlashInfo* info, void* obj, uint32_t addr, uint16_t bytes, uint8_t* data, b_iter fn)
{
    //  calculate block / offset etc.
    while (bytes) {
        const uint16_t bsize = info->block_size;
        const uint16_t block = addr / bsize;
        const uint16_t offset = addr % bsize;
        const uint16_t size = min(bytes, bsize - offset);

        if (block >= info->blocks)
            return;

        fn(obj, block, offset, size, data);

        data += size;
        bytes -= size;
        addr += size;
    }
} 

// FIN
