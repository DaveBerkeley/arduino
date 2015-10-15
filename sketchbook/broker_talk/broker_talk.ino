
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
#include <led.h>

#include <OneWire.h>
#include <DallasTemperature.h>

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
  * LED control
  */

#define TX_LED 6
#define RX_LED 7
#define LOOP_LED 8
#define UNKNOWN_LED 9

static void set_tx_led(bool on) {
  digitalWrite(TX_LED, !on);
}

static void set_rx_led(bool on) {
  digitalWrite(RX_LED, !on);
}

static void set_status_led(bool on) {
  digitalWrite(LOOP_LED, !on);
}

static void set_unknown_led(bool on) {
  digitalWrite(UNKNOWN_LED, !on);
}

static LED tx(set_tx_led);
static LED rx(set_rx_led);
static LED status_led(set_status_led);
static LED unknown_led(set_unknown_led);

static LED* all_leds[] = {
    & tx, & rx, & status_led, & unknown_led, 
    0
};

static int8_t leds[] = { 
  TX_LED,
  RX_LED,
  LOOP_LED,
  UNKNOWN_LED,
  -1,
};

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

class Packet
{
public:
  int node;
  int length;
  unsigned char data[MAX_DATA];

  void reset()
  {
      node = 0;
      length = -1;
  }
};

class Parser
{
public:
  PARSE_STATE state;
  unsigned char* next_data;
  int count;

  Parser()
  : state(WAIT)
  {
  }

  void reset(Packet* msg)
  {
    msg->reset();
    count = 0;
    next_data = 0;
    state = WAIT;
  }

  int parse(Packet* msg, unsigned char c)
  {
    switch (state)
    {
      case WAIT : {
        if (c == 'l') {
          state = NODE;
          return 0;
        }
 
        reset(msg);      
        return 0;
      }
      case NODE : {
        if (c == 'i') {
          msg->node = 0;
          return 0;
        }
        if ((c >= '0') && (c <= '9')) {
          msg->node *= 10;
          msg->node += c - '0';
          return 0;
        }
        if (c == 'e') {
          msg->length = 0;
          state = LENGTH;
          return 0;
        }
        
        reset(msg);
        return 0;
      }
      
      case LENGTH : {
        if (c == ':') {
          if (msg->length >= MAX_DATA) {
            reset(msg);
            return 0;
          }
          state = DATA;
          count = 0;
          next_data = msg->data;
          return 0;
        }
        if ((c >= '0') && (c <= '9')) {
          msg->length *= 10;
          msg->length += c - '0';
          return 0;
        }
        
        reset(msg);
        return 0;
      }
      
      case DATA : {
        if (count < msg->length) {
          *next_data++ = c;
          count++;
          return 0;
        }
        
        // done : message is complete
        return 1;
      }
    }
    return 0;
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
  *
  */

// Map of unknown devices
static uint32_t unknown_devs;

#define CMD_UNKNOWN (1<<0)

// 'present' flags
#define PRESENT_TEMP 0x01

static int decode_command(uint8_t* data, int length)
{
  Message command((void*) data);

  unknown_led.set(0);

  // Check for unknown device bitmap
  uint32_t mask;
  if (command.extract(CMD_UNKNOWN, & mask, sizeof(mask))) {
    unknown_devs = mask;
    if (mask) {
      unknown_led.set(8000);
    }
  }

  if (!(command.get_ack()))
    return 0;
  
  Message response(command.get_mid(), GATEWAY_ID);

  sensors.requestTemperatures(); // Send the command to get temperatures
  const float ft = sensors.getTempCByIndex(0);
  const uint16_t t = int(ft * 100);
  response.append(PRESENT_TEMP, & t, sizeof(t));

  to_host(GATEWAY_ID, (uint8_t*) response.data(), response.size());

  return 1;
}

#define MAX_PACKETS 4
static Parser parser;
static Packet packets[MAX_PACKETS];
static uint8_t host_idx = 0;
static Packet* host_packet = & packets[host_idx];

static Packet* next_packet()
{
    for (int i = 0; i < MAX_PACKETS; ++i) {
        Packet* p = & packets[i];
        if (p == host_packet)
            continue;
        if (p->node != 0)
            return p;
    }
    return 0;
}

static Packet* next_host_packet()
{
    //  TODO : Need better buffer then this!
    host_idx += 1;
    host_idx %= MAX_PACKETS;
    return & packets[host_idx];
}

 /*
  *
  */

void setup () {
  Serial.begin(57600);

  LED::init(leds);

  // use the 1.1V internal ref for the ADC
  analogReference(INTERNAL);

  pinMode(PULLUP_PIN, INPUT_PULLUP);
  sensors.begin();

  rf12_initialize(GATEWAY_ID, RF12_868MHZ, 6);

  // LEDs off
  for (LED** led = all_leds; *led; ++led) {
      (*led)->set(false);
  }
  // chase the LEDs
  for (LED** led = all_leds; *led; ++led) {
      (*led)->set(true);
      delay(200);
      (*led)->set(false);
  }

  parser.reset(host_packet);
}

MilliTimer sendTimer;
MilliTimer ledTimer;
#define FLASH_PERIOD 2

#define MAX_DEVS 32 // defined elsewhere?
static uint8_t ack_mids[MAX_DEVS];
static uint32_t ack_mask;

static uint8_t get_next_ack(uint8_t* mid)
{
    // Return the next device requiring an ack
    uint32_t mask = ack_mask;
    for (uint8_t dev = 0; mask; dev += 1, mask >>= 1) {
        if (mask & 0x01) {
            *mid = ack_mids[dev];
            return dev;
        }
    }
    return 0;
}

static void clear_ack(uint8_t dev)
{
    ack_mask &= ~(1 << dev);
}

static void mark_ack(uint8_t dev, uint8_t mid)
{
    ack_mids[dev] = mid;
    ack_mask |= 1 << dev;
}

  /*
  *  Main loop
  */

void loop () {
  
  // Flash the lights
  if (ledTimer.poll(25))
  {
    for (LED** led = all_leds; *led; ++led) {
        (*led)->poll();
    }
  }

  // send any rx data to the host  
  if (rf12_recvDone() && (rf12_crc == 0)) {
    rx.set(FLASH_PERIOD);
    // Pass the data straight to the host
    packet_to_host();

    Message msg((void*) rf12_data);
    if (msg.get_ack()) {
        // Mark the device as requiring an ACK for mid
        mark_ack(rf12_hdr & 0x1F, msg.get_mid());
    }
  }

  uint8_t mid;
  const uint8_t dev = get_next_ack(& mid);
  if (dev) {
    if (rf12_canSend()) {
      // send ack back
      tx.set(FLASH_PERIOD);
 
      Message msg(mid, dev);
 
      if (unknown_devs & (1<<dev)) {
        msg.set_admin();
      } else {
        // TODO : send any queued messages
      }
 
      rf12_sendStart(dev, msg.data(), msg.size());
      clear_ack(dev);
    }    
    return;
  }

  // read from host rx buffers
  Packet* pm = next_packet();
  if (pm) {
    if (rf12_canSend()) {
      // transmit waiting message
      tx.set(FLASH_PERIOD);
 
      rf12_sendStart(pm->node, pm->data, pm->length);
      pm->reset();
    }
    return;
  }

  // read any host messages
  if (Serial.available()) {
    if (parser.parse(host_packet, Serial.read())) {
      status_led.set(FLASH_PERIOD);
      if (host_packet->node == GATEWAY_ID) {
        // it is for me!!
        decode_command(host_packet->data, host_packet->length);
      } else {
        // copy to rx buffers

        // TODO : for sleepy devices, queue awaiting ACK_REQ
        host_packet = next_host_packet();
      }
      parser.reset(host_packet);
    }
  }
}

// FIN
