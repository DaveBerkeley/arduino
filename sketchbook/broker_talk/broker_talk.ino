
/*
 Copyright (C) 2012, 2015 Dave Berkeley projects2@rotwang.co.uk

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
#include <bencode.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <MsTimer2.h>

  /*
  *
  */
  
// Data wire
#define ONE_WIRE_BUS 5 // jeenode port 2 digital pin
#define PULLUP_PIN A1  // jeenode port 2 analog pin

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

static void packet_to_host(void)
{
  // send the packet in Bencode to the host    
  Bencode::to_host((int) rf12_hdr, (uint8_t*) rf12_data, (int) rf12_len);
}

static void send_fn(const void* data, int bytes)
{
  Bencode::to_host(GATEWAY_ID, (const uint8_t*) data, bytes);    
}

    /*
    *   Sleepy / Unknown maps
    */

// Map of unknown devices
static uint32_t unknown_devs;
// Map of known sleepy devices
static uint32_t sleepy_devs;

static bool is_unknown(uint8_t dev)
{
    return unknown_devs & (1 << dev);
}

static bool is_sleepy(uint8_t dev)
{
    return sleepy_devs & (1 << dev);
}

    /*
    * Packet management
    */

#define MAX_PACKETS 8
static Packet packets[MAX_PACKETS];
static Packet* host_packet = & packets[0];

    /*
    *
    */

typedef struct {
    uint8_t     node;
    uint32_t    timeout;
}   AckTimer;

static AckTimer ack_timers[MAX_PACKETS];

#define ACK_TIMEOUT_MS 200

static AckTimer* get_ack_timer(uint8_t node)
{
    for (uint8_t i = 0; i < MAX_PACKETS; ++i) {
        AckTimer* at = & ack_timers[i];
        if (at->node == node)
            return at;
    }
    return 0;
} 

static void set_ack_timer(AckTimer* at, uint8_t node)
{
    at->node = node;
    if (node) {
        at->timeout = millis() + ACK_TIMEOUT_MS;
    }
}

static bool waiting_for_ack(uint8_t node)
{
    AckTimer* at = get_ack_timer(node);
    if (!at)
        return false;
    const bool waiting = at->timeout > millis();
    if (!waiting)
        at->node = 0; // free the timer
    return waiting;
}

static void clear_ack_timer(uint8_t node)
{
    AckTimer* at = get_ack_timer(node);
    if (!at)
        return;
    at->node = 0; // free the timer
}

    /*
    *
    */

static Packet* next_wakey_packet()
{
    // Find an allocated packet to send next
    for (int i = 0; i < MAX_PACKETS; ++i) {
        Packet* p = & packets[i];
        if (p == host_packet)
            continue;
        if (is_sleepy(p->node))
            continue;
        if (waiting_for_ack(p->node))
            continue;
        if (p->node != 0)
            return p;
    }
    return 0;
}

static Packet* get_packet(uint8_t dev)
{
    // Find a packet allocated to dev
    for (int i = 0; i < MAX_PACKETS; ++i) {
        Packet* p = & packets[i];
        if (p == host_packet)
            continue;
        if (p->node == dev)
            return p;
    }
    return 0;
}

static uint8_t spare_packets()
{
    uint8_t count = 0;
    // Find a packet allocated to dev
    for (int i = 0; i < MAX_PACKETS; ++i) {
        Packet* p = & packets[i];
        if (p == host_packet)
            continue;
        if (p->node)
            count++;
    }
    return MAX_PACKETS - 1 - count;
}

static Packet* next_host_packet()
{
    // Allocated a packet for the host communication.
    Packet* p = get_packet(0); // any unused packet
    if (p)
        return p;

    // Need to overwrite an allocated packet.
    p = host_packet + 1;
    if (p >= & packets[MAX_PACKETS])
        p = & packets[0];
    return p;
}

    /*
    *
    */

// host->radio messages
#define CMD_UNKNOWN (1<<0)
#define CMD_SLEEPY  (1<<1)

// radio->host messages
#define PRESENT_TEMP            (1<<0)
#define PRESENT_PACKET_COUNT    (1<<1)

static void add_packet_info(Message* msg)
{
    const uint8_t c = spare_packets();
    msg->append(PRESENT_PACKET_COUNT, & c, sizeof(c));
    const uint16_t d = MAX_PACKET_DATA;
    msg->append(0, & d, sizeof(d));
}

static void notify_packet_send()
{
  // Inform host of packet send status
  Message msg(make_mid(), GATEWAY_ID);
  add_packet_info(& msg);
  Bencode::to_host(GATEWAY_ID, (uint8_t*) msg.data(), msg.size());
}

static uint8_t read_data(uint8_t* data, int length) {
  Message msg((void*) data);

  unknown_led.set(0);

  // Check for unknown device bitmap
  uint32_t mask;
  if (msg.extract(CMD_UNKNOWN, & mask, sizeof(mask))) {
    unknown_devs = mask;
    if (mask) {
      unknown_led.set(8000);
    }
  }

  if (msg.extract(CMD_SLEEPY, & mask, sizeof(mask))) {
    sleepy_devs = mask;
  }

  if (!(msg.get_ack()))
    return 0;

  return msg.get_mid();  
}

static int decode_command(uint8_t* data, int length)
{
  const uint8_t mid = read_data(data, length);

  if (!mid)
      return 0;
 
  Message response(mid, GATEWAY_ID);

  // Send Temperature.
  sensors.requestTemperatures();
  const float ft = sensors.getTempCByIndex(0);
  const uint16_t t = int(ft * 100);
  response.append(PRESENT_TEMP, & t, sizeof(t));

  // Add info / status of packet buffer.
  add_packet_info(& response);

  Bencode::to_host(GATEWAY_ID, (uint8_t*) response.data(), response.size());

  return 1;
}

    /*
    *   Timer Interrupt
    */

static void on_timer()
{
    // Flash the lights
    for (LED** led = all_leds; *led; ++led) {
        (*led)->poll();
    }
}

 /*
  *
  */

static Bencode parser;

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

  MsTimer2::set(100, on_timer);
  MsTimer2::start();
}

#define FLASH_PERIOD 1

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

  // Process any pending ACK Requests
  uint8_t mid;
  const uint8_t dev = get_next_ack(& mid);
  if (dev) {
    if (rf12_canSend()) {
      if (is_sleepy(dev)) {
          // send any queued messages
          Packet* pm = get_packet(dev);
          if (pm) {
              tx.set(FLASH_PERIOD);
              rf12_sendStart(pm->node, pm->data, pm->length);
              pm->reset();
              clear_ack(dev);
              // notify host that packet was sent
              notify_packet_send();
              return;
          }
      }

      // ACK with an empty message
      Message msg(mid, dev);
 
      // Set IDENTIFY request if node is unknown
      if (is_unknown(dev)) {
        msg.set_admin();
      }

      // Send ACK packet
      tx.set(FLASH_PERIOD);
      rf12_sendStart(dev, msg.data(), msg.size());
      clear_ack(dev);
      clear_ack_timer(dev);
    }    
    return;
  }

  // read from host rx buffers
  Packet* pm = next_wakey_packet();
  if (pm) {
    const uint8_t node = pm->node;
    if (rf12_canSend()) {
      // transmit message to wakey node
      tx.set(FLASH_PERIOD);
      rf12_sendStart(node, pm->data, pm->length);
      pm->reset();
      // set the waiting for ack timer
      AckTimer* at = get_ack_timer(0);
      if (at) {
        set_ack_timer(at, node);
      }
      // notify host that packet was sent
      notify_packet_send();
    }
  }

  // read any host messages
  if (Serial.available()) {
    if (parser.parse(host_packet, Serial.read())) {
      status_led.set(FLASH_PERIOD);
 
      if (host_packet->node == GATEWAY_ID) {
        // it is for me!!
        decode_command(host_packet->data, host_packet->length);
      } else {
        // leave it in rx buffers and select a new host buffer
        host_packet = next_host_packet();
      }
      parser.reset(host_packet);
    }
  }
}

// FIN
