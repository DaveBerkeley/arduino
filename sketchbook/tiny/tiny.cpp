
#include <stdint.h>

#include <avr/io.h>

#undef BIN // to prevent compiler warning

#include <Arduino.h> // Arduino 1.0

#include "pins_arduino.h"

#include "RF12.h"

#if 0
    /*
     *  Provide stripped down versions of the Arduino library
     *  functions to work with the attiny84.
     */

typedef struct 
{
    volatile uint8_t* port;
    volatile uint8_t* ddr;
    uint8_t mask;
}   PortMap;

#define MAKE_PIN(port,pin) { & PORT ## port, & DDR ## port, _BV(pin) }

// pin mapping defined for http://code.google.com/p/arduino-tiny/

// ATMEL ATTINY84 / ARDUINO
//
//                          +-\/-+
//                    VCC  1|    |14  GND
//            (D  0)  PB0  2|    |13  AREF (D 10)
//            (D  1)  PB1  3|    |12  PA1  (D  9) 
//            RESET   PB3  4|    |11  PA2  (D  8) 
//  PWM  INT0 (D  2)  PB2  5|    |10  PA3  (D  7) 
//  PWM       (D  3)  PA7  6|    |9   PA4  (D  6) SCL
//  PWM  MOSI (D  4)  PA6  7|    |8   PA5  (D  5) MISO PWM
//                          +----+

#if defined(__AVR_ATtiny84__)
static const PortMap port_map[] = {
    MAKE_PIN(B, 0),  //  D0
    MAKE_PIN(B, 1),  //  D1
    MAKE_PIN(B, 2),  //  D2
    MAKE_PIN(A, 7),  //  D3
    MAKE_PIN(A, 6),  //  D4
    MAKE_PIN(A, 5),  //  D5
    MAKE_PIN(A, 4),  //  D6
    MAKE_PIN(A, 3),  //  D7
    MAKE_PIN(A, 2),  //  D8
    MAKE_PIN(A, 1),  //  D9
    MAKE_PIN(A, 0),  //  D10
};
#endif

void digitalWrite(uint8_t pin, uint8_t value)
{
    const PortMap* pm = & port_map[pin];
    if (value)
        *pm->port |= pm->mask;
    else
        *pm->port &= ~pm->mask;
}

int digitalRead(uint8_t pin)
{
    const PortMap* pm = & port_map[pin];
    return !!(*pm->port & pm->mask);
}

void pinMode(uint8_t pin, uint8_t mode)
{
    const PortMap* pm = & port_map[pin];
    if (mode == OUTPUT)
        *pm->ddr |= pm->mask;
    else
        *pm->ddr &= ~pm->mask;
}

    /*
     *  Interrupt handler.
     */

static void (*irq_fn)(void);

#define EICRA MCUCR
#define EIMSK GIMSK

#define IRQ_MODE_MASK ((1 << ISC00) | (1 << ISC01))

void attachInterrupt(uint8_t irq, void (*fn)(void), int mode)
{
    // always irq==0 with RF12 library

    const uint8_t flags = SREG;
    cli();
    irq_fn = fn;
    SREG = flags;

    EICRA = (EICRA & ~IRQ_MODE_MASK) | (mode << ISC00);
    EIMSK |= 1 << INT0;    
}

void detachInterrupt(uint8_t irq) 
{ 
    EIMSK &= ~(1 << INT0);
    if (irq == 0)
        irq_fn = 0;
}

ISR(INT0_vect)
{
    if (irq_fn)
        irq_fn();
}

    /*
     *  Note :- in the RF12 library, millis() is only used
     *  by the rf12_easyPoll() function.
     */

unsigned long millis(void)
{
    return 0;
}

#endif

    /*
     *
     */

#define TEST_PIN 8

#define GROUP_ID 6
#define NODE_ID 10
//#define BAND_868 2 // RF12_868MHZ
#define BAND_868 2 // RF12_868MHZ

int main()
{
    //  Test the rpi - tiny radio board
#if 1
    pinMode(TEST_PIN, OUTPUT);
    bool on = false;
    while (1)
    {
        digitalWrite(TEST_PIN, on = !on);
        __builtin_avr_delay_cycles(100000);
    }
    //  FIN
#endif

    sei();

#if defined(TEST_PIN)
    digitalWrite(TEST_PIN, 1);
    pinMode(TEST_PIN, OUTPUT);
#endif
    uint8_t node = NODE_ID;
    uint8_t group = 6;
    rf12_initialize(node, BAND_868, group);

    bool led_on = false;
    uint8_t dest = 31;

    while (1) 
    {
        if (rf12_canSend())
        {
            digitalWrite(TEST_PIN, 0);
            led_on = true;
            const uint8_t msg[] = { 0x11, 0x22, 0x33 };
            rf12_sendStart(dest, msg, sizeof(msg));
        }

        // must call this to drive the state machine (!!)
        if (rf12_recvDone())
        {
            //  TODO
        }

        __builtin_avr_delay_cycles(10000);
        if (led_on)
        {
            digitalWrite(TEST_PIN, 1);
            led_on = false;
        }
        __builtin_avr_delay_cycles(200000);
    }

    return 0;
}

// FIN
