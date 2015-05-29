

#include <SoftwareSerial.h>
#include <JeeLib.h>
#include <radionet.h>
#include <util/crc16.h>

#define rx 0
#define tx 1
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
        //crc = _crc_xmodem_update(crc, ((const byte*) ptr)[i]);
    }
    return crc;
}

static void saveConfig (byte node_id, byte group) {
    // save to EEPROM
  RF12Config config = { 0 };

  config.nodeId = (node_id & 0x1F) + 0x80; // 868MHz
  config.group = group;
  config.format = RF12_EEPROM_VERSION;

  word crc = calcCrc(& config, sizeof(config)-2);

  config.crc = crc;

  for (int i = 0; i < sizeof(config); ++i)
  {
    byte* b = & ((byte*)& config)[i];
    eeprom_write_byte(RF12_EEPROM_ADDR+i, *b);
  }

  Serial.print(crc, HEX);
  Serial.print(" ");

  crc = calcCrc(& config, sizeof(config));

  Serial.print(crc, HEX);
  Serial.print("\r\n");

  //if (!rf12_config())
  //    Serial.println(PSTR("config failed"));
}

bool validate()
{
  byte config[RF12_EEPROM_SIZE];

  for (int i = 0; i < RF12_EEPROM_SIZE; ++i)
  {
    const byte b = eeprom_read_byte(RF12_EEPROM_ADDR + i);
    config[i] = b;
    Serial.print(b, HEX);
    Serial.print(" ");
  }
  Serial.print("\r\n");
  Serial.print((int)RF12_EEPROM_ADDR, HEX);
  Serial.print("\r\n");

  word crc = calcCrc(config, sizeof(config));
  return crc == 0;
}

  /*
   *
   */

// needed by the watchdog code
EMPTY_INTERRUPT(WDT_vect);

#define led 2

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

  saveConfig(20, 7);
  if (!validate())
  {
    Serial.print("Bad RFM12 config crc\r\n");
  }
  rf12_configSilent();
}

void loop() {
  delay(100);
  static bool led_state;
  setLed(led_state = !led_state);
}

// FIN
