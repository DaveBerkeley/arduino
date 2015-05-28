

#include <SoftwareSerial.h>
#include <JeeLib.h>
#include <radionet.h>

#define rx 0
#define tx 1
SoftwareSerial serial(rx, tx);

static char pins[] = {
  2,
  3, 
  7,  
  10,
  -1,
};

void setup() {

  for (int i = 0; ; i++)
  {
    if (pins[i] == -1)
      break;
    pinMode(pins[i], OUTPUT);    
  }

  serial.begin(9600);
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
  serial.print(state);
  serial.print("\r\n");
}

// FIN
