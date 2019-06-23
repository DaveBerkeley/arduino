
#include <math.h>

#define UNUSED(x) ((x) = (x))
#define ASSERT(x) assert(x)

void pinMode(int pin, int mode);

enum {
    INPUT = 1,
    OUTPUT
};

void digitalWrite(int pin, int value);

void delayMicroseconds(int us);

// FIN
