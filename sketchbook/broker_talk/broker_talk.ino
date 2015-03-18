
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

 /*
  *
  */

#define TX_LED 4
#define RX_LED 5
#define LOOP_LED 6
#define GND_LED 7

static void tx_led(byte on) {
  digitalWrite(TX_LED, on);
}

static void rx_led(byte on) {
  digitalWrite(RX_LED, on);
}

static void status_led(byte on) {
  digitalWrite(LOOP_LED, on);
}

class LED {
private:
  void (*m_fn)(byte on);
  int m_count;
public:
  LED(void (*fn)(byte on))
  : m_fn(fn),
    m_count(0)
  {
  }

  void set(int count)
  {
    (*m_fn)(count > 0);
    m_count = count;
  }
  
  void poll(void)
  {
    if (m_count > 0)
    {
      if (!--m_count)
      {
        (*m_fn)(0);
      }
    }
  }
};

static LED tx(tx_led);
static LED rx(rx_led);
static LED status(status_led);

  /*
  *
  */
  
enum PARSE_STATE
{
  WAIT,
  NODE,
  LENGTH,
  DATA,
};

#define MAX_DATA 128

class Parser
{
  
public:
  int state;
  int node;
  int length;
  int count;
  unsigned char* next_data;
  unsigned char data[MAX_DATA];

  Parser()
  {
    reset();
  }

  void reset(void)
  {
    length = -1;
    count = 0;
    next_data = 0;
    state = WAIT;
  }

  int parse(unsigned char c)
  {
    switch (state)
    {
      case WAIT : {
        if (c = 'l') {
          state = NODE;
          return 0;
        }
        
        reset();      
        return 0;
      }
      case NODE : {
        if (c == 'i') {
          node = 0;
          return 0;
        }
        if ((c >= '0') && (c <= '9')) {
          node *= 10;
          node += c - '0';
          return 0;
        }
        if (c == 'e') {
          length = 0;
          state = LENGTH;
          return 0;
        }
        
        reset();
        return 0;
      }
      
      case LENGTH : {
        if (c == ':') {
          if (length >= MAX_DATA) {
            reset();
            return 0;
          }
          state = DATA;
          count = 0;
          next_data = data;
          return 0;
        }
        if ((c >= '0') && (c <= '9')) {
          length *= 10;
          length += c - '0';
          return 0;
        }
        
        reset();
        return 0;
      }
      
      case DATA : {
        if (count < length) {
          *next_data++ = c;
          count++;
          return 0;
        }
        
        // done : message is complete
        return 1;
      }
    }
  }
};

static void to_host(int node, uint8_t* data, int bytes)
{
  // send the packet in Bencode to the host        
  Serial.print("li");
  Serial.print(node);
  Serial.print("e");
  Serial.print(bytes);
  Serial.print(":");
  for (int i = 0; i < bytes; ++i)
    Serial.print((char) data[i]);
  Serial.print("e");
}

static void packet_to_host(void)
{
  // send the packet in Bencode to the host    
  to_host((int) rf12_hdr, (uint8_t*) rf12_data, (int) rf12_len);
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
  *
  */


// 'present' flags
#define PRESENT_TEMP 0x01

#define TEMP_PIN 0

static int decode_command(uint8_t* data, int length)
{
  Message command((void*) data);

  if (!(command.get_ack()))
    return 0;

  Message response(command.get_mid(), GATEWAY_ID);
  const uint16_t t = get_temperature(TEMP_PIN);
  response.append(PRESENT_TEMP, & t, sizeof(t));

  to_host(GATEWAY_ID, (uint8_t*) response.data(), response.size());

  return 1;
}

 /*
  *
  */
  
static byte my_node = 0;

void setup () {
  Serial.begin(57600);

  pinMode(RX_LED, OUTPUT);
  pinMode(TX_LED, OUTPUT);
  pinMode(LOOP_LED, OUTPUT);
  pinMode(GND_LED, OUTPUT);

  digitalWrite(TX_LED, 0);
  digitalWrite(RX_LED, 0);
  digitalWrite(LOOP_LED, 0);
  digitalWrite(GND_LED, 0);

  // use the 1.1V internal ref for the ADC
  analogReference(INTERNAL);

  rf12_initialize(my_node = GATEWAY_ID, RF12_868MHZ, 6);
  //my_node = rf12_config();
}

MilliTimer sendTimer;
MilliTimer ledTimer;
byte needToSend;
#define FLASH_PERIOD 2
static uint8_t auto_ack_id = 0;
static uint8_t auto_dest;

Parser parser;

void loop () {
  
  static int hasSend = 0;
  
  // Flash the lights
  if (ledTimer.poll(50))
  {
    tx.poll();
    rx.poll();
    status.poll();
  }

  // send any rx data to the host  
  if (rf12_recvDone() && rf12_crc == 0) {
    rx.set(FLASH_PERIOD);
    packet_to_host();

    Message msg((void*) rf12_data);
    if (msg.get_ack()) {
        auto_ack_id = msg.get_mid();
        auto_dest = rf12_hdr & 0x1F;
    }
  }

  if (auto_ack_id) {
    if (rf12_canSend()) {
      // send ack back
      tx.set(FLASH_PERIOD);
 
      Message msg(auto_ack_id, auto_dest);
 
      rf12_sendStart(auto_dest, msg.data(), msg.size());
      auto_ack_id = 0;
    }    
    return;
  }

  if (hasSend) {
    if (rf12_canSend()) {
      // transmit waiting message
      tx.set(FLASH_PERIOD);
 
      rf12_sendStart(parser.node, parser.data, parser.length);
      parser.reset();
      hasSend = 0;
    }
    return;
  }

  // read any host messages
  if (Serial.available()) {
    if (parser.parse(Serial.read())) {
      status.set(FLASH_PERIOD);
      if (parser.node == GATEWAY_ID) {
        // it is for me!!
        decode_command(parser.data, parser.length);
        parser.reset();
        parser.node = 0;
      }
      else
        hasSend = 1;
    }
  }
}

// FIN
