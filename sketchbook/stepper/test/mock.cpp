
#include <Arduino.h>

#define UNUSED(x) ((x) = (x))

void pinMode(int pin, int mode)
{
    UNUSED(pin);
    UNUSED(mode);
}

void digitalWrite(int pin, int value)
{
    UNUSED(pin);
    UNUSED(value);
}

void delayMicroseconds(int us)
{
    UNUSED(us);
}

//  FIN
