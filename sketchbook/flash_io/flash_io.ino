

#include <flash.h>

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
    
static FlashIO io = {
    & i2c,
};

void setup()
{
    Serial.begin(57600);

    i2c_init(& i2c);
}

void loop()
{
    Serial.print(i2c_is_present(& i2c));
    delay(100);
}
