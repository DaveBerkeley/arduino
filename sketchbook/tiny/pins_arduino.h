
#ifndef Pins_Arduino_h
#define Pins_Arduino_h

    /*
     *  Provide hardware definitions used by RF12 code.
     */

#if 0

// attiny85 ...
//
// Define the PORTB pins used to talk to the radio
#define SS_DDR      DDRB
#define SS_PORT     PORTB
#define SS_BIT      3

// Normally the Arduino pins, but we provide the io functions
#define SPI_SS      3
#define SPI_MISO    1
#define SPI_MOSI    0
#define SPI_SCK     2
#define RFM_IRQ     4
#endif

#define RFM_IRQ     2
#define SS_DDR      DDRB
#define SS_PORT     PORTB
#define SS_BIT      1

#define SPI_SS      1     // PB1, pin 3
#define SPI_MISO    4     // PA6, pin 7
#define SPI_MOSI    5     // PA5, pin 8
#define SPI_SCK     6     // PA4, pin 9

    /*
     *  Handy place to subvert Arduino libraries
     */

class __Serial
{
public:
    void print(char p);
    void println();
};

extern __Serial Serial;

#endif
