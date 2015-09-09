
/* 
 * Dave Berkeley (c) 2015 projects2@rotwang.co.uk
 * 
 * Based on Compass5883JeeNodes.ino
 * code for the Modern Device HMC5883 Compass sensor used with JeeNodes
 * Requires the JeeLib library found here: https://github.com/jcw/jeelib
 * code in the public domain.
 */

#include <JeeLib.h>
#include <math.h>

#define HMC5883_WriteAddress 0x1E //  i.e 0x3C >> 1
#define HMC5883_ModeRegisterAddress 0x02
#define HMC5883_ContinuousModeCommand 0x00
#define HMC5883_DataOutputXMSBAddress  0x03

#define regb     0x01
#define regbdata  0x40

class Magnetometer : public DeviceI2C {
    int data[6];
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
}

void loop() {
    int16_t x,y,z;
    magnetometer.convert(& x, &y, &z);

    Serial.print(x);
    Serial.print(" ");
    Serial.print(y);
    Serial.print(" ");
    Serial.print(z);
    Serial.print("\n");

    delay(500);
}

// FIN
