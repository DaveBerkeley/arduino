 
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

/*

    Triac control library.

    Uses timer1 to provide a phase control signal to a triac.
    Needs a zero crossing detector driving an interrupt, see 
    http://www.rotwang.co.uk/projects/kettle.html and
    http://www.rotwang.co.uk/projects/triac.html for example hardware.

    Only one triac output is currently supported. Sharing the timer1 
    control between multiple triac outputs would be possible.

    Usage:

    #include <triac.h>

    ...

    #define TRIAC_PIN 5
    #define ZERO_CROSSING_PIN 3
    #define ZERO_CROSSING_IRQ 1

    ...

    static void set_triac(bool on)
    {
        // function to write to triac control hardware, e.g.
        digitalWrite(TRIAC_PIN, on);
    }

    static bool get_zero(void)
    {
        // function to read zero crossing state, e.g.
        return digitialRead(ZERO_CROSSING_PIN);
    }

    // Create a Triac instance
    static Triac triac(set_triac, get_zero);

    ...

    void setup(void) {
      triac.init(ZERO_CROSSING_IRQ);
      ...
    }

    void loop(void) {

        ...

        triac.set_percent(x);

        ...

        uint8_t p = triac.get_percent();

    }

*/

class Triac
{
    typedef void (*set_fn)(bool on);
    typedef bool (*get_fn)(void);
    // function to set the triac hardware
    set_fn m_set_fn;
    get_fn m_get_fn;

    typedef enum { 
      ZERO,      // zero crossing period
      PHASE,     // start of cycle
      TRIAC_ON,  // pulse the triac
    } state;

private:
    state m_state;
    int8_t m_percent;
    // variables used to calculate cycle period
    long m_start_t;
    long m_zero_t;
    long m_period;

    void set_state(state next);
    int percent_to_count(int percent);

public:
    Triac(set_fn sfn, get_fn gfn)
    :   m_set_fn(sfn),
        m_get_fn(gfn),
        m_percent(0),
        m_start_t(0),
        m_zero_t(0),
        m_period(0)
    { }

    void init(int irq_num);

    void on_change(void);
    void on_timer(void);

    void set_percent(uint8_t percent) { m_percent = percent; }
    uint8_t get_percent(void) { return m_percent; }

};

// FIN
