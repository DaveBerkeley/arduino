
/*
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

#include <JeeLib.h>
#include <radionet.h>
#include <triac.h>

static const char* banner = "Triac Control v1.0";

// Send out messages every BLEAT_PERIOD if no
// messages received. Also turn power on full.
#define BLEAT_PERIOD 60000

static uint8_t my_node;

#define PRESENT_POWER (1<<0)
#define PRESENT_TEMPERATURE (1<<1)

static uint16_t ack_id;

MilliTimer led_timer;
MilliTimer status_timer;

#define ZERO_CROSSING_PIN 3
#define ZERO_CROSSING_IRQ 1

#define TEMPERATURE_PIN 0

Port leds(2);
Port control(1);

 /*
  *
  */

void set_triac(bool on) {
    control.digiWrite(on);
}

static bool get_zero_cross(void) {
    return digitalRead(ZERO_CROSSING_PIN);
}

static Triac triac(set_triac, get_zero_cross);

 /*
  *
  */

static void tx_led(bool on) {
  leds.digiWrite(on);
}

static void rx_led(bool on) {
  leds.digiWrite2(!on); // inverse, because LED is tied to VCC
}

class LED {
private:
  int m_count;
  void (*m_fn)(bool on);
public:
  LED(void (*fn)(bool on))
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

void setup () {
  Serial.begin(57600);
  Serial.println(banner);

  leds.mode(OUTPUT);
  leds.mode2(OUTPUT);
  leds.digiWrite(0);
  leds.digiWrite2(1);
  
  control.mode(OUTPUT);
  control.digiWrite(0);

  my_node = rf12_config();

  triac.init(ZERO_CROSSING_IRQ);

  // use the 1.1V internal ref for the ADC
  analogReference(INTERNAL);

  // periodically send out a message, if not in communication
  status_timer.set(1000 + (my_node * 10));
}

static Message message(0, GATEWAY_ID);

void loop () {
  // Flash the lights
  if (led_timer.poll(50))
  {
    rx.poll();
    tx.poll();
  }

  if (rf12_recvDone() && (rf12_crc == 0)) {
    rx.set(2);

    Message msg((void*) rf12_data);

    if (msg.get_dest() == my_node) {

        if (msg.get_admin()) {
          // TODO : handle admin message
        }
        else
        {
          uint16_t p;
          if (msg.extract(PRESENT_POWER, & p, sizeof(p)))
            triac.set_percent(p);

          if (msg.get_ack())
            ack_id = msg.get_mid();
        }
        status_timer.set(BLEAT_PERIOD);
    }
  }

  if (ack_id && rf12_canSend()) {
    tx.set(2);

    message.reset();
    message.set_mid(ack_id);

    const uint16_t percent = triac.get_percent();
    message.append(PRESENT_POWER, & percent, sizeof(percent));
    uint16_t t = get_temperature(TEMPERATURE_PIN);
    message.append(PRESENT_TEMPERATURE, & t, sizeof(t));

    send_message(& message);

    ack_id = 0;
  }

  static bool send_status = false;
  if (status_timer.poll()) {
    send_status = true;
  }

  if (send_status && rf12_canSend()) {
    tx.set(2);

    send_text(banner, make_mid(), false);
    send_status = 0;
    status_timer.set(BLEAT_PERIOD);
  }
}

// FIN
