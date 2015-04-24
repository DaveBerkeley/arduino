
// Based on SparkFun example ...

#include <SFE_BMP180.h>
#include <Wire.h>

SFE_BMP180 pressure;

#define ALTITUDE 75.0 // Altitude of home

void setup()
{
  Serial.begin(9600);

  // Initialize the sensor (it is important to get calibration values stored on the device).

  if (!pressure.begin())
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.

    Serial.println("BMP180 init fail\n\n");
    while(1); // Pause forever.
  }
}

void loop()
{
  char status;
  double t, p, sea, a;

  delay(10000);

  status = pressure.startTemperature();
  if (status == 0)
  {
    Serial.println("error starting temperature measurement");
    return;
  }

  // Wait for the measurement to complete:
  delay(status);

  // Retrieve the completed temperature measurement:
  // Note that the measurement is stored in the variable T.
  // Function returns 1 if successful, 0 if failure.

  status = pressure.getTemperature(t);
  if (status == 0) {
    Serial.println("error retrieving temperature measurement");
    return;
  }

  status = pressure.startPressure(3);
  if (status == 0)
  {
    Serial.println("error retrieving temperature measurement");
    return;
  }

  // Wait for the measurement to complete:
  delay(status);

  status = pressure.getPressure(p,t);
  if (status == 0)
  {
    Serial.println("error retrieving pressure measurement");
    return;
  }

  sea = pressure.sealevel(p,ALTITUDE); 

  // Print out the measurement:
  Serial.print("temp,");
  Serial.print(t,2);

  Serial.print(",p,");
  Serial.print(p,2);

  Serial.print(",sea,");
  Serial.print(sea,2);

  Serial.print(",alt,");
  Serial.print(ALTITUDE,0);
  
  Serial.println("");
}

// FIN
