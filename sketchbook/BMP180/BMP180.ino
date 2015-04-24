
// Based on SparkFun example ...

#include <SFE_BMP180.h>
#include <Wire.h>

SFE_BMP180 pressure;

#define ALTITUDE 75.0 // Altitude of home

void setup()
{
  Serial.begin(9600);
  Serial.println("REBOOT");

  // Initialize the sensor (it is important to get calibration values stored on the device).

  if (pressure.begin())
    Serial.println("BMP180 init success");
  else
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
  double T,P,p0,a;

  // Loop here getting pressure readings every 10 seconds.
  delay(5000);  // Pause for 5 seconds.

  // If you want sea-level-compensated pressure, as used in weather reports,
  // you will need to know the altitude at which your measurements are taken.
  // We're using a constant called ALTITUDE in this sketch:
  
  Serial.print("alt,");
  Serial.print(ALTITUDE,0);
  
  // If you want to measure altitude, and not pressure, you will instead need
  // to provide a known baseline pressure. This is shown at the end of the sketch.

  // You must first get a temperature measurement to perform a pressure reading.
  
  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.

  status = pressure.startTemperature();
  if (status == 0)
  {
    Serial.println("error starting temperature measurement\n");
    return;
  }

  // Wait for the measurement to complete:
  delay(status);

  // Retrieve the completed temperature measurement:
  // Note that the measurement is stored in the variable T.
  // Function returns 1 if successful, 0 if failure.

  status = pressure.getTemperature(T);
  if (status == 0) {
    Serial.println("error retrieving temperature measurement\n");
    return;
  }

  // Print out the measurement:
  Serial.print(",temp,");
  Serial.print(T,2);

  // Start a pressure measurement:
  // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.

  status = pressure.startPressure(3);
  if (status == 0)
  {
    Serial.println("error retrieving temperature measurement\n");
    return;
  }

  // Wait for the measurement to complete:
  delay(status);

  // Retrieve the completed pressure measurement:
  // Note that the measurement is stored in the variable P.
  // Note also that the function requires the previous temperature measurement (T).
  // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
  // Function returns 1 if successful, 0 if failure.

  status = pressure.getPressure(P,T);
  if (status == 0)
  {
    Serial.println("error retrieving pressure measurement\n");
    return;
  }

  // Print out the measurement:
  Serial.print(",p,");
  Serial.print(P,2);

  // The pressure sensor returns abolute pressure, which varies with altitude.
  // To remove the effects of altitude, use the sealevel function and your current altitude.
  // This number is commonly used in weather reports.
  // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
  // Result: p0 = sea-level compensated pressure in mb

  p0 = pressure.sealevel(P,ALTITUDE); 
  Serial.print(",sea,");
  Serial.print(p0,2);
  Serial.println("");
}

// FIN
