/*
 Copyright (C) 2015 Dave Berkeley projects2@rotwang.co.uk

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 USA
*/

    /*
     *  I2C Implementation
     */

typedef struct {
    volatile uint8_t* ddr;    // data direction register r/w (1==output)
    volatile uint8_t* data;   // data register - r/w
    volatile uint8_t* pin;    // pin register - read input state
    uint8_t mask;
}   PinIo;

#define PinIoD(bit) { & DDRD, & PORTD, & PIND, 1<<(bit) }
#define PinIoC(bit) { & DDRC, & PORTC, & PINC, 1<<(bit) }
#define PinIoB(bit) { & DDRB, & PORTB, & PINB, 1<<(bit) }

static inline void pin_change(volatile uint8_t* reg, uint8_t mask, bool state)
{
    if (state)
        *reg |= mask;
    else
        *reg &= ~mask;
}

void pin_mode(const PinIo* pin, bool output);
void pin_set(const PinIo* pin, bool state);
bool pin_get(const PinIo* pin);

// FIN
