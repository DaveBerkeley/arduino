 

#include <Arduino.h>

#define LED 13

#define PINS 7

static const int pin_numbers[PINS] = {
  2, 3, 4, 5, 6, 7, 8,
};

class Pin
{
  int pin;
  int timer;
  int start;
public:
  Pin()
  : pin(-1),
    timer(0),
    start(11)
  {
  }
  
  void init(int p)
  {
    pin = p;
    digitalWrite(p, HIGH);
    pinMode(p, OUTPUT);
  }
  
  void on()
  {
    digitalWrite(pin, LOW);
    timer = 5;
  }
 
  void off()
  {
    digitalWrite(pin, LOW);
    timer = 25;
  }
  
  void poll()
  {
    // called every 100ms
    if (!timer)
      return;
      
    timer -= 1;
    if (!timer)
    {
      digitalWrite(pin, HIGH);
    }
  }
  
  bool starting()
  {
    if (!start)
      return false;
      
    start -= 1;
    
    if (start == 10)
    {
      on();
    }
      
    if (start == 0)
    {
      off();
    }
      
    return true;
  }
};

static Pin pins[PINS];

static void set_all(bool on)
{
  for (int i = 0; i < PINS; i++)
  {
    if (on)
      pins[i].on();
    else
      pins[i].off();
  }
}

static void poll()
{
  for (int i = 0; i < PINS; i++)
  {
    pins[i].poll();
  }
}

  /*
  *
  */

void setup()
{
  pinMode(LED, OUTPUT);
  
  for (int i = 0; i < PINS; i++)
  {
    pins[i].init(pin_numbers[i]);
  }

  Serial.begin(9600);
  Serial.print("USB Control v 1.0\r\n");
}

void loop()
{
  static bool state = true;
  digitalWrite(LED, state ? HIGH : LOW);

  state = !state;

  static int count = 0;
  Serial.print(count++);
  Serial.print(" hello\r\n");

  static int start = 0;

  if (start != -1)
  {
    if (!pins[start].starting())
    {
      start += 1;
      if (start >= PINS)
        start = -1;
    }
  }

  poll();
  delay(100); // ms 
}

// FIN
