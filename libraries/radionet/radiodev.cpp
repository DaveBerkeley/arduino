
#include "Arduino.h"

#include <JeeLib.h>

#include <radiodev.h>

  /*
  *
  */

RadioDev::RadioDev(uint8_t gateway_id, uint16_t sleeps)
: message(0, gateway_id),
  gateway_id(gateway_id),
  nsleep(sleeps),
  sleep_count(0)
{
}

void RadioDev::init(void)
{
  my_node = rf12_configSilent();
  state = START;
}
  
void RadioDev::sleep(uint16_t time)
{
  set_led(OK, 0);
  set_led(TEST, 0);
  rf12_sleep(0); // turn the radio off

  if (state != SLEEP) {
      sleep_count = nsleep;
  }

  state = SLEEP;
  //Serial.flush(); // wait for output to complete
  Sleepy::loseSomeTime(time);
}

 /*
  * Build a data Message 
  */
  
void RadioDev::make_message(Message* msg, int msg_id, bool ack) 
{
  msg->reset();
  msg->set_dest(gateway_id);
  msg->set_mid(msg_id);
  if (ack)
    msg->set_ack();
 
  append_message(msg);
}

void RadioDev::radio_loop(uint16_t time)
{
  set_led(OK, 1); // show we are awake

  if (rf12_recvDone() && (rf12_crc == 0)) {
    Message m((void*) & rf12_data[0]);

    if (m.get_dest() == my_node) {
      on_message(& m);
      if (m.get_ack()) {
        // ack the info
        ack_id = m.get_mid();
      }
      else
      {
        ack_id = 0;
        if (state == WAIT_FOR_ACK) {
          if (m.get_admin()) {
            state = START;
            set_led(TEST, 1);
          } else {
            if (m.get_mid() == message.get_mid()) {
              // if we have our ack, go back to sleep
              sleep(time);
            }
          }
        }
      }
    }
  }

  if (state == START) {
    //Serial.print("hello\r\n");
    send_text(banner(), ack_id, false);
    rf12_sendWait(0);
    ack_id = 0;
    set_led(TEST, 0);
    sleep(time);
    return;
  }

  if (state == SLEEP) {
      //Serial.println("send");

      if (sleep_count) {
          sleep_count -= 1;
          if (sleep_count != 0) {
              sleep(time);
              return;
          }
      }

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
      if (--retries) {
        state = SENDING; // try again
      } else {
        sleep(time);
      }
    }
  }
}

// FIN
