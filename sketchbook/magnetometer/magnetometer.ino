
/*
 Copyright (C) 2015 Dave Berkeley projects2@rotwang.co.uk

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 USA
*/

/* 
 * Based on Compass5883JeeNodes.ino
 * code for the Modern Device HMC5883 Compass sensor used with JeeNodes
 * Requires the JeeLib library found here: https://github.com/jcw/jeelib
 * code in the public domain.
 */

#include <JeeLib.h>

#include <radiodev.h>
#include <radioutils.h>

  /*
   *
   */

// needed by the watchdog code
EMPTY_INTERRUPT(WDT_vect);

  /*
   *  HMC5883 3-axis Magnetometer
   */

#define HMC5883_WriteAddress 0x1E //  i.e 0x3C >> 1
#define HMC5883_ModeRegisterAddress 0x02
#define HMC5883_ContinuousModeCommand 0x00
#define HMC5883_DataOutputXMSBAddress  0x03

#define regb     0x01
#define regbdata  0x40

  /*
   *
   */

class Magnetometer : public DeviceI2C {
    int16_t data[6];
    PortI2C& m_port;

public:
    Magnetometer(PortI2C& port) 
    :  m_port(port),
       DeviceI2C(port, HMC5883_WriteAddress)
    {
    }
 
    void convert(int16_t* x, int16_t* y, int16_t* z)
    {
      int result = m_port.start(HMC5883_WriteAddress);

      send();
      write(regb);   
      write(regbdata);   
      stop();

      send();
      write(HMC5883_ModeRegisterAddress);
      write(HMC5883_ContinuousModeCommand);
      receive();
      for (int i=0; i<5; i++){ 
          data[i] = read(0);
      }
      data[5] = read(1); 
      stop();

      *x = (data[0] << 8) | data[1];
      *z = (data[2] << 8) | data[3];
      *y = (data[4] << 8) | data[5];
    }
};

  /*
  *
  */

#define JEEPORT 1

PortI2C myBus (JEEPORT);

Magnetometer magnetometer(myBus);

// node -> gateway data
#define PRESENT_TEMPERATURE (1 << 1)
#define PRESENT_VCC         (1 << 2)
#define PRESENT_X           (1 << 3)
#define PRESENT_Y           (1 << 4)
#define PRESENT_Z           (1 << 5)

  /*
  *
  */

class MagnetometerRadio : public RadioDev
{
public:

  MagnetometerRadio()
  : RadioDev(GATEWAY_ID, 1)
  {
  }

  virtual void init()
  {
    RadioDev::init();

    // use the 1.1V internal ref for the ADC
    analogReference(INTERNAL);
    
    //pinMode(PULLUP_PIN, INPUT_PULLUP);
  }

  virtual const char* banner()
  {
    return "Magnetometer v1.0";
  }

  virtual void append_message(Message* msg)
  {
    const uint16_t vcc = read_vcc();
    msg->append(PRESENT_VCC, & vcc, sizeof(vcc));
    
    int16_t x, y, z;
    magnetometer.convert(& x, & y, &z);

    msg->append(PRESENT_X, & x, sizeof(x));
    msg->append(PRESENT_Y, & y, sizeof(y));
    msg->append(PRESENT_Z, & z, sizeof(z));
  }

  virtual void loop(void)
  {
    radio_loop(32767);
  }
};

static MagnetometerRadio radio;

  /*
  *
  */

void setup () {
    Serial.begin(57600);

    if (!magnetometer.isPresent ()){
        Serial.print("No compass found on Port ");
        Serial.print(JEEPORT);
        Serial.print("\n");
    }
    
    radio.init();
}

void loop() {
    radio.loop();
}

// FIN
