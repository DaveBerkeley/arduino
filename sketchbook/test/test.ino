
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

#include <radionet.h>

  /*
   *
   */

// needed by the watchdog code
EMPTY_INTERRUPT(WDT_vect);

 /*
  * IO
  */

Port led(2);

static void ok_led(byte on) 
{
  led.digiWrite(on);
}

static void test_led(byte on) 
{
  led.digiWrite2(!on);
}

  /*
  *
  */
  
#define ACK_WAIT_MS 100
#define ACK_RETRIES 5

class Radio {
public:
  typedef enum {
    START=0,
    SLEEP,
    SENDING,
    WAIT_FOR_ACK,
  } STATE;

  STATE state;
  byte my_node = 0;
  uint16_t ack_id;
  uint8_t retries;
  Message message;
  uint32_t wait_until = 0;

  Radio()
  : message(0, GATEWAY_ID)
  {
  }

  virtual const char* banner() = 0;
  
  void init(void)
  {
    my_node = rf12_configSilent();
  }

  static const int SLEEP_TIME = 32000;

  void sleep(uint16_t time=SLEEP_TIME)
  {
    ok_led(0);
    test_led(0);
    rf12_sleep(0); // turn the radio off
    state = SLEEP;
    //Serial.flush(); // wait for output to complete
    Sleepy::loseSomeTime(time);
  }

   /*
    * Build a data Message 
    */
  
  virtual void append_message(Message* msg) = 0;

  void make_message(Message* msg, int msg_id, bool ack) 
  {
    msg->reset();
    msg->set_dest(GATEWAY_ID);
    msg->set_mid(msg_id);
    if (ack)
      msg->set_ack();
   
    append_message(msg);
  }
  
  void loop(void)
  {
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
            if (m.get_admin()) {
              state = START;
              test_led(1);
            } else {
              if (m.get_mid() == message.get_mid()) {
                sleep();
              }
            }
          }
        }
      }
    }
  
    if (state == START) {
      Serial.print("hello\r\n");
      send_text(banner(), ack_id, false);
      rf12_sendWait(0);
      ack_id = 0;
      test_led(0);
      sleep();
      return;
    }
  
    if (state == SLEEP) {
        Serial.println("send");
        state = SENDING;
        retries = ACK_RETRIES;
        make_message(& message, make_mid(), true);      
        // turn the radio on
        rf12_sleep(-1);
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
};

  /*
  *
  */

// node -> gateway data
#define PRESENT_TEMPERATURE (1 << 1)

#define TEMPERATURE_PIN 0

 /*
  *  Temperature 
  */

static const float temp_scale = (1.1 * 100) / 1024;

static int get_temperature(int pin) {
  static int t;
  return ++t * 100;
  //uint16_t analog = analogRead(pin);
  //const float t = temp_scale * analog;
  //return int(t * 100);
}

class TestRadio : public Radio
{
public:
  virtual const char* banner()
  {
    return "Test Device v1.0";
  }

  virtual void append_message(Message* msg)
  {
    const uint16_t t = get_temperature(TEMPERATURE_PIN);
    msg->append(PRESENT_TEMPERATURE, & t, sizeof(t));
  }
};

static TestRadio radio;

 /*
  *
  */
  
void setup() 
{
  ok_led(0);
  test_led(0);

  led.mode(OUTPUT);  
  led.mode2(OUTPUT);  

  Serial.begin(57600);
  Serial.println(radio.banner());

  //my_node = rf12_configSilent();
  radio.init();

  // use the 1.1V internal ref for the ADC
  analogReference(INTERNAL);

  radio.state = Radio::START;
}

 /*
  *
  */

void loop()
{
  radio.loop();
}

// FIN
