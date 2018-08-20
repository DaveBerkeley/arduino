 
#include <Arduino.h>

#include <NewPing.h>

#define LED 13

#define TRIGGER_PIN 11
#define ECHO_PIN 12
#define MAX_DISTANCE 200

static NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

  /*
  *
  */

void setup()
{
  pinMode(LED, OUTPUT);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.begin(9600);
  Serial.print("Underfloor v 1.0\r\n");
}

void loop()
{
  static bool state = true;
  digitalWrite(LED, state ? HIGH : LOW);

  state = !state;

  static int count = 0;
  Serial.print(sonar.ping_cm());
  Serial.print(" hello\r\n");

  delay(500); // ms 
}

// FIN
