/*  
 Serial driver for ADNS2010, by Conor Peterson (robotrobot@gmail.com)
 Serial I/O routines adapted from Martjin The and Beno?t Rosseau's work.
 Converted to C++ class by Dave Berkeley projects2@rotwang.co.uk
 Delay timings verified against ADNS2061 datasheet.
 
 The serial I/O routines are apparently the same across several Avago chips.
 The primary difference between, say, the ADNS2610 and the ADNS2051 
 are the schemes they use to dump the data
 (the ADNS2610 has an 18x18 framebuffer which can't be directly addressed).
*/

#include <Arduino.h>

#include <ADNS2610.h>

const byte regConfig    = 0x00;
const byte regStatus    = 0x01;
const byte regPixelData = 0x08;
const byte maskNoSleep  = 0x01;
const byte maskPID      = 0xE0;

    /*
     *
     */

MouseCam::MouseCam(uint8_t sdio, uint8_t sck)
:   m_sdio(sdio), 
    m_sck(sck)
{
}

    /*
     *
     */

bool MouseCam::init()
{
  pinMode(m_sck, OUTPUT);
  //pinMode(SDIO, OUTPUT);
  digitalWrite(m_sck, HIGH);
  delayMicroseconds(5);
  digitalWrite(m_sck, LOW);
  delayMicroseconds(1);
  digitalWrite(m_sck, HIGH);
  delay(1025);
  writeRegister(regConfig, maskNoSleep); //Force the mouse to be always on.
  return true;
}

    /*
     *
     */

uint8_t MouseCam::getId(void)
{
  unsigned int val;

  val = readRegister(regStatus);
  return (val & maskPID) >> 5;
}

    /*
     *
     */

void MouseCam::writeRegister(byte addr, byte data)
{
  byte i;

  addr |= 0x80; //Setting MSB high indicates a write operation.

  //Write the address
  pinMode (m_sdio, OUTPUT);
  for (i = 8; i != 0; i--)
  {
    digitalWrite (m_sck, LOW);
    digitalWrite (m_sdio, addr & (1 << (i-1) ));
    digitalWrite (m_sck, HIGH);
  }

  //Write the data    
  for (i = 8; i != 0; i--)
  {
    digitalWrite (m_sck, LOW);
    digitalWrite (m_sdio, data & (1 << (i-1) ));
    digitalWrite (m_sck, HIGH);
  }
}

    /*
     *
     */

byte MouseCam::readRegister(byte addr)
{
  byte i;
  byte r = 0;

  //Write the address
  pinMode (m_sdio, OUTPUT);
  for (i = 8; i != 0; i--)
  {
    digitalWrite (m_sck, LOW);
    digitalWrite (m_sdio, addr & (1 << (i-1) ));
    digitalWrite (m_sck, HIGH);
  }

  pinMode (m_sdio, INPUT);  //Switch the dataline from output to input
  delayMicroseconds(110);  //Wait (per the datasheet, the chip needs a minimum of 100 µsec to prepare the data)

  //Clock the data back in
  for (i = 8; i != 0; i--)
  {                             
    digitalWrite (m_sck, LOW);
    digitalWrite (m_sck, HIGH);
    r |= (digitalRead (m_sdio) << (i-1) );
  }

  delayMicroseconds(110);  //Tailing delay guarantees >100 µsec before next transaction

  return r;
}

    /*
     *
     */

//ADNS2610 dumps a 324-byte array, so this function assumes arr points to a buffer of at least 324 bytes.
bool MouseCam::readFrame(byte *frame, void (*idle)(void*), void* arg)
{
  //Ask for a frame dump
  writeRegister(regPixelData, 0x2A);

  const unsigned long timeout = millis() + 1000;

  int idx = 0;
  while (idx <= FRAMELENGTH)
  {
    if (millis() > timeout) {
        //  Timeout error
        return false;
    }

    const byte val = readRegister(regPixelData);

    // loop until the data is valid
    if (!(val & 0x40) )
    {
      if (idle)
          idle(arg);
      continue;
    }

    //  Valid frame when EndOfFrame detected and all pixels acquired.
    if ((val & 0x80) && (idx == FRAMELENGTH)) {
      return true;
    }

    *frame++ = val & 0x3F;
    idx += 1;
  }

  // EndOfFrame not seen
  return false;
}

//  FIN
