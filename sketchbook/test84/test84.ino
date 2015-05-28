

//#include <Arduino.h>
#include <SoftwareSerial.h>
#include <JeeLib.h>
#include <radionet.h>

#define rx 0
#define tx 1
SoftwareSerial serial(rx, tx);

#if defined(__AVR_ATtiny84__)
SerialX Serial(serial);
#endif

static char pins[] = {
  2,
  //3, 
  //7,  
  //10,
  -1,
};

static const char* banner = "Tiny Device v1.0";

void setup() {

  for (int i = 0; ; i++)
  {
    if (pins[i] == -1)
      break;
    pinMode(pins[i], OUTPUT);    
  }

  Serial.begin(9600);  
  Serial.println(banner);
}

void loop() {
  delay(100);

  static int state;
  for (int i = 0; ; i++) {
    if (pins[i] == -1)
      break;
    digitalWrite(pins[i], (state & (1 << i)) ? HIGH : LOW);
  }
  state += 1;
  Serial.print(state);
  Serial.print("\r\n");
}

// FIN
