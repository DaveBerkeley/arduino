
#include <math.h>
#include <stdio.h>

#define PIN_MIN 0
#define PIN_MAX 14

void pinMode(int pin, int mode);

enum {
    INPUT = 1,
    OUTPUT,
    INPUT_PULLUP
};

enum {
    LOW = 0,
    HIGH
};

void digitalWrite(int pin, int value);
bool digitalRead(int pin);

void delayMicroseconds(int us);
void delay(int ms);
int millis();

// FIN
