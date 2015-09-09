
/* Compass5883JeeNodes.ino
 * code for the Modern Device HMC5883 Compass sensor used with JeeNodes
 * Requires the JeeLib library found here: https://github.com/jcw/jeelib
 * code in the public domain
 */

#include <JeeLib.h>
#include <math.h>

#define HMC5883_WriteAddress 0x1E //  i.e 0x3C >> 1
#define HMC5883_ModeRegisterAddress 0x02
#define HMC5883_ContinuousModeCommand 0x00
#define HMC5883_DataOutputXMSBAddress  0x03

int regb=0x01;
int regbdata=0x40;

const int portForCompass = 1;  // change this number to port number you are using for sensor
PortI2C myBus (portForCompass);
//DeviceI2C compass (myBus, HMC5883_WriteAddress); 

class Magnetometer : public DeviceI2C {
    int outputData[6];
    PortI2C& m_port;

public:
    Magnetometer(PortI2C& port) 
    :  m_port(port),
       DeviceI2C(port, HMC5883_WriteAddress)
    {
    }
 
    void convert(int* x, int* y, int* z)
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
          outputData[i] = read(0);
      }
      outputData[5] = read(1); 
      stop();

      *x = (outputData[0] << 8) | outputData[1];
      *z = (outputData[2] << 8) | outputData[3];
      *y = (outputData[4] << 8) | outputData[5];
    }
};

Magnetometer magnetometer(myBus);

void setup () {
    Serial.begin(57600);

    if (magnetometer.isPresent () == 1){
        Serial.print("HMC5883 compass found on Port ");
        Serial.println(portForCompass);
    }
    else {
        Serial.print("No compass found on Port ");
        Serial.print(portForCompass);
        Serial.println(", check port setting, and orienation");
    }

}

void loop() {
    delay(500);

    int x,y,z;
    magnetometer.convert(& x, &y, &z);

    Serial.print(x);
    Serial.print(" ");
    Serial.print(y);
    Serial.print(" ");
    Serial.print(z);
    Serial.print("\n");
}

// FIN
