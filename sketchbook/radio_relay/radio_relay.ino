
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

#include <OneWire.h>
#include <DallasTemperature.h>

  /*
   *
   */

// needed by the watchdog code
EMPTY_INTERRUPT(WDT_vect);

  /*
  *
  */
  
// Data wire
#define ONE_WIRE_BUS 5 // jeenode port 1 digital pin
#define PULLUP_PIN A1  // jeenode port 1 analog pin

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

  /*
  *
  */  

// node -> gateway data
#define PRESENT_TEMPERATURE (1 << 1)
#define PRESENT_STATE (1 << 2)
#define PRESENT_VCC (1 << 3)

  /*
  *  LED handling
  */
  
#define OK_LED 6
#define TEST_LED 7
#define RELAY 4
#define RELAY_LED 9

class Pin
{
  int m_pin;
  RadioDev::LED m_led;
  int16_t  m_count;
public:
  Pin(int pin, RadioDev::LED led)
  : m_pin(pin),
    m_led(led),
    m_count(0)
  {
  }
  
  bool match(RadioDev::LED led)
  {
    return led == m_led;
  }
  
  void init()
  {
    pinMode(m_pin, OUTPUT);
    set(0);
  }
  
  void set(int16_t count)
  {
    m_count = count;
    digitalWrite(m_pin, (count > 0) ? LOW : HIGH);
  }
  
  void poll()
  {
    if (m_count > 0) {
      if (!--m_count) {
        set(0);
      }
    }
  }
};

#define NPINS 2

static Pin pins[NPINS] = {
  Pin(OK_LED, RadioDev::OK),
  Pin(TEST_LED, RadioDev::TEST),
};

static void set_led(RadioDev::LED led, bool state)
{
  for (int p = 0; p < NPINS; ++p) {
    if (pins[p].match(led)) {
      pins[p].set(state ? 1000 : 0);
      return;
    }
  }
}

static void poll_leds()
{
  for (int p = 0; p < NPINS; ++p) {
    pins[p].poll();
  }
}

static void init_leds()
{
  for (int p = 0; p < NPINS; ++p) {
    pins[p].init();
  }
}

  /*
  *
  */

class RadioRelay : public RadioDev
{
  bool m_on;
public:

  RadioRelay()
  : RadioDev(GATEWAY_ID, 0),
    m_on(false)
  {
  }
  
  void set_relay(bool state)
  {
    m_on = state;
    digitalWrite(RELAY, state ? LOW : HIGH);
    digitalWrite(RELAY_LED, state ? LOW : HIGH);
  }

  virtual void init()
  {
    RadioDev::init();
    pinMode(RELAY, OUTPUT);
    pinMode(RELAY_LED, OUTPUT);

    // use the 1.1V internal ref for the ADC
    analogReference(INTERNAL);
    
    pinMode(PULLUP_PIN, INPUT_PULLUP);
    sensors.begin();
    
    init_leds();
    set_relay(0);
  }

  virtual const char* banner()
  {
    return "Relay Device v1.0";
  }

  virtual void append_message(Message* msg)
  {
    sensors.requestTemperatures(); // Send the command to get temperatures
    const float ft = sensors.getTempCByIndex(0);
    const uint16_t t = int(ft * 100);
    msg->append(PRESENT_TEMPERATURE, & t, sizeof(t));
    
    // append relay state
    msg->append(PRESENT_STATE, & m_on, sizeof(m_on));

    const uint16_t vcc = read_vcc();
    msg->append(PRESENT_VCC, & vcc, sizeof(vcc));
  }

  virtual void loop(void)
  {
    radio_poll();
    poll_leds();
  }

  virtual void on_message(Message* msg)
  {
    set_led(OK, true);
    const uint8_t a = msg->get_admin();
    const uint8_t f = msg->get_flags();
    Serial.print(millis());
    Serial.print(" on message ");
    Serial.print(a);
    Serial.print(" ");
    Serial.print(f);
    Serial.print("\r\n");
    
    bool r;
    if (msg->extract(PRESENT_STATE, & r, sizeof(r)))
    {
      set_relay(r);
      Serial.print("set relay ");
      Serial.print(r);
      Serial.print("\r\n");
    }
  }

  virtual void set_led(LED idx, bool state)
  {
    ::set_led(idx, state);
  }
};

static RadioRelay radio;

 /*
  *
  */
  
void setup() 
{
  Serial.begin(57600);
  Serial.println(radio.banner());

  radio.init();
  radio.power_on();
}

 /*
  *
  */

void loop()
{
  radio.loop();
  
  static uint32_t count = 500000;
  
  if (count) {
    if (!--count) {
      count = 500000;
      Serial.print("req tx\r\n");
      radio.req_tx_message();
    }
  }
}

// FIN
