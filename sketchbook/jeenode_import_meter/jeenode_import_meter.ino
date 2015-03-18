
/*
 Arduino code to read data from an Elster A100C electricity meter.

 Copyright (C) 2012 Dave Berkeley solar@rotwang.co.uk

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

#include <buffer.h>
#include <elster.h>
#include <radionet.h>

static const char* banner = "Elster Meter Reader v1.0";

// set pin numbers:
const int intPin = 3;

// node -> gateway data
#define PRESENT_METER           (1 << 0)
#define PRESENT_TEMPERATURE     (1 << 1)
#define PRESENT_UPDATED         (1 << 2)

static uint16_t ack_id;

#define TEMPERATURE_PIN 3

MilliTimer ledTimer;

  /*
   *
   */

Port leds(2);
Port debug(3);

static void tx_led(byte on) {
  leds.digiWrite(on);
}

static void rx_led(byte on) {
  leds.digiWrite2(!on); // inverse, because LED is tied to VCC
}

static void ok_led(byte on) {
  debug.digiWrite(on);
}

class LED {
private:
  int m_count;
  void (*m_fn)(byte on);
public:
  LED(void (*fn)(byte on))
  : m_fn(fn)
  {
  }
  
  void set(int count)
  {
    if (count)
      (*m_fn)(1);
    m_count = count;
  }
  
  void poll(void)
  {
    if (m_count > 0) {
      if (!--m_count) {
        (*m_fn)(0);
      }
    }
  }
};

static LED rx(rx_led);
static LED tx(tx_led);
static LED okay(ok_led);

#define FLASH_PERIOD 2

 /*
  *
  */

static uint32_t reading = 0;
static byte updated = 0; // 1 if updated since last read

void meter_reading(unsigned long r)
{
  //  TODO : send over radio
  Serial.print(r);
  Serial.print("\r\n");

  reading = r;
  // show it is working
  okay.set(FLASH_PERIOD);

  updated = true;
}

ElsterA100C meter(meter_reading);

 /*
  *  Temperature 
  */

static const float temp_scale = (1.1 * 100) / 1024;

static int get_temperature(int pin) {
  uint16_t analog = analogRead(pin);
  const float t = temp_scale * analog;
  return int(t * 100);
}

 /*
  *
  */
  
static byte my_node = 0;

void setup() 
{
  meter.init(1);

  pinMode(intPin, INPUT);

  leds.mode(OUTPUT);
  leds.mode2(OUTPUT);
  leds.digiWrite(0);
  leds.digiWrite2(1);

  debug.mode(OUTPUT);  
  debug.digiWrite(0);
  
  Serial.begin(57600);
  Serial.println(57600);
  Serial.println("Elster Meter Reader");

  my_node = rf12_config();

  // use the 1.1V internal ref for the ADC
  analogReference(INTERNAL);
}
    
 /*
  *
  */

void loop()
{
  // Flash the lights
  if (ledTimer.poll(50))
  {
    rx.poll();
    tx.poll();
    okay.poll();
  }

  // Decode the meter stream
  const int byte_data = meter.decode_bit_stream();
  if (byte_data != -1) {
    meter.on_data(byte_data);
  }

  if (rf12_recvDone() && rf12_crc == 0) {
    rx.set(FLASH_PERIOD);

    Message m((void*) & rf12_data[0]);

    /*
    Serial.print("got message ");
    Serial.print(rf12_hdr & RF12_HDR_MASK);
    Serial.print(" ");
    Serial.print(m.get_mid());
    Serial.print(" ");
    Serial.print(m.get_dest());
    if (m.get_dest() == my_node)
      Serial.print("*");
    Serial.print("\n\r");
    */
    
    if (m.get_dest() == my_node) {

      // TODO : handle message

      if (m.get_ack()) {
        ack_id = m.get_mid();
      }
    }
  }

  if (ack_id && rf12_canSend()) {
    tx.set(FLASH_PERIOD);

    Message msg(ack_id, GATEWAY_ID);
    const uint16_t t = get_temperature(TEMPERATURE_PIN);
    msg.append(PRESENT_METER, & reading, sizeof(reading));
    msg.append(PRESENT_TEMPERATURE, & t, sizeof(t));
    msg.append(PRESENT_UPDATED, & updated, sizeof(updated));
    rf12_sendStart(GATEWAY_ID, msg.data(), msg.size());

    updated = false;

    ack_id = 0;
  }
}
 
// FIN
