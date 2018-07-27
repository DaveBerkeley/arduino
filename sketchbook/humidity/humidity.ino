 

#include <Arduino.h>

// DHT22 code Based on AdaFruit library ...
#include <DHT.h>

#define LED 13

#define PIN 5

#define DHT_TYPE DHT22

  /*
  *
  */

static DHT dht(PIN, DHT_TYPE);

void setup()
{
  pinMode(LED, OUTPUT);
  dht.begin();

  Serial.begin(9600);
  Serial.print("DHT22\r\n");
}

void loop()
{
    static bool state = true;
    digitalWrite(LED, state ? HIGH : LOW);

    state = !state;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    Serial.print("humidity=");
    Serial.print(h);
    Serial.print(" temp=");
    Serial.print(t);
    Serial.print("\r\n");

    delay(1000); // ms 
}

// FIN
