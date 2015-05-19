

#include <Arduino.h>
#include <ADNS2610.h>

#define DLEDG 9
#define DLEDR 10
#define DLEDY 11
#define DLEDPERF 13

byte frame[FRAMELENGTH];

void flipLED(void)
{
  static byte flop;
  flop = !flop;
  digitalWrite(DLEDY, flop ? HIGH : LOW);
}

void setup()
{
  pinMode(SCLK, OUTPUT);
  pinMode(SDIO, OUTPUT); 

  pinMode(DLEDY, OUTPUT);
  pinMode(DLEDR, OUTPUT);
  pinMode(DLEDG, OUTPUT);
  pinMode(DLEDPERF, OUTPUT);

  flash(DLEDY, 1);
  flash(DLEDR, 1);
  flash(DLEDG, 1);
  flash(DLEDPERF, 1);

  flop = false;

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

  readFrame(frame, flipLED);

  if( Serial.available() )
  {
    input = Serial.read();
    switch( input )
    {
    case 'f':
      Serial.println("Frame capture.");
      readFrame(frame, flipLED);
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

void flash(byte pin, byte nTimes)
{
  while( nTimes-- )
  {
    digitalWrite(pin, HIGH);
    delay(120);
    digitalWrite(pin, LOW);
    delay(80);
  } 
}

// FIN
