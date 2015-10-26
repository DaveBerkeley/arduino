
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
  sleep_count(0),
  debug_fn(0)
{
}

void RadioDev::init(void)
{
  my_node = rf12_configSilent();
  state = START;
}

void RadioDev::set_debug(void (*fn)(const char*))
{
  debug_fn = fn;
}

void RadioDev::debug(const char* text)
{
  if (debug_fn)
    debug_fn(text);
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
      on_message_handler(& m);
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

void RadioDev::power_on()
{
  // turn the radio on
  rf12_sleep(-1);
}

void RadioDev::on_message_handler(Message* msg)
{
  uint8_t size;
  if (msg->extract(Message::TEXT, & size, sizeof(size))) {
    char* text = (char*) msg->payload();
    if (size < (Message::DATA_SIZE - 1)) {
        text[size] = '\0';
        text -= 1;
        *text = '!';
        debug(text);
    }
  } else {
    on_message(msg);
  }
}

void RadioDev::radio_poll()
{
  if (rf12_recvDone() && (rf12_crc == 0)) {
    Message m((void*) & rf12_data[0]);

    if (m.get_dest() == my_node) {
      on_message_handler(& m);
      if (m.get_ack()) {
        // ack the message : probably a poll from the gateway
        const uint8_t ack = m.get_mid();
        state = SENDING;
        retries = ACK_RETRIES;
        make_message(& message, ack, false);
      }
      else
      {
        if (state == WAIT_FOR_ACK) {
          if (m.get_admin()) {
            state = START;
            set_led(TEST, 1);
          } else {
            if (m.get_mid() == message.get_mid()) {
              // if we have our ack, go back to listening
              state = LISTEN;
            }
          }
        }
      }
    }
  }

  if (state == START) {
    if (debug_fn) {
        debug_fn(banner());
        debug_fn("\r\n");
    }
    send_text(banner(), ack_id, false);
    rf12_sendWait(0);
    ack_id = 0;
    state = LISTEN;
    return;
  }

  if (state == LISTEN) {
      // no-op state
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
        delay(10);
      }
    }
  }
}

void RadioDev::req_tx_message()
{
  state = SENDING;
  retries = ACK_RETRIES;
  make_message(& message, make_mid(), true);      
}

// FIN
