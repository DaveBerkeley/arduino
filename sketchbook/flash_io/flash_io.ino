

#include <flash.h>
#include <bencode.h>
#include <radionet.h>

    /*
     *  I2C Interface.
     */

static PinIo d4 = PinIoD(4);
static PinIo a0 = PinIoC(0);

static I2C i2c = {
    & d4,   //  SDA
    & a0,   //  SCL
    0x50 << 1,
};

static FlashIO flash = { & i2c, };

static Bencode parser;
static Packet packet;

    /*
    *
    */

static void send_fn(const void* data, int length)
{
    Serial.print("send(");
    Serial.print((int)data);
    Serial.print(",");
    Serial.print(length);
    Serial.print(")\r\n");
}

static void debug_fn(const char* text)
{
    Serial.print(text);
}

static void decode_command(uint8_t* data, int length)
{
    Message msg(data);

    flash_req_handler(& flash, & msg);
}

void setup()
{
    Serial.begin(57600);

    flash_init(& flash, debug_fn, send_fn);
    packet.reset();
}

void loop()
{
    if (Serial.available()) {
        if (parser.parse(& packet, Serial.read())) {
            decode_command(packet.data, packet.length);
            parser.reset(& packet);
        }
    }
}

// FIN
