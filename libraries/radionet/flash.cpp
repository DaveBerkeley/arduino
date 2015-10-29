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
    uint16_t    blocks;
    uint16_t    block_size;
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

#if defined(ALLOW_VERBOSE)
void show_block(const uint8_t* data, uint16_t len)
{
    for (uint16_t i = 0; i < len; ++i) {
        if (i && !(i % 16))
            debug("\r\n");
        char buff[8];
        snprintf(buff, sizeof(buff), "%02X ", data[i]);
        debug(buff);
    }
    debug("\r\n");
}
#else
#define show_block(x, y)
#endif

static FlashInfo flash_info = { FLASH_INFO, };

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
        flash_info.blocks = (128 * 1024L) / 256;
        flash_info.block_size = 256;
        flash_info.packet_size = sizeof(FlashRead::data);
    }

    return true;
}

    /*
     *  Block Iterator to handle Flash page boundaries.
     */

typedef void (*b_iter)(void* obj, uint16_t block, uint16_t offset, uint16_t bytes, uint8_t* data);

static void block_iter(void* obj, uint32_t addr, uint16_t bytes, uint8_t* data, b_iter fn)
{
    //  calculate block / offset etc.
    while (bytes) {
        const uint16_t bsize = flash_info.block_size;
        const uint16_t block = addr / bsize;
        const uint16_t offset = addr % bsize;
        const uint16_t size = min(bytes, bsize - offset);

        if (block >= flash_info.blocks)
            return;

        fn(obj, block, offset, size, data);

        data += size;
        bytes -= size;
        addr += size;
    }
} 

    /*
     *  Save data to Flash
     */

static void saver(void* obj, uint16_t block, uint16_t offset, uint16_t bytes, uint8_t* data)
{
    uint16_t* xfered = (uint16_t*) obj;

#if defined(ALLOW_VERBOSE)
    if (debug_fn) {
        char buff[24];
        snprintf(buff, sizeof(buff), "save(%d,%d,%d)\r\n", block, offset, bytes);
        debug(buff);
    }
#endif // ALLOW_VERBOSE

    mem->save(block, offset, data, bytes);
    *xfered += bytes;
}

static void save(uint16_t* xfered, uint32_t addr, uint16_t bytes, uint8_t* data)
{
    //show_block(data, bytes);
    block_iter(xfered, addr, bytes, data, saver);
}

    /*
     *  CRC a block of Flash
     */

//#define crc_calc _crc_xmodem_update
#define crc_calc _crc16_update

static void crcer(void* obj, uint16_t block, uint16_t offset, uint16_t bytes, uint8_t*) {
    uint8_t buff[bytes];

#if defined(ALLOW_VERBOSE)
    if (debug_fn) {
        char buff[24];
        snprintf(buff, sizeof(buff), "crcer(%d,%d,%d)\r\n", block, offset, bytes);
        debug(buff);
    }
#endif // ALLOW_VERBOSE

    mem->load(block, offset, buff, bytes);

    uint16_t* crc = (uint16_t*) obj;
    for (uint16_t i = 0; i < bytes; ++i) {
        *crc = crc_calc(*crc, buff[i]);
    }
}

static uint16_t get_crc(uint32_t addr, uint16_t bytes) {
    uint16_t crc = 0;
    block_iter(& crc, addr, bytes, 0, crcer);
    return crc;
}

    /*
     *  Read data from Flash
     */

static void reader(void* obj, uint16_t block, uint16_t offset, uint16_t bytes, uint8_t* data)
{
    uint16_t* xfered = (uint16_t*) obj;

#if defined(ALLOW_VERBOSE)
    if (debug_fn) {
        char buff[24];
        snprintf(buff, sizeof(buff), "read(%d,%d,%d)\r\n", block, offset, bytes);
        debug(buff);
    }
#endif // ALLOW_VERBOSE

    mem->load(block, offset, data, bytes);
    //show_block(data, bytes);
    *xfered += bytes;
}

static void read(uint16_t* xfered, uint32_t addr, uint16_t bytes, uint8_t* data, uint16_t max_size)
{
    block_iter(xfered, addr, min(bytes, max_size), data, reader);
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
            info.crc = get_crc(fc->addr, fc->bytes);

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
            save(& info.bytes, fc->addr, fc->bytes, fc->data);
            // CRC the EEPROM that we've just written
            info.crc = get_crc(fc->addr, fc->bytes);            

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

            read(& info.bytes, fc->addr, fc->bytes, info.data, sizeof(info.data));
    
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
            read(& xferred, offset, size, (uint8_t*) & info.record, size);
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
                save(& info.bytes, offset, size, (uint8_t*) & fc->record);
                // CRC the EEPROM that we've just written
                info.crc = get_crc(offset, size);            
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
