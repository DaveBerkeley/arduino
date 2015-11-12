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

#include "pinio.h"

void delay_us(uint16_t us);

    /*
     *  I2C Interface
     */

typedef struct {
    PinIo*      sda;
    PinIo*      scl;
    uint8_t     addr;
    uint16_t    scl_delay;
    PinIo*      trig;
}   I2C;

// Low level I2C io functions
void i2c_sda(I2C* i2c, bool data);
bool i2c_get(I2C* i2c);
void i2c_scl(I2C* i2c, bool state);

// High level I2C functions
void i2c_init(I2C* i2c);
bool i2c_start(I2C* i2c, uint8_t addr);
void i2c_stop(I2C* i2c);
bool i2c_write(I2C* i2c, uint8_t data);
uint8_t i2c_read(I2C* i2c, bool last);
bool i2c_is_present(I2C* i2c);

    /*
     *  Interface to Flash Memory.
     */

bool i2c_load(I2C* i2c, uint16_t page, uint8_t offset, void* buff, int count);
bool i2c_save(I2C* i2c, uint16_t page, uint8_t offset, const void* buff, int count);

typedef struct {
    uint16_t    pages;
    uint16_t    page_size;
}   _FlashInfo;

typedef struct {
    I2C* i2c;
    _FlashInfo info;
}   FlashIO;

void i2c_probe(I2C* i2c, _FlashInfo* info);

// FIN
