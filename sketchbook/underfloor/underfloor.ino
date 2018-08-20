 
#include <Arduino.h>

// DHT22 code Based on AdaFruit library ...
#include <DHT.h>
#include <NewPing.h>

#define DHT_TYPE DHT22

#define LED 13

#define TRIGGER_PIN 11
#define ECHO_PIN 12
#define HUMIDITY_PIN 3
#define DHT_POWER 2

#define MAX_DISTANCE 200

static NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
static DHT dht(HUMIDITY_PIN, DHT_TYPE);

  /*
  *
  */

void setup()
{
  pinMode(LED, OUTPUT);
  pinMode(DHT_POWER, OUTPUT);
  digitalWrite(DHT_POWER, HIGH);
  dht.begin();

  Serial.begin(9600);
  Serial.print("Underfloor v 1.0\r\n");
}

void loop()
{
  static bool state = true;
  digitalWrite(LED, state ? HIGH : LOW);

  state = !state;

  const int distance = sonar.ping_cm();
  const float temp = dht.readTemperature();
  const float humidity = dht.readHumidity();
  
  // TODO
  const int pump = 0;
  const int fan = 50;

  Serial.print("d=");
  Serial.print(distance);
  Serial.print(" t=");
  Serial.print(temp);
  Serial.print(" h=");
  Serial.print(humidity);
  Serial.print(" p=");
  Serial.print(pump);
  Serial.print(" f=");
  Serial.print(fan);
  Serial.print("\r\n");

  delay(1000); // ms 
}

// FIN
