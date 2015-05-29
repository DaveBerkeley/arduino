

#include <SoftwareSerial.h>
#include <JeeLib.h>
#include <radionet.h>
#include <util/crc16.h>

#define led 3

#define rx 0
#define tx 7
SoftwareSerial serial(rx, tx);

#if defined(__AVR_ATtiny84__)
SerialX Serial(serial);
#endif

typedef struct {
    byte nodeId;            // used by rf12_config, offset 0
    byte group;             // used by rf12_config, offset 1
    byte format;            // used by rf12_config, offset 2
    byte flags;
    word frequency_offset;  // used by rf12_config, offset 4
    byte pad[RF12_EEPROM_SIZE-8];
    word crc;
} RF12Config;

static word calcCrc (const void* ptr, byte len) {
    word crc = ~0;
    for (byte i = 0; i < len; ++i) {
        crc = _crc16_update(crc, ((const byte*) ptr)[i]);
    }
    return crc;
}

static bool validate()
{
  byte config[RF12_EEPROM_SIZE];

  for (int i = 0; i < RF12_EEPROM_SIZE; ++i)
  {
    config[i] = eeprom_read_byte(RF12_EEPROM_ADDR + i);
  }

  return calcCrc(config, sizeof(config)) == 0;
}

static void saveConfig (byte node_id, byte group) {
    // save to EEPROM
  RF12Config config = { 0 };

  config.nodeId = (node_id & 0x1F) + 0x80; // 868MHz
  config.group = group;
  config.format = RF12_EEPROM_VERSION;
  config.frequency_offset = 1600;

  word crc = calcCrc(& config, sizeof(config)-2);

  config.crc = crc;

  byte* b = (byte*) & config;
  for (int i = 0; i < sizeof(config); ++i)
  {
    eeprom_write_byte(RF12_EEPROM_ADDR+i, *b++);
  }

  if (!validate())
  {
    Serial.println(PSTR("Bad RFM12 config crc"));
  }

  if (!rf12_config())
  {
    Serial.println(PSTR("config failed"));
  }
}

 /*
  * Build a data Message 
  */

// node -> gateway data
#define PRESENT_TEMPERATURE (1 << 1)

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
static uint16_t sleep_count = 0;

void make_message(Message* msg, int msg_id, bool ack) 
{
  msg->reset();
  msg->set_dest(GATEWAY_ID);
  msg->set_mid(msg_id);
  if (ack)
    msg->set_ack();

  const uint16_t t = 1234; // temperature
  msg->append(PRESENT_TEMPERATURE, & t, sizeof(t));
}


  /*
   *
   */

// needed by the watchdog code
EMPTY_INTERRUPT(WDT_vect);

void setLed(bool state)
{
  digitalWrite(led, state ? LOW : HIGH);
}

static const char* banner = "Tiny Device v1.0";

  /*
   * 
   */

void setup() {

  pinMode(led, OUTPUT);  
  setLed(false);

  Serial.begin(9600);  
  Serial.println(banner);

  saveConfig(20, 212);
  uint8_t node = rf12_configSilent();
  rf12_configDump();
  state = START;
  Serial.print("node ");
  Serial.print(node);
  Serial.print("\r\n");
}

void loop() {
  static uint16_t changes = 0;

  if (1) {
    static bool b;
    static int c;
    Serial.print(c++);
    Serial.print(" loop\r\n");
    setLed(b = !b); // show we are awake
    delay(1000);
    send_text(banner, ack_id, false);
    rf12_sendWait(0);
    return;
  }

  return;

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
            ;//sleep();
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
    ;//sleep();
    return;
  }

  if (state == SLEEP) {
    //static uint16_t last_changes = -1;
    //if (changes != last_changes) {
      // PIR sensor has changed state
      Serial.println("send");
      //last_changes = changes;
      state = SENDING;
      retries = ACK_RETRIES;
      sleep_count = 0;
      make_message(& message, make_mid(), true);      
      // turn the radio on
      rf12_sleep(-1);
      return;
    //}


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
            ;//sleep();
    }
  }
}

// FIN
