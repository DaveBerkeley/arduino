
/*
 Copyright (C) 2014 Dave Berkeley projects2@rotwang.co.uk

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

  /*
   *
   */

static const char* banner = "Test Device v1.0";

// node -> gateway data
#define PRESENT_TEMPERATURE (1 << 1)

#define TEMPERATURE_PIN 3

Port pir(1);
Port led(2);

static uint16_t ack_id;
static byte my_node = 0;

typedef enum {
  START=0,
  SLEEP,
  SENDING,
  WAIT_FOR_ACK,
} STATE;

static STATE state;

#define ACK_WAIT_MS 100
#define ACK_RETRIES 5

static uint8_t retries;

static Message message(0, GATEWAY_ID);
static uint32_t wait_until = 0;

// needed by the watchdog code
EMPTY_INTERRUPT(WDT_vect);

 /*
  * IO
  */

static void ok_led(byte on) 
{
  led.digiWrite(on);
}

static void test_led(byte on) 
{
  led.digiWrite2(!on);
}

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
  * Fall into a deep sleep
  */

static uint16_t sleep_count = 0;

#define SLEEP_TIME (32000)
static const int LONG_WAIT = 1;

static void sleep(uint16_t time=SLEEP_TIME)
{
  //rx_led(0);
  //tx_led(0);
  ok_led(0);
  rf12_sleep(0); // turn the radio off
  state = SLEEP;
  //Serial.flush(); // wait for output to complete
  Sleepy::loseSomeTime(time);
}

 /*
  * Build a data Message 
  */

void make_message(Message* msg, int msg_id, bool ack) 
{
  msg->reset();
  msg->set_dest(GATEWAY_ID);
  msg->set_mid(msg_id);
  if (ack)
    msg->set_ack();

  const uint16_t t = get_temperature(TEMPERATURE_PIN);
  msg->append(PRESENT_TEMPERATURE, & t, sizeof(t));
}

 /*
  *
  */

#define IRQ 1
  
void setup() 
{
  ok_led(0);
  test_led(0);
  pir.digiWrite3(0);
  
  led.mode(OUTPUT);  
  led.mode2(OUTPUT);  
  pir.mode3(INPUT);

  Serial.begin(57600);
  Serial.println(banner);

  my_node = rf12_configSilent();

  // use the 1.1V internal ref for the ADC
  analogReference(INTERNAL);

  state = START;
}

 /*
  *
  */

void loop()
{
  static uint16_t changes = 0;
  
  ok_led(1); // show we are awake

  if (rf12_recvDone() && (rf12_crc == 0)) {
    Message m((void*) & rf12_data[0]);

    if (m.get_dest() == my_node) {
      if (m.get_ack()) {
        // ack the info
        ack_id = m.get_mid();
      }
      else
      {
        ack_id = 0;
        if (state == WAIT_FOR_ACK) {
          // if we have our ack, go back to sleep
          if (m.get_mid() == message.get_mid()) {
            // TODO : if the PIR is active it will be power hungry
            // turn it off here for a while
            sleep();
          }
        }
      }
    }
  }

  if (state == START) {
    send_text(banner, ack_id, false);
    rf12_sendWait(0);
    sleep_count = 0;
    ack_id = 0;
    sleep();
    return;
  }

  if (state == SLEEP) {
    static uint16_t last_changes = -1;
    if (changes != last_changes) {
      // PIR sensor has changed state
      Serial.println("send");
      last_changes = changes;
      state = SENDING;
      retries = ACK_RETRIES;
      sleep_count = 0;
      make_message(& message, make_mid(), true);      
      // turn the radio on
      rf12_sleep(-1);
      return;
    }

    // we've woken from sleep with no changes
    if (++sleep_count >= LONG_WAIT) {
      state = START;
    } else {
      // go back to ...
      sleep();
    }
    return;
  }

  if (state == SENDING) {
    if (rf12_canSend()) {
      // report the change
      send_message(& message);
      rf12_sendWait(0); // NORMAL when finished
      state = WAIT_FOR_ACK;
      wait_until = millis() + ACK_WAIT_MS;
    }
    return;
  }

  if (state == WAIT_FOR_ACK) {
    if (millis() > wait_until)
    {
        if (--retries)
            state = SENDING; // try again
        else
            sleep();
    }
  }
}
 
// FIN
