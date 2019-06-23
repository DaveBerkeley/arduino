 
#include <stdlib.h>

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

// TODO
static int pump = 0;
static int fan = 50;

  /*
  *
  */

class Command
{
  enum cmd { UNKNOWN, FAN, PUMP, };
  
  enum cmd cmd;
  int value;
  char buff[16];
  int idx;

public:
  Command()
  : idx(0)
  {
  }

  bool parse(const char *line)
  {
    cmd = UNKNOWN;
    switch (*line++)
    {
      case 'p' :
      {
        //  pump
        cmd = PUMP;
        value = atoi(line);
        break;
      }
      case 'f' :
      {
        //  fan
        cmd = FAN;
        value = atoi(line);
        break;
      }
      default :
      {
        // error
        return false;
      }
    }
    
    return true;
  }
  
  bool command()
  {
    if (!Serial.available())
    {
      return false;
    }  
    
    const int c = Serial.read();
    buff[idx] = c;
    idx += 1;
    if (idx >= sizeof(buff))
    {
      idx = 0;
      return true;
    }

    buff[idx] = '\0';
    
    if (c != '\n')
      return true;

    idx = 0;
    
    switch (buff[0])
    {
      case 'f' :
      {
        int value = atoi(& buff[1]);
        // TODO : validate etc.
        fan = value;
        break;
      }
      case 'p' :
      {
        int value = atoi(& buff[1]);
        // TODO : validate etc.
        pump = value;
        break;
      }
      default:
        return false;
    }
    
    return true;
  }
};

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
  
  static Command cmd;

  while (cmd.command())
    ;

  const int distance = sonar.ping_cm();
  const float temp = dht.readTemperature();
  const float humidity = dht.readHumidity();

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
