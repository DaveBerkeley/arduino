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

void delay_us(uint16_t us);
uint32_t get_ms();

    /*
     *  I2C Implementation
     */

typedef struct {
    volatile uint8_t* ddr;    // data direction register r/w (1==output)
    volatile uint8_t* data;   // data register - r/w
    volatile uint8_t* pin;    // pin register - read input state
    uint8_t mask;
}   Pin;

// Pin io functions
#if 0
void pin_mode(const Pin* pin, bool output);
void pin_set(const Pin* pin, bool state);
bool pin_get(const Pin* pin);
#endif

    /*
     *  I2C Interface
     */

typedef struct {
    Pin*    sda;
    Pin*    scl;
    Pin*    trig;
    Pin*    debug;
    uint8_t addr;
    uint16_t scl_delay;
    //uint32_t next_save;
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

void i2c_load(I2C* i2c, uint16_t page, uint8_t offset, void* buff, int count);
void i2c_save(I2C* i2c, uint16_t page, uint8_t offset, const void* buff, int count);

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

typedef void (*flash_iter)(const FlashIO* io, void* obj, uint16_t block, uint16_t offset, uint16_t bytes, uint8_t* data);

void flash_block(const FlashIO* io, void* obj, uint32_t addr, uint16_t bytes, uint8_t* data, flash_iter fn);

//  Data Transfer.

void flash_save(const FlashIO* io, uint16_t* xfered, uint32_t addr, uint16_t bytes, uint8_t* data);
void flash_read(const FlashIO* io, uint16_t* xfered, uint32_t addr, uint16_t bytes, uint8_t* data);

// FIN
