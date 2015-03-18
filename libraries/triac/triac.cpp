 
/*
 Arduino / JeeNode library to phase control a triac.

 Copyright (C) 2012 Dave Berkeley projects@rotwang.co.uk

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

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include <pins_arduino.h>
#endif

#include <triac.h>

 /*
  * The timer is set to ck/8 prescalar. At 16MHz this gives
  * 500us per count. With 50Hz mains, half cycle is 10ms long.
  * Therefore 20000 timer counts (10000us) equals one half cycle.
  */

#define COUNTS(n) ((n) << 1) // convert us to counts

/*
  *  Timer / counter config
  */

static void clear_timer()
{
    TIMSK1 = 0; // no interrupts
    TCCR1A = 0; // counter off
    TCCR1B = 0;
    TCCR1C = 0;
    TCNT1 = 0;
    OCR1A = 0;
    OCR1B = 0;
}

static void set_timer(unsigned long us)
{
    TCCR1A = 0;
    // divide by 8 prescaling
    TCCR1B = 0 << CS22 | 1 << CS21 | 0 << CS20;
    TCCR1B |= 0 << WGM13 | 1 << WGM12; // CTC mode

    OCR1A = COUNTS(us);
    // enable output compare A match interrupt
    TIMSK1 = 1 << OCIE1A;
}

 /*
  *  Zero crossing interrupt handler
  */

static Triac* triacs[2];

static void on_change_0()
{
    Triac* triac = triacs[0];
    if (triac)
        triac->on_change();
}

static void on_change_1()
{
    Triac* triac = triacs[1];
    if (triac)
        triac->on_change();
}

 /*
  *  Timer compare interrupt
  */

ISR(TIMER1_COMPA_vect)
{
    for (int i = 0; i < 2; ++i) {
        Triac* triac = triacs[i];
        if (triac)
            triac->on_timer();
    }
}

 /*
  * Zero Crossing Interrupt Handler
  */

void Triac::on_change(void)
{
    if (m_get_fn()) 
    {
        // end of cycle
        set_state(ZERO);
    } else {
        // start of cycle.
        set_state(PHASE);
    }
}

 /*
  *  State Machine
  */

int Triac::percent_to_count(int percent)
{
    const long t = m_period - m_zero_t;
    return t - ((t * m_percent) / 100);
}

void Triac::set_state(Triac::state next)
{
    clear_timer();
    const unsigned long now = micros();

    switch (next) {
        case ZERO :
            m_set_fn(0);
            m_period = now - m_start_t;
            m_start_t = now;
  
        case PHASE :
            set_timer(percent_to_count(m_percent));
            m_zero_t = now - m_start_t;
            break;

        case TRIAC_ON :
            m_set_fn(1);
            break;
    }
  
    if (m_percent == 100)
        m_set_fn(1);

    m_state = next;
}

 /*
  *  Timer Compare interrupt
  */

void Triac::on_timer(void)
{
    if (m_state == PHASE)
        set_state(TRIAC_ON);
}

void Triac::init(int irq_num)
{
    switch (irq_num)
    {
        case 0 : attachInterrupt(0, on_change_0, CHANGE); break;
        case 1 : attachInterrupt(1, on_change_1, CHANGE); break;
        default: return;
    }

    triacs[irq_num] = this;

    clear_timer();
    set_state(ZERO);
}

// FIN
