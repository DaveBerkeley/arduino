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

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include <pins_arduino.h>
#endif

#include <util/crc16.h>

// JeeLib memory interface
#include "Ports.h"

#include "flash.h"
#include "radionet.h"

static MemoryPlug* mem;
static bool verbose;

    /*
     *  Command Interface
     */

enum FLASH_CMD {
    FLASH_INFO_REQ = 1,
    FLASH_INFO = 2,
    //FLASH_CLEAR = 3,
    //FLASH_CLEARED = 4,
    FLASH_WRITE = 5,
    FLASH_WRITTEN = 6,
    FLASH_CRC_REQ = 7,
    FLASH_CRC = 8,
    FLASH_READ_REQ = 9,
    FLASH_READ = 10,
    FLASH_REBOOT = 11,
};

#define MAX_DATA 128

typedef struct {
    uint8_t     cmd;
    uint16_t    blocks;
    uint16_t    block_size;
}   FlashInfo;

typedef struct {
    uint8_t     cmd;
    uint16_t    block;
}   FlashBlock;

typedef FlashBlock FlashClear;
typedef FlashBlock FlashCleared;

typedef struct {
    uint8_t     cmd;
    uint16_t    addr;
    uint16_t    bytes;
}   FlashAddress;

typedef struct {
    uint8_t     cmd;
    uint16_t    addr;
    uint16_t    bytes;
    uint16_t    crc;
}   FlashCrc;

typedef FlashAddress FlashCrcReq;

typedef struct {
    uint8_t     cmd;
    uint16_t    addr;
    uint16_t    bytes;
    uint8_t     data[0];
}   FlashWrite;

typedef FlashAddress FlashWritten;
typedef FlashAddress FlashReadReq;

typedef struct {
    uint8_t     cmd;
    uint16_t    addr;
    uint16_t    bytes;
    uint8_t     data[MAX_DATA - sizeof(FlashReadReq)];
}   FlashRead;

    /*
     *
     */

void show_block(const uint8_t* data, uint16_t len)
{
    for (uint16_t i = 0; i < len; ++i) {
        if (i && !(i % 16))
            Serial.print("\r\n");
        char buff[8];
        snprintf(buff, sizeof(buff), "%02X ", data[i]);
        Serial.print(buff);
    }
    Serial.print("\r\n");
}

static FlashInfo flash_info = { FLASH_INFO, };

bool flash_init(MemoryPlug* m, bool v)
{
    verbose = v;
    mem = m;

    if (!mem)
        return false;

    if (!mem->isPresent())
        return false;
 
    if (verbose) {
        Serial.print("flash_init()\r\n");

        uint8_t buff[64];
        mem->load(0, 0, buff, sizeof(buff));
        show_block(buff, sizeof(buff));
    }

    flash_info.blocks = (128 * 1024L) / 256;
    flash_info.block_size = 256;

    return true;
}

// save(page, offset, bytes, count);
static void save(uint16_t addr, uint16_t bytes, uint8_t* data)
{
    //  TODO : calculate block / offset etc.
    show_block(data, bytes);
    mem->save(0, addr, data, bytes);
}

static uint16_t get_crc(uint16_t addr, uint16_t bytes) {
    uint16_t crc = 0;

    while (bytes--) {
        uint8_t c;
        // TODO : calculate block offsets etc.
        mem->load(0, addr++, & c, 1);
        crc = _crc_xmodem_update(crc, c);
    }
    return crc;
}

    /*
     *
     */

void send_flash_message(const void* data, int length)
{
    Message msg(make_mid(), GATEWAY_ID);

    msg.append(Message::FLASH, data, length);
    send_message(& msg);
}

    /*
     *
     */

bool flash_req_handler(Message* msg)
{
    //  Is there any Flash attached?    
    if (!flash_info.blocks)
        return false;

    uint8_t* payload = (uint8_t*) msg->payload();

    //  If msg is not a flash request, handle it elsewhere
    uint8_t cmd = 0;
    if (!msg->extract(Message::FLASH, & cmd, sizeof(cmd))) {
        return false;
    }

    switch (cmd) {
        case FLASH_INFO_REQ   : {
            if (verbose) {
                Serial.print("flash_info_req()\r\n");
            }
            send_flash_message(& flash_info, sizeof(flash_info));
            break;
        }
        case FLASH_CRC_REQ : {
            FlashCrcReq* fc = (FlashCrcReq*) payload;
            FlashCrc info;

            info.cmd = FLASH_CRC;
            info.addr = fc->addr;
            info.bytes = fc->bytes;
            info.crc = get_crc(fc->addr, fc->bytes);

            if (verbose) {
                char buff[32];
                snprintf(buff, sizeof(buff), "flash_crc(%d,%d,%X)\r\n", info.addr, info.bytes, info.crc);
                Serial.print(buff);
            }

            send_flash_message(& info, sizeof(info));
            break;
        }
        case FLASH_REBOOT   : {
            Serial.print("flash_reboot()\r\n");
            // TODO : REBOOT
            break;
        }
        case FLASH_WRITE : {
            FlashWrite* fc = (FlashWrite*) payload;
            FlashWritten info;

            // Do the write
            save(fc->addr, fc->bytes, fc->data);

            info.cmd = FLASH_WRITTEN;
            info.addr = fc->addr;
            info.bytes = fc->bytes;

            if (verbose) {
                char buff[24];
                snprintf(buff, sizeof(buff), "flash_write(%d,%d)\r\n", info.addr, info.bytes);
                Serial.print(buff);
            }

            send_flash_message(& info, sizeof(info));
            break;
        }
        case FLASH_READ_REQ : {
            FlashReadReq* fc = (FlashReadReq*) payload;
            FlashRead info;

            // TODO : FLASH READ
            info.cmd = FLASH_READ;
            info.addr = fc->addr;
            info.bytes = fc->bytes;

            if (verbose) {
                Serial.print("flash_read_req(");
                Serial.print(info.addr);
                Serial.print(",");
                Serial.print(info.bytes);
                Serial.print(")\r\n");
            }

            send_flash_message(& info, sizeof(info));
            break;
        }
        // Don't implement these on node :
        case FLASH_CRC :
        case FLASH_READ :
        case FLASH_WRITTEN :
        case FLASH_INFO :
        default :   {
            if (verbose) {
                Serial.print("unknown flash(");
                Serial.print(cmd);
                Serial.print(")\r\n");
            }
            // Not implemented or unrecognised
            return false;
        }
    }

    return true;
}

// FIN
