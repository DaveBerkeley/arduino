

#include <OneWire.h>
#include <DallasTemperature.h>

// copied from 
// https://github.com/milesburton/Arduino-Temperature-Control-Library/blob/master/examples/Simple/Simple.pde

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
#define PULLUP_PIN 3

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");

  // Start up the library
  sensors.begin();
  
  pinMode(PULLUP_PIN INPUT_PULLUP);
}

void loop(void)
{ 
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  
  Serial.print("Temperature for the device 1 (index 0) is: ");
  Serial.println(sensors.getTempCByIndex(0));
  delay(100);
}

