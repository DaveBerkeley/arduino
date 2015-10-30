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
#include <avr/wdt.h>

// JeeLib memory interface
#include "Ports.h"

#include "flash.h"
#include "radionet.h"
#include "i2c.h"

static MemoryPlug* mem;
static void (*flash_send_fn)(const void* data, int length) = 0;

#define ALLOW_VERBOSE

#if defined(ALLOW_VERBOSE)
static void (*debug_fn)(const char* text);

static void debug(const char* text)
{
    if (debug_fn)
        debug_fn(text);
}
#endif // ALLOW_VERBOSE

    /*
     *  Command Interface
     */

enum FLASH_CMD {
    FLASH_INFO_REQ = 1,
    FLASH_INFO = 2,
    FLASH_RECORD_REQ = 3,
    FLASH_RECORD = 4,
    FLASH_WRITE = 5,
    FLASH_WRITTEN = 6,
    FLASH_CRC_REQ = 7,
    FLASH_CRC = 8,
    FLASH_READ_REQ = 9,
    FLASH_READ = 10,
    FLASH_REBOOT = 11,
    FLASH_SET_FAST_POLL = 12,
};

typedef struct {
    uint8_t     cmd;
    _FlashInfo  info;
    uint16_t    packet_size;
}   FlashInfo;

#define MAX_DATA (52 + sizeof(FlashInfo))

typedef struct {
    uint8_t     cmd;
    uint8_t     poll;
}   FlashFastPoll;

typedef struct {
    uint8_t     cmd;
    uint32_t    addr;
    uint16_t    bytes;
}   FlashAddress;

typedef FlashAddress FlashCrcReq;
typedef FlashAddress FlashReadReq;

typedef struct {
    uint8_t     cmd;
    uint32_t    addr;
    uint16_t    bytes;
    uint16_t    crc;
}   FlashCrc;

typedef FlashCrc FlashWritten;

typedef struct {
    uint8_t     cmd;
    uint32_t    addr;
    uint16_t    bytes;
    uint8_t     data[0];
}   FlashWrite;

typedef struct {
    uint8_t     cmd;
    uint32_t    addr;
    uint16_t    bytes;
    uint8_t     data[MAX_DATA - sizeof(FlashReadReq)];
}   FlashRead;

typedef struct {
    uint8_t     cmd;
    uint8_t     slot;
}   FlashRecordReq;

typedef struct {
    uint8_t     name[8];
    uint32_t    addr;
    uint16_t    bytes;
    uint16_t    crc;
}   _FlashRecord;

typedef struct {
    uint8_t     cmd;
    uint8_t     slot;
    _FlashRecord record;
}   FlashRecord;

#define MAX_SLOTS 8

static bool fast_poll = false;

    /*
     *
     */

bool flash_fast_poll()
{
    return fast_poll;
}

    /*
     *
     */

static FlashInfo flash_info = { FLASH_INFO, };

    /*
     *  I2C Interface.
     */

static void i2c_load(uint16_t page, uint8_t offset, void* buff, int count)
{
    mem->load(page, offset, buff, count);
}

static void i2c_save(uint16_t page, uint8_t offset, const void* buff, int count)
{
    mem->save(page, offset, buff, count);
}

static FlashIO flash_io = {
    & flash_info.info,
    i2c_load,
    i2c_save,
};

    /*
     *
     */

bool flash_init(MemoryPlug* m, 
    void (*text_fn)(const char*), 
    void (*send_fn)(const void* data, int length))
{
    flash_send_fn = send_fn;
#if defined(ALLOW_VERBOSE)
    debug_fn = text_fn;
#endif
    mem = m;

    if (!mem)
        return false;

    if (mem->isPresent()) {
#if defined(ALLOW_VERBOSE)
        debug("flash_init()\r\n");
#endif
        // How to find this out from the memory device?
        flash_info.info.blocks = (128 * 1024L) / 256;
        flash_info.info.block_size = 256;
        flash_info.packet_size = sizeof(FlashRead::data);
    }

    return true;
}

    /*
     *  CRC a block of Flash
     */

//#define crc_calc _crc_xmodem_update
#define crc_calc _crc16_update

static void crcer(const FlashIO* io, void* obj, uint16_t block, uint16_t offset, uint16_t bytes, uint8_t*) {
    uint8_t buff[bytes];

    mem->load(block, offset, buff, bytes);

    uint16_t* crc = (uint16_t*) obj;
    for (uint16_t i = 0; i < bytes; ++i) {
        *crc = crc_calc(*crc, buff[i]);
    }
}

static uint16_t get_crc(const FlashIO* io, uint32_t addr, uint16_t bytes) {
    uint16_t crc = 0;
    flash_block(io, & crc, addr, bytes, 0, crcer);
    return crc;
}

    /*
     *  Send a FLASH message to the Gateway (if node)
     *  or to host (if gateway).
     */

void send_flash_message(const void* data, int length)
{
    Message msg(make_mid(), GATEWAY_ID);

    msg.append(Message::FLASH, data, length);

    if (flash_send_fn) {
        flash_send_fn(msg.data(), msg.size());
    } else {
        send_message(& msg);
    }
}

    /*
     *  Flash Message handler.
     */

bool flash_req_handler(Message* msg)
{
    uint8_t* payload = (uint8_t*) msg->payload();

    //  If msg is not a flash request, handle it elsewhere
    uint8_t cmd = 0;
    if (!msg->extract(Message::FLASH, & cmd, sizeof(cmd))) {
        return false;
    }

    switch (cmd) {
        case FLASH_INFO_REQ   : {
#if defined(ALLOW_VERBOSE)
            debug("flash_info_req()\r\n");
#endif // ALLOW_VERBOSE
            send_flash_message(& flash_info, sizeof(flash_info));
            break;
        }
        case FLASH_CRC_REQ : {
            FlashCrcReq* fc = (FlashCrcReq*) payload;
            FlashCrc info;

            info.cmd = FLASH_CRC;
            info.addr = fc->addr;
            info.bytes = fc->bytes;
            info.crc = get_crc(& flash_io, fc->addr, fc->bytes);

#if defined(ALLOW_VERBOSE)
            if (debug_fn) {
                char buff[32];
                snprintf(buff, sizeof(buff), "flash_crc(%ld,%d,%X)\r\n", info.addr, info.bytes, info.crc);
                debug(buff);
            }
#endif // ALLOW_VERBOSE

            send_flash_message(& info, sizeof(info));
            break;
        }
        case FLASH_REBOOT   : {
#if defined(ALLOW_VERBOSE)
            debug("flash_reboot()\r\n");
#endif // ALLOW_VERBOSE
            wdt_enable(WDTO_15MS);
            noInterrupts();
            while (true) ;
            break;
        }
        case FLASH_WRITE : {
            FlashWrite* fc = (FlashWrite*) payload;
            FlashWritten info;

            info.cmd = FLASH_WRITTEN;
            info.addr = fc->addr;
            info.bytes = 0;

            // Do the write
            flash_save(& flash_io, & info.bytes, fc->addr, fc->bytes, fc->data);
            // CRC the EEPROM that we've just written
            info.crc = get_crc(& flash_io, fc->addr, fc->bytes);            

#if defined(ALLOW_VERBOSE)
            if (debug_fn) {
                char buff[24];
                snprintf(buff, sizeof(buff), "flash_write(%ld,%d)\r\n", info.addr, info.bytes);
                debug(buff);
            }
#endif // ALLOW_VERBOSE

            send_flash_message(& info, sizeof(info));
            break;
        }
        case FLASH_READ_REQ : {
            FlashReadReq* fc = (FlashReadReq*) payload;
            FlashRead info;

            // FLASH READ
            info.cmd = FLASH_READ;
            info.addr = fc->addr;
            info.bytes = 0;

            flash_read(& flash_io, & info.bytes, fc->addr, min(fc->bytes, sizeof(info.data)), info.data);
    
#if defined(ALLOW_VERBOSE)
            if (debug_fn) {
                char buff[32];
                snprintf(buff, sizeof(buff), 
                        "flash_read_req(%ld,%d)\r\n",
                        info.addr,
                        info.bytes);
                debug(buff);
            }
#endif // ALLOW_VERBOSE

            send_flash_message(& info, sizeof(FlashReadReq) + info.bytes);
            break;
        }
        case FLASH_SET_FAST_POLL : {
            FlashFastPoll* fc = (FlashFastPoll*) payload;
            fast_poll = fc->poll;
#if defined(ALLOW_VERBOSE)
            if (debug_fn) {
                char buff[32];
                snprintf(buff, sizeof(buff), "set_fast_poll(%d)\r\n", fast_poll);
                debug(buff);
            }
#endif // ALLOW_VERBOSE
            break;
        }
        case FLASH_RECORD_REQ : {
            FlashRecordReq* fc = (FlashRecordReq*) payload;

#if defined(ALLOW_VERBOSE)
            if (debug_fn) {
                char buff[32];
                snprintf(buff, sizeof(buff), "flash_record_req(%d)\r\n", fc->slot);
                debug(buff);
            }
#endif // ALLOW_VERBOSE

            FlashRecord info;

            info.cmd = FLASH_RECORD;
            info.slot = fc->slot;

            const uint16_t size = sizeof(info.record);
            const uint32_t offset = fc->slot * size;
            uint16_t xferred = 0;
            // Read Record
            flash_read(& flash_io, & xferred, offset, size, (uint8_t*) & info.record);
            send_flash_message(& info, sizeof(info));
            break;
        }
        case FLASH_RECORD : {
            FlashRecord* fc = (FlashRecord*) payload;
#if defined(ALLOW_VERBOSE)
            if (debug_fn) {
                char buff[32];
                snprintf(buff, sizeof(buff), "flash_record(%d)\r\n", fc->slot);
                debug(buff);
            }
#endif // ALLOW_VERBOSE
            // Write Record
            const uint16_t size = sizeof(fc->record);
            const uint32_t offset = fc->slot * size;
            FlashWritten info;

            info.cmd = FLASH_WRITTEN;
            info.addr = offset;
            info.bytes = 0;

            // Do the write
            if (fc->slot <= MAX_SLOTS) {
                flash_save(& flash_io, & info.bytes, offset, size, (uint8_t*) & fc->record);
                // CRC the EEPROM that we've just written
                info.crc = get_crc(& flash_io, offset, size);            
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
#if defined(ALLOW_VERBOSE)
            if (debug_fn) {
                char buff[32];
                snprintf(buff, sizeof(buff), "flash(%d)\r\n", cmd);
                debug(buff);
            }
#endif // ALLOW_VERBOSE
            // Not implemented or unrecognised
            return false;
        }
    }

    return true;
}

// FIN
