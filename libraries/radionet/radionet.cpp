
/*
 RF12 utilities.

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

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include <pins_arduino.h>
#endif

#include <JeeLib.h>

#include <radionet.h>

  /*
  * Messaging
  */
 
uint8_t make_mid()
{
    static uint8_t msg_id = 0;
    if (++msg_id == 0)
        msg_id = 1;
    return msg_id;
}

void show_message(Message* msg, const char* text, uint8_t my_node)
{
    Serial.print(text);
    Serial.print(" id=");
    Serial.print(msg->get_mid());
    Serial.print(" dst=");
    Serial.print(msg->get_dest());
    Serial.print(" ack=");
    Serial.print(msg->get_ack());
    if (msg->get_dest() == my_node)
      Serial.print("*");
    Serial.print("\r\n");
}

void send_message(Message* msg)
{
  rf12_sendStart(msg->get_dest(), msg->data(), msg->size());
}

static void make_message(Message* msg, int msg_id, bool ack) 
{
  msg->reset();
  msg->set_dest(GATEWAY_ID);
  msg->set_mid(msg_id);
  if (ack)
    msg->set_ack();
}

void send_text(const char* text, int msg_id, bool ack)
{
    Message msg(0);

    if (!msg_id)
        msg_id = make_mid();
    make_message(& msg, msg_id, ack);

    const uint8_t len = strlen(text);
    msg.append(Message::TEXT, & len, sizeof(len));
    msg.append(Message::TEXT, text, len);

    send_message(& msg);
}

// FIN
