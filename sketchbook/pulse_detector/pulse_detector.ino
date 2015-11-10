
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

#include <JeeLib.h>

#include <radiodev.h>
#include <radioutils.h>

#define INT_PIN 3
#define LED_PIN 9

  /*
   *
   */

// needed by the watchdog code
EMPTY_INTERRUPT(WDT_vect);

static volatile uint32_t pulses;

static void on_pulse()
{
    static bool led = true;
    digitalWrite(LED_PIN, led = !led);
    pulses += 1;
}

  /*
  *
  */
  
// node -> gateway data
#define PRESENT_PULSE (1 << 0)
#define PRESENT_VCC (1 << 1)

class PulseRadio : public RadioDev
{

public:

  PulseRadio()
  : RadioDev(GATEWAY_ID)
  {
  }

  virtual void init()
  {
    RadioDev::init();

    // use the 1.1V internal ref for the ADC
    analogReference(INTERNAL);
  }

  virtual const char* banner()
  {
    return "Pulse Detector v1.0";
  }

  virtual void append_message(Message* msg)
  {
    const uint32_t p = pulses;
    msg->append(PRESENT_PULSE, & p, sizeof(p));

    const uint16_t vcc = read_vcc();
    msg->append(PRESENT_VCC, & vcc, sizeof(vcc));
  }

  virtual void set_led(LED idx, bool state)
  {
    ; // Do nothing
  }
  
  virtual void loop(void)
  {
    radio_loop(32767);
  }
};

static PulseRadio radio;

 /*
  *
  */

void setup() 
{
  Serial.begin(57600);
  Serial.println(radio.banner());
    
  pinMode(INT_PIN, INPUT);
  attachInterrupt(1, on_pulse, RISING);

  pinMode(LED_PIN, OUTPUT);

  radio.init();
}

 /*
  *
  */

void loop()
{
    radio.loop();
    static uint32_t was = -1;

    if (pulses != was) {
        Serial.print("P:");
        Serial.print(pulses);
        Serial.print("\r\n");
        was = pulses;
    }

    //Serial.print(digitalRead(INT_PIN));
    //Serial.print("\r\n");
}

// FIN
