

#include <Arduino.h>
#include <ADNS2610.h>

#define LED 13

byte frame[FRAMELENGTH];

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
  static bool led_state = false;

  readFrame(frame);
  // toggle LED
  led_state = !led_state;
  digitalWrite(LED, led_state ? HIGH : LOW);

  for(int i = 0; i < FRAMELENGTH; i++)
  {
    Serial.print((char) (frame[i] & 0x3F));
    Serial.flush();
  }
  Serial.print(char(0x80)); // end of frame marker
}

// FIN
