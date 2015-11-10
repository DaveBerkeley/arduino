
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

  /*
   *
   */

// needed by the watchdog code
EMPTY_INTERRUPT(WDT_vect);

  /*
  *
  */
  
// node -> gateway data
#define PRESENT_PULSE (1 << 1)
#define PRESENT_VCC (1 << 2)

class PulseRadio : public RadioDev
{
  Port led;

  void ok_led(byte on) 
  {
    led.digiWrite(on);
  }

  void test_led(byte on) 
  {
    led.digiWrite2(!on);
  }

public:

  PulseRadio()
  : RadioDev(GATEWAY_ID),
    led(2)
  {
  }

  virtual void init()
  {
    ok_led(0);
    test_led(0);

    led.mode(OUTPUT);  
    led.mode2(OUTPUT);  

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
    const uint8_t pulse = 1;
    msg->append(PRESENT_PULSE, & pulse, sizeof(pulse));
    const uint16_t vcc = read_vcc();
    msg->append(PRESENT_VCC, & vcc, sizeof(vcc));
  }

  virtual void set_led(LED idx, bool state)
  {
    switch (idx) {
      case   OK   :  ok_led(state);  break;
      case   TEST :  test_led(state);  break;
    }
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

  radio.init();
}

 /*
  *
  */

void loop()
{
  radio.loop();
}

// FIN
