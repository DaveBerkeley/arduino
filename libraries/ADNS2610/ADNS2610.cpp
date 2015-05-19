/*  
 Serial driver for ADNS2010, by Conor Peterson (robotrobot@gmail.com)
 Serial I/O routines adapted from Martjin The and Beno?t Rosseau's work.
 Delay timings verified against ADNS2061 datasheet.
 
 The serial I/O routines are apparently the same across several Avago chips.
 It would be a good idea to reimplement this code in C++. The primary difference
 between, say, the ADNS2610 and the ADNS2051 are the schemes they use to dump the data
 (the ADNS2610 has an 18x18 framebuffer which can't be directly addressed).
 
 This code assumes SCLK is defined elsewhere to point to the ADNS's serial clock,
 with SDIO pointing to the data pin.
*/

#include <Arduino.h>

#include <ADNS2610.h>

const byte regConfig    = 0x00;
const byte regStatus    = 0x01;
const byte regPixelData = 0x08;
const byte maskNoSleep  = 0x01;
const byte maskPID      = 0xE0;

void mouseInit(void)
{
  digitalWrite(SCLK, HIGH);
  delayMicroseconds(5);
  digitalWrite(SCLK, LOW);
  delayMicroseconds(1);
  digitalWrite(SCLK, HIGH);
  delay(1025);
  writeRegister(regConfig, maskNoSleep); //Force the mouse to be always on.
}

void dumpDiag(void)
{
  unsigned int val;

  val = readRegister(regStatus);

  Serial.print("Product ID: ");
  Serial.println( (unsigned int)((val & maskPID) >> 5));
  Serial.println("Ready.");
  Serial.flush();
}

void writeRegister(byte addr, byte data)
{
  byte i;

  addr |= 0x80; //Setting MSB high indicates a write operation.

  //Write the address
  pinMode (SDIO, OUTPUT);
  for (i = 8; i != 0; i--)
  {
    digitalWrite (SCLK, LOW);
    digitalWrite (SDIO, addr & (1 << (i-1) ));
    digitalWrite (SCLK, HIGH);
  }

  //Write the data    
  for (i = 8; i != 0; i--)
  {
    digitalWrite (SCLK, LOW);
    digitalWrite (SDIO, data & (1 << (i-1) ));
    digitalWrite (SCLK, HIGH);
  }
}

byte readRegister(byte addr)
{
  byte i;
  byte r = 0;

  //Write the address
  pinMode (SDIO, OUTPUT);
  for (i = 8; i != 0; i--)
  {
    digitalWrite (SCLK, LOW);
    digitalWrite (SDIO, addr & (1 << (i-1) ));
    digitalWrite (SCLK, HIGH);
  }

  pinMode (SDIO, INPUT);  //Switch the dataline from output to input
  delayMicroseconds(110);  //Wait (per the datasheet, the chip needs a minimum of 100 µsec to prepare the data)

  //Clock the data back in
  for (i = 8; i != 0; i--)
  {                             
    digitalWrite (SCLK, LOW);
    digitalWrite (SCLK, HIGH);
    r |= (digitalRead (SDIO) << (i-1) );
  }

  delayMicroseconds(110);  //Tailing delay guarantees >100 µsec before next transaction

  return r;
}


//ADNS2610 dumps a 324-byte array, so this function assumes arr points to a buffer of at least 324 bytes.
void readFrame(byte *arr)
{
  byte *pos;
  byte *uBound;
  unsigned long timeout;
  byte val;

  //Ask for a frame dump
  writeRegister(regPixelData, 0x2A);

  val = 0;
  pos = arr;
  uBound = arr + 325;

  timeout = millis() + 1000;

  //There are three terminating conditions from the following loop:
  //1. Receive the start-of-field indicator after reading in some data (Success!)
  //2. Pos overflows the upper bound of the array (Bad! Might happen if we miss the start-of-field marker for some reason.)
  //3. The loop runs for more than one second (Really bad! We're not talking to the chip properly.)
  while( millis() < timeout && pos < uBound)
  {
    val = readRegister(regPixelData);

    //Only bother with the next bit if the pixel data is valid.
    if( !(val & 64) )
      continue;

    //If we encounter a start-of-field indicator, and the cursor isn't at the first pixel,
    //then stop. ('Cause the last pixel was the end of the frame.)
    if( ( val & 128 ) 
      &&  ( pos != arr) )
      break;

    *pos = val & 63;
    pos++;
  }
}

//  FIN
