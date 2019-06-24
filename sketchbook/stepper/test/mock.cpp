
#include <gtest/gtest.h>

#include "Arduino.h"
#include "mock.h"

static int pins[PIN_MAX];

void mock_setup()
{
    memset(pins, 0, sizeof(pins));
}

void mock_teardown()
{
}

void pinMode(int pin, int mode)
{
    ASSERT(pin >= PIN_MIN);
    ASSERT(pin < PIN_MAX);
    ASSERT((mode == INPUT) || (mode == OUTPUT) || (mode == INPUT_PULLUP));
}

void digitalWrite(int pin, int value)
{
    ASSERT(pin >= PIN_MIN);
    ASSERT(pin < PIN_MAX);
    pins[pin] = value ? 1 : 0;
}

bool pins_match(int num, int start, const int *p)
{
    //ASSERT(start >= PIN_MIN);
    //ASSERT((start + num) < PIN_MAX);
    UNUSED(start);
    UNUSED(num);

    for (int i = 0; i < num; i++)
    {
        if (pins[i+start] != p[i])
        {
            printf("%d: %d %d\n", i, pins[i+start], p[i]);
            return false;
        }
    }

    return true;
}

void delayMicroseconds(int us)
{
    UNUSED(us);
}

//  FIN
