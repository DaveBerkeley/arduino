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

#include <util/crc16.h>
#include <avr/wdt.h>
#include <stdio.h>

#include "flash.h"
#include "radioutils.h"

static void (*flash_send_fn)(const void* data, int length) = 0;

#define ALLOW_VERBOSE

#if defined(ALLOW_VERBOSE)
static void (*debug_fn)(const char* text);

static void debug(const char* text)
{
    if (debug_fn)
        debug_fn(text);
}
#else
#define debug(x)
#endif // ALLOW_VERBOSE

    /*
     *  Command Interface
     */

enum FLASH_CMD {
    FLASH_INFO_REQ = 1,
    FLASH_INFO = 2,
    FLASH_SLOT_REQ = 3,
    FLASH_SLOT = 4,
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
    uint8_t     req_id;
}   FlashInfoReq;

typedef struct {
    uint8_t     cmd;
    uint8_t     req_id;
    uint16_t    pages;
    uint16_t    page_size;
    uint16_t    packet_size;
}   FlashInfo;

#define MAX_DATA (54 + sizeof(FlashInfo))

typedef struct {
    uint8_t     cmd;
    uint8_t     req_id;
    uint8_t     poll;
}   FlashFastPoll;

typedef struct {
    uint8_t     cmd;
    uint8_t     req_id;
    uint32_t    addr;
    uint16_t    bytes;
}   FlashAddress;

typedef FlashAddress FlashCrcReq;
typedef FlashAddress FlashReadReq;

typedef struct {
    uint8_t     cmd;
    uint8_t     req_id;
    uint32_t    addr;
    uint16_t    bytes;
    uint16_t    crc;
}   FlashCrc;

typedef FlashCrc FlashWritten;

typedef struct {
    uint8_t     cmd;
    uint8_t     req_id;
    uint32_t    addr;
    uint16_t    bytes;
    uint8_t     data[0];
}   FlashWrite;

typedef struct {
    uint8_t     cmd;
    uint8_t     req_id;
    uint32_t    addr;
    uint16_t    bytes;
    uint8_t     data[MAX_DATA - sizeof(FlashReadReq)];
}   FlashRead;

typedef struct {
    uint8_t     cmd;
    uint8_t     req_id;
    uint8_t     slot;
}   FlashSlotReq;

typedef struct {
    uint8_t     cmd;
    uint8_t     req_id;
    uint8_t     slot;
    _FlashSlot entry;
}   FlashSlot;

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
     *  Detect EEPROM
     */

static uint16_t probe(I2C* i2c, uint16_t max_page)
{
    uint8_t c;
    uint16_t page;

    for (page = 0; page <= max_page; page += 0x100)
        if (!i2c_load(i2c, page, 0, & c, sizeof(c)))
            break;
    return page;
}

static void flash_probe(FlashIO* io)
{
    // reduce retries as we are using NAK fails to detect EEPROM
    const int16_t was = io->i2c->retries;
    io->i2c->retries = 1;
    io->info.page_size = 256;
    const uint16_t max_pages = 1024 * (1024 / 256); // 1MB
    io->info.pages = probe(io->i2c, max_pages);
    // restore the retries count
    io->i2c->retries = was;
}

    /*
     *
     */

bool flash_init(FlashIO* io, 
    void (*text_fn)(const char*), 
    void (*send_fn)(const void* data, int length))
{
    flash_send_fn = send_fn;
#if defined(ALLOW_VERBOSE)
    debug_fn = text_fn;
#endif

    i2c_init(io->i2c);

    if (i2c_is_present(io->i2c)) {
        // Probe memory device to find its size.
        flash_probe(io);
#if defined(ALLOW_VERBOSE)
        if (debug_fn) {
            char buff[32];
            snprintf(buff, sizeof(buff), 
                    "flash_init(%d,%d)\r\n", 
                    io->info.pages,
                    io->info.page_size);
            debug(buff);
        }
#endif
    }

    return true;
}

    /*
     *  Save data to Flash
     */

static void saver(const FlashIO* io, void* obj, uint16_t block, uint16_t offset, uint16_t bytes, uint8_t* data)
{
    uint16_t* xfered = (uint16_t*) obj;

    if (i2c_save(io->i2c, block, offset, data, bytes))
        *xfered += bytes;
}

uint16_t flash_save(const FlashIO* io, uint32_t addr, uint16_t bytes, uint8_t* data)
{
    uint16_t xfered = 0;
    flash_block(io, & xfered, addr, bytes, data, saver);
    return xfered;
}

    /*
     *  CRC a block of Flash
     */

//#define crc_calc _crc_xmodem_update
#define crc_calc _crc16_update

static void crcer(const FlashIO* io, void* obj, uint16_t block, uint16_t offset, uint16_t bytes, uint8_t*) {
    uint8_t buff[bytes];

    i2c_load(io->i2c, block, offset, buff, bytes);

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
     *  Send a FLASH message using provided callback.
     */

void send_flash_message(const void* data, int length)
{
    if (flash_send_fn) {
        flash_send_fn(data, length);
    } else {
        debug("no send_fn()\r\n");
    }
}

    /*
     *  Handlers for individual Flash messages.
     */

static void flash_info_req(FlashIO* io, FlashInfoReq* fc)
{

#if defined(ALLOW_VERBOSE)
    if (debug_fn) {
        char buff[32];
        snprintf(buff, sizeof(buff), 
                "flash_info_req(r=%d)\r\n", 
                (int) fc->req_id);
        debug(buff);
    }
#endif // ALLOW_VERBOSE

    FlashInfo fi;

    fi.pages = io->info.pages;
    fi.page_size = io->info.page_size;
    fi.cmd = FLASH_INFO;
    fi.req_id = fc->req_id;
    fi.packet_size = sizeof(FlashRead::data);
    send_flash_message(& fi, sizeof(fi));
}

    /*
     *
     */

static void flash_slot_req(FlashIO* io, FlashSlotReq* fc)
{

#if defined(ALLOW_VERBOSE)
    if (debug_fn) {
        char buff[32];
        snprintf(buff, sizeof(buff), 
                "flash_record_req(r=%d,%d)\r\n", 
                (int) fc->req_id,
                fc->slot);
        debug(buff);
    }
#endif // ALLOW_VERBOSE

    FlashSlot info;

    info.cmd = FLASH_SLOT;
    info.req_id = fc->req_id;
    info.slot = fc->slot;

    const uint16_t size = sizeof(info.entry);
    const uint32_t offset = fc->slot * size;
    // Read Record
    flash_read(io, offset, size, (uint8_t*) & info.entry);
    send_flash_message(& info, sizeof(info));
}

    /*
     *
     */

static void flash_crc_req(FlashIO* io, FlashCrcReq* fc)
{
    FlashCrc info;

    info.cmd = FLASH_CRC;
    info.req_id = fc->req_id;
    info.addr = fc->addr;
    info.bytes = fc->bytes;

#if defined(ALLOW_VERBOSE)
    if (debug_fn) {
        char buff[32];
        snprintf(buff, sizeof(buff), 
                "flash_crc(r=%d,%ld,%d,", 
                (int) fc->req_id,
                info.addr, info.bytes);
        debug(buff);
    }
#endif // ALLOW_VERBOSE

    info.crc = get_crc(io, fc->addr, fc->bytes);

#if defined(ALLOW_VERBOSE)
    if (debug_fn) {
        char buff[32];
        snprintf(buff, sizeof(buff), "%X)\r\n", info.crc);
        debug(buff);
    }
#endif // ALLOW_VERBOSE

    send_flash_message(& info, sizeof(info));
}

    /*
     *
     */

#define min(a, b) (((a) < (b)) ? (a) : (b))

static void flash_read_req(FlashIO* io, FlashReadReq* fc)
{
    FlashRead info;

    // FLASH READ
    info.cmd = FLASH_READ;
    info.req_id = fc->req_id;
    info.addr = fc->addr;

    info.bytes = flash_read(io, fc->addr, min(fc->bytes, sizeof(info.data)), info.data);

#if defined(ALLOW_VERBOSE)
    if (debug_fn) {
        char buff[32];
        snprintf(buff, sizeof(buff), 
                "flash_read_req(r=%d,%ld,%d)\r\n",
                (int) info.req_id,
                info.addr,
                info.bytes);
        debug(buff);
    }
#endif // ALLOW_VERBOSE

    send_flash_message(& info, sizeof(FlashReadReq) + info.bytes);
}

    /*
     *
     */

static void flash_slot(FlashIO* io, FlashSlot* fc)
{
#if defined(ALLOW_VERBOSE)
    if (debug_fn) {
        char buff[32];
        snprintf(buff, sizeof(buff), 
                "flash_record(r=%d,%d)\r\n", 
                (int) fc->req_id,
                fc->slot);
        debug(buff);
    }
#endif // ALLOW_VERBOSE
    // Write Record
    const uint16_t size = sizeof(fc->entry);
    const uint32_t offset = fc->slot * size;
    FlashWritten info;

    info.cmd = FLASH_WRITTEN;
    info.req_id = fc->req_id;
    info.addr = offset;
    info.bytes = 0;

    // Do the write
    if (fc->slot <= MAX_SLOTS) {
        info.bytes = flash_save(io, offset, size, (uint8_t*) & fc->entry);
        // CRC the EEPROM that we've just written
        info.crc = get_crc(io, offset, size);            
    }
    send_flash_message(& info, sizeof(info));
}

    /*
     *
     */

static void flash_write(FlashIO* io, FlashWrite* fc)
{
    FlashWritten info;

    info.cmd = FLASH_WRITTEN;
    info.req_id = fc->req_id;
    info.addr = fc->addr;
    info.bytes = 0;

    // Do the write
    info.bytes = flash_save(io, fc->addr, fc->bytes, fc->data);

#if defined(ALLOW_VERBOSE)
    if (debug_fn) {
        char buff[24];
        snprintf(buff, sizeof(buff), 
                "flash_write(r=%d,%ld,%d)\r\n", 
                (int) info.req_id,
                info.addr, 
                info.bytes);
        debug(buff);
    }
#endif // ALLOW_VERBOSE

    // CRC the EEPROM that we've just written
    info.crc = get_crc(io, fc->addr, fc->bytes);            

    send_flash_message(& info, sizeof(info));
}

    /*
     *  Flash Message handler.
     */

void flash_req_handler(FlashIO* io, uint8_t cmd, uint8_t* payload)
{
    switch (cmd) {
        case FLASH_INFO_REQ   : {
            flash_info_req(io, (FlashInfoReq*) payload);
            break;
        }
        case FLASH_CRC_REQ : {
            flash_crc_req(io, (FlashCrcReq*) payload);
            break;
        }
        case FLASH_REBOOT   : {
#if defined(ALLOW_VERBOSE)
            debug("flash_reboot()\r\n");
#endif // ALLOW_VERBOSE
            reboot();
            break;
        }
        case FLASH_WRITE : {
            flash_write(io, (FlashWrite*) payload);
            break;
        }
        case FLASH_READ_REQ : {
            flash_read_req(io, (FlashReadReq*) payload);
            break;
        }
        case FLASH_SET_FAST_POLL : {
            FlashFastPoll* fc = (FlashFastPoll*) payload;
            fast_poll = fc->poll;
#if defined(ALLOW_VERBOSE)
            if (debug_fn) {
                char buff[32];
                snprintf(buff, sizeof(buff), 
                        "set_fast_poll(r=%d,%d)\r\n", 
                        (int) fc->req_id,
                        fast_poll);
                debug(buff);
            }
#endif // ALLOW_VERBOSE
            break;
        }
        case FLASH_SLOT_REQ : {
             flash_slot_req(io, (FlashSlotReq*) payload);
             break;
        }

        case FLASH_SLOT : {
            flash_slot(io, (FlashSlot*) payload);
            break;
        }

        // Don't implement these on node :
        case FLASH_CRC :
        case FLASH_READ :
        case FLASH_WRITTEN :
        case FLASH_INFO :
        default :   {
            // Not implemented or unrecognised.
            // Silently ignore
#if defined(ALLOW_VERBOSE)
            if (debug_fn) {
                char buff[32];
                snprintf(buff, sizeof(buff), "flash(%d)\r\n", cmd);
                debug(buff);
            }
#endif // ALLOW_VERBOSE
        }
    }
}

// FIN
