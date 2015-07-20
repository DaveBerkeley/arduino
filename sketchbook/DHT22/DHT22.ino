
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

// DHT22 code Based on AdaFruit library ...

#include <DHT.h>

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
  *  Temperature / Humidity
  */

#define PIN 4 // DIO Port1 on JeeNode
// Connect Vdd to 3V3 and GDND to GND

#define DHT_TYPE DHT22

// node -> gateway data
#define PRESENT_TEMPERATURE (1 << 1)
#define PRESENT_HUMIDITY (1 << 2)
#define PRESENT_VCC (1 << 3)

class HumidityDev : public RadioDev
{
private:
  Port led;
  DHT dht;

  void ok_led(byte on) 
  {
    led.digiWrite(on);
  }

  void test_led(byte on) 
  {
    led.digiWrite2(!on);
  }

  int get_temperature() {
    float t = dht.readTemperature();
    return int(t * 100);
  }

  int get_humidity() {
    float h = dht.readHumidity();
    return int(h * 100);
  }

public:
  HumidityDev(uint8_t gateway_id)
  : RadioDev(gateway_id),
    led(2),
    dht(PIN, DHT_TYPE)
  {
  }  

  virtual void init()
  {
    ok_led(0);
    test_led(0);
  
    led.mode(OUTPUT);  
    led.mode2(OUTPUT);  

    dht.begin();

    RadioDev::init();
  }

  virtual const char* banner()
  {
    return "Humidity Device v1.0";
  }
  
  virtual void loop()
  {
    radio_loop(32000);
  }

private:
  virtual void append_message(Message* msg)
  {
    const uint16_t t = get_temperature();
    msg->append(PRESENT_TEMPERATURE, & t, sizeof(t));
    const uint16_t h = get_humidity();
    msg->append(PRESENT_HUMIDITY, & h, sizeof(h));
    const uint16_t vcc = read_vcc();
    msg->append(PRESENT_VCC, & vcc, sizeof(vcc));
  }
  
  virtual void set_led(LED idx, bool on)
  {
    switch (idx) {
      case OK  :  ok_led(on);    break;
      case TEST:  test_led(on);  break;
    }
  }
};

static HumidityDev radio(GATEWAY_ID);

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
