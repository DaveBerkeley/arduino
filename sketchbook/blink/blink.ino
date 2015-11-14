
//#define FLASH

#if defined(FLASH)

#include <flash.h>

    /*
     *  I2C Interface.
     */

#define TRIGGER

static PinIo d4 = PinIoD(4);
static PinIo a0 = PinIoC(0);

#if defined(TRIGGER)
static PinIo d6 = PinIoD(6);

static void trig()
{
    // trigger pulse for logic analyser
    pin_mode(& d6, true);
    pin_set(& d6, false);
    pin_set(& d6, true);
}
#endif

static I2C i2c = {
    & d4,   //  SDA
    & a0,   //  SCL
    0x50 << 1,
    0, // i2c_delay,
#if defined(TRIGGER)
    & trig,
#endif
};

static FlashIO io = {
    & i2c,
};

#endif // defined(FLASH)

#define LED 4

void setup()
{
  Serial.begin(57600);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

#if defined(FLASH)
  i2c_init(& i2c);
#endif
}

void loop()
{
  static bool on = true;
  
  digitalWrite(LED, on ? LOW : HIGH);
  on = !on;
  delay(200);

#if defined(FLASH)
  Serial.print(i2c_is_present(& i2c));
#endif
}
