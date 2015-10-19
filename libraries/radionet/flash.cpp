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

#include "flash.h"

#include "radionet.h"

enum FLASH_CMD {
    FLASH_INFO_REQ = 1,
    FLASH_INFO = 2,
    FLASH_CLEAR = 3,
    FLASH_CLEARED = 4,
    FLASH_WRITE = 5,
    FLASH_WRITTEN = 6,
    FLASH_CRC_REQ = 7,
    FLASH_CRC = 8,
    FLASH_READ_REQ = 9,
    FLASH_READ = 10,
    FLASH_REBOOT = 11,
};

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
typedef FlashBlock FlashCrcReq;

typedef struct {
    uint8_t     cmd;
    uint16_t    block;
    uint16_t    crc;
}   FlashCrc;

typedef struct {
    uint8_t     cmd;
    uint16_t    addr;
    uint16_t    bytes;
    uint8_t     data[0];
}   FlashWrite;

typedef struct {
    uint8_t     cmd;
    uint16_t    addr;
    uint16_t    bytes;
}   FlashWritten;

    /*
     *
     */

bool flash_init()
{
    // TODO : initialise I2C flash if present
    return true;
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
    const uint8_t* payload = (uint8_t*) msg->payload();

    //  If msg is a flash request, handle it
    uint8_t cmd = 0;
    if (!msg->extract(Message::FLASH, & cmd, sizeof(cmd))) {
        return false;
    }

    switch (cmd) {
        case FLASH_INFO_REQ   : {
            FlashInfo info;

            // TODO : READ FLASH INFO
            info.cmd = FLASH_INFO;
            info.blocks = 1024;
            info.block_size = 256;

            Serial.print("flash_info_req\r\n");
            send_flash_message(& info, sizeof(info));
            break;
        }
        case FLASH_CLEAR : {
            FlashClear* fc = (FlashClear*) payload;
            FlashCleared info;

            // TODO : CLEAR FLASH
            info.cmd = FLASH_CLEARED;
            info.block = fc->block;

            Serial.print("flash_clear(");
            Serial.print(info.block);
            Serial.print(")\r\n");
            send_flash_message(& info, sizeof(info));
            break;
        }
        case FLASH_CRC_REQ : {
            FlashCrcReq* fc = (FlashCrcReq*) payload;
            FlashCrc info;

            // TODO : FLASH CRC
            info.cmd = FLASH_CRC;
            info.block = fc->block;
            info.crc = 1234;

            Serial.print("flash_crc(");
            Serial.print(info.block);
            Serial.print(",");
            Serial.print(info.crc);
            Serial.print(")\r\n");
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

            // TODO : FLASH WRITE
            info.cmd = FLASH_WRITTEN;
            info.addr = fc->addr;
            info.bytes = fc->bytes;

            Serial.print("flash_write(");
            Serial.print(info.addr);
            Serial.print(",");
            Serial.print(info.bytes);
            Serial.print(")\r\n");
            send_flash_message(& info, sizeof(info));
            break;
        }
        case FLASH_READ_REQ :
        // Don't implement these on node :
        case FLASH_CLEARED :
        case FLASH_CRC :
        case FLASH_READ :
        case FLASH_WRITTEN :
        case FLASH_INFO :
        default :   {
            Serial.print("unknown flash(");
            Serial.print(cmd);
            Serial.print(")\r\n");
            // Not implemented or unrecognised
            return false;
        }
    }

    return true;
}

// FIN
