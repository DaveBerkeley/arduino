
// Based on AdaFruit library ...

#include <DHT.h>

#define PIN 4 // DIO Port1 on JeeNode
// Connect Vdd to 3V3 and GDND to GND

#define DHT_TYPE DHT22

DHT dht(PIN, DHT_TYPE);

void setup()
{
  Serial.begin(9600);

  Serial.print("Humidity Sensor v1.0\n");
  dht.begin();
}

void loop()
{
  delay(10000);
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print("%, Temp: ");
  Serial.print(t);
  Serial.print("C\n");
}

// FIN
