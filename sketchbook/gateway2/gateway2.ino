
#include <JeeLib.h>

#include <radionet.h>
#include <led.h>

  /*
  *  LEDs
  */

#define TX_LED 4
#define RX_LED 5
#define PWR    7

#define FLASH_PERIOD 1000

static void tx_led(bool on) 
{
  digitalWrite(TX_LED, on ? HIGH : LOW);
}

static void rx_led(bool on) 
{
  digitalWrite(RX_LED, on ? HIGH : LOW);
}

static LED rx(rx_led);
static LED tx(tx_led);

  /*
  *
  */
  
static uint8_t my_node;

static char hex(uint8_t n)
{
  if (n < 10)
    return '0' + n;
  return n - 10 + 'A';
}

static void print_hex(uint8_t n)
{
  Serial.print(hex(n >> 4));
  Serial.print(hex(n & 0x0F));
}

static void to_host(int node, uint8_t* data, int bytes)
{
  // send the packet to the host        
  print_hex(node);
  Serial.print(" :");

  for (int i = 0; i < bytes; ++i)
  {
    Serial.print(" ");
    print_hex(data[i] & 0xFF);
  }

  Serial.print("\r\n");
}

static void packet_to_host(void)
{
  to_host((int) rf12_hdr, (uint8_t*) rf12_data, (int) rf12_len);
}

  /*
  *
  */

void setup()
{
  pinMode(TX_LED, OUTPUT);
  pinMode(RX_LED, OUTPUT);
  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, LOW);

  Serial.begin(57600);
  rf12_initialize(my_node = GATEWAY_ID, RF12_868MHZ, 6);
  Serial.print("Hello world!\n");

  tx.set(0);
  rx.set(0);

  //rx.set(8000);
  //tx.set(8000);
}

void loop()
{
  rx.poll();
  tx.poll();

  // send any rx data to the host  
  if (rf12_recvDone() && rf12_crc == 0) {
    rx.set(FLASH_PERIOD);
    packet_to_host();
    // TODO : ack the messages?
  }

  //rx.set(8000);
  delay(10);
}

// FIN
