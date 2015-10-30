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

typedef struct {
    uint16_t    blocks;
    uint16_t    block_size;
}   _FlashInfo;

typedef struct {
    _FlashInfo* info;
    void (*load)(uint16_t page, uint8_t offset, void* buff, int count);
    void (*save)(uint16_t page, uint8_t offset, const void* buff, int count);
}   FlashIO;

// Block Iterator to handle Flash page boundaries.

typedef void (*b_iter)(const FlashIO* io, void* obj, uint16_t block, uint16_t offset, uint16_t bytes, uint8_t* data);

void block_iter(const FlashIO* io, void* obj, uint32_t addr, uint16_t bytes, uint8_t* data, b_iter fn);

void save(const FlashIO* io, uint16_t* xfered, uint32_t addr, uint16_t bytes, uint8_t* data);
void read(const FlashIO* io, uint16_t* xfered, uint32_t addr, uint16_t bytes, uint8_t* data, uint16_t max_size);

// FIN
