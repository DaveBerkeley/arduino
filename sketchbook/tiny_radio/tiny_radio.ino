
#include <JeeLib.h>

#include <SoftwareSerial.h>

#include <radionet.h>

#include <radio.h>
#include <led.h>

#define LED_PIN     8   // PA2
#define RX_PIN      7   // PA3
#define TX_PIN      3   // PA7

#define RED_LED_PIN  10     // red led source
#define RED_LED_POWER  9      // red led sink

SoftwareSerial serial_port(RX_PIN, TX_PIN);

// Initializes an interrupt routine to allow for easy sleeping
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

typedef struct {
  uint8_t node;
  uint8_t group;
}  Config;

static Config config;

static void read_config()
{
    config.node = 7;
    config.group = 6;
}

  /*
   *
   */

static void led_on(bool on)
{
    digitalWrite(LED_PIN, on);
}

static void red_on(bool on)
{
    digitalWrite(RED_LED_PIN, on);
}

static LED led(led_on);
static LED red(red_on);

    /*
    *
    */

class Talk : public Radio
{
    virtual void on_rx(uint8_t node, uint8_t* msg, uint8_t size)
    {
        red.set(500);
        serial_port.print("Rx(");
        serial_port.print(node);
        serial_port.print(")\r\n");
    }

    virtual void on_tx(uint8_t node)
    {
        static uint8_t i = 0;
        led.set(500);
        serial_port.print("Tx(");
        serial_port.print(node);
        serial_port.print(",");
        serial_port.print(i++);
        serial_port.print(")\r\n");
    }
};

static Talk radio;

    /*
    *
    */

void setup()
{  
  // Run at 8MHz system clock.
  CLKPR = 0x80;
  CLKPR = 0x00; // ck/1

  // configure the software serial port
  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);
  serial_port.begin(9600);  

  // Supply for red LED.
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(RED_LED_POWER, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(PA0, 0);
  digitalWrite(RED_LED_POWER, 0);
  digitalWrite(LED_PIN, 0);

  // TODO : read radio config from EEPROM
  read_config();
  radio.init(config.node, config.group, Radio::MHz_868, 0);
}

    /*
    *
    */

void loop()
{
  static uint32_t counter = 0;
  counter += 1;

  led.poll();
  red.poll();

  radio.poll();

  if (counter != 0x20000L)
    return;

  counter = 0;

  Message m(make_mid(), GATEWAY_ID);
  m.set_ack();

  static uint8_t b = 0;
  b += 1;
  m.append(1 << 0, & b, sizeof(b));
  uint16_t w = 0xface;
  m.append(1 << 1, & w, sizeof(w));

  radio.send(m.get_dest(), (uint8_t*) & m, m.size());
}

//  FIN
