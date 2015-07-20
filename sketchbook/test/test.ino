
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

  /*
   *
   */

// needed by the watchdog code
EMPTY_INTERRUPT(WDT_vect);

  /*
  *  Read Vcc using the 1.1V reference.
  *
  *  from :
  *
  *  https://code.google.com/p/tinkerit/wiki/SecretVoltmeter
  */

long read_vcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC))
    ;
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

  /*
  *
  */
  
// node -> gateway data
#define PRESENT_TEMPERATURE (1 << 1)
#define PRESENT_VCC (1 << 2)

#define TEMPERATURE_PIN 0

static const float temp_scale = (1.1 * 100) / 1024;

static int get_temperature(int pin) {
  uint16_t analog = analogRead(pin);
  const float t = temp_scale * analog;
  return int(t * 100);
}

class TestRadio : public RadioDev
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

  TestRadio(uint8_t gateway_id)
  : RadioDev(gateway_id),
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
    return "Test Device v1.0";
  }

  virtual void append_message(Message* msg)
  {
    const uint16_t t = get_temperature(TEMPERATURE_PIN);
    msg->append(PRESENT_TEMPERATURE, & t, sizeof(t));
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
    radio_loop(32000);
  }
};

static TestRadio radio(GATEWAY_ID);

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
