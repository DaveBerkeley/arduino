

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

  Serial.begin(38400);
  Serial.println("Serial established.");
  Serial.flush();

  mouseInit();
  dumpDiag();
}

void loop()
{
  unsigned int s;
  int input;  

  readFrame(frame);
  flipLED();

  if( Serial.available() )
  {
    input = Serial.read();
    switch( input )
    {
    case 'f':
      Serial.println("Frame capture.");
      readFrame(frame);
      flipLED();
      Serial.println("Done.");
      break;
    case 'd':
      for( input = 0; input < FRAMELENGTH; input++ )  //Reusing 'input' here
        Serial.print( (byte) frame[input] );
      Serial.print( (byte)127 );
      break;
    }
    Serial.flush();
  }
}

// FIN
