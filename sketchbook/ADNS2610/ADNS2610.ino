

#include <Arduino.h>
#include <ADNS2610.h>

#define LED 13

byte frame[FRAMELENGTH];

void flipLED(void)
{
  static byte state = false;
  state = !state;
  digitalWrite(LED, state ? HIGH : LOW);
}

void setup()
{
  pinMode(SCLK, OUTPUT);
  pinMode(SDIO, OUTPUT);
  pinMode(LED, OUTPUT);

  Serial.begin(19200);
  Serial.print("ADNS2610\n");

  mouseInit();
  dumpDiag();
}

void loop()
{
  unsigned int s;
  int input;  

  readFrame(frame);
  flipLED();

  for(int i = 0; i < FRAMELENGTH; i++)
  {
    //Serial.print((int) frame[i]);
    //Serial.print(",");
    Serial.print((char) frame[i]);
  }
  //Serial.print("\n");
  Serial.print(char(127));
}

// FIN
