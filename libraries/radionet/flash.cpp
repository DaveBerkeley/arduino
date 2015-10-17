
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
    FLASH_INFO,
    FLASH_CLEAR,
    FLASH_CLEARED,
    FLASH_WRITE,
    FLASH_WRITTEN,
    FLASH_CRC_REQ,
    FLASH_CRC,
    FLASH_READ_REQ,
    FLASH_READ,
    FLASH_REBOOT,
};

typedef struct {
    FLASH_CMD   cmd;
}   FlashHeader;

typedef struct {
    FlashHeader hdr;
    uint16_t    blocks;
    uint16_t    block_size;
}   FlashInfo;

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

void send_message(const void* data, int length)
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
    //  If msg is a flash request, handle it
    if (!(msg->get_flags() & Message::FLASH))
        return false;

    const FlashHeader* hdr = (FlashHeader*) msg->payload();    

    switch (hdr->cmd) {
        case FLASH_REBOOT   : {
            //  TODO
            break;
        }
        case FLASH_INFO_REQ   : {
            FlashInfo info;

            info.hdr.cmd = FLASH_INFO;
            info.blocks = 1024;
            info.block_size = 256;

            send_message(& info, sizeof(info));
            break;
        }
        case FLASH_CLEAR :
        case FLASH_WRITE :
        case FLASH_CRC_REQ :
        case FLASH_READ_REQ :
        // Don't implement these on node :
        case FLASH_CLEARED :
        case FLASH_CRC :
        case FLASH_READ :
        case FLASH_WRITTEN :
        case FLASH_INFO :
        default :   {
            // Not implemented or unrecognised
            return false;
        }
    }

    return true;
}

// FIN
