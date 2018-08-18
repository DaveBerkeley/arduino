 

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
public:
  Pin()
  : pin(-1),
    timer(0)
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
  
  int get()
  {
    return timer;
  }
};

static Pin pins[PINS];

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
  
static int get_ports(char *buff)
{
  int bits = 0;
  int port = 0;

  for (; *buff; buff++)
  {  
    const char c = *buff;
    if ((c >= '0') && (c <= '9'))
    {
      int p = c - '0';
      bits |= 1 << p;
    }
    
    port += 1;
  }
  
  return bits;
}
  
static void process_line(char *buff)
{
  const int bits = get_ports(buff+1);

  switch (buff[0])
  {
    case '?' :
    {
      //  show status
      for (int i = 0; i < PINS; i++)
      {
          Serial.print(pins[i].get() ? 'X' : '-');
      }
      Serial.print("\r\n");
      return;
    }
    case 'S' : // Set
    {
      for (int i = 0; i < PINS; i++)
      {
        if ((1 << i) & bits)
        {
          pins[i].on();
        }
      }
      break;
    }
    case 'R' : // Reset
    {
      for (int i = 0; i < PINS; i++)
      {
        if ((1 << i) & bits)
        {
          pins[i].off();
        }
      }
      break;
    }
    default :
    {
      Serial.print("error\r\n");
      return;
    }
  }
  Serial.print("okay\r\n");
}
  
static bool command()
{
  if (!Serial.available())
    return false;

  static char buff[32];
  static int idx = 0; 

  int c = Serial.read();
  
  buff[idx] = c;
  idx += 1;
  
  if (idx >= sizeof(buff))
  {
    idx = 0;
    return false;
  }

  buff[idx] = '\0';
  
  if (c == '\n')
  {
    idx = 0;
    process_line(buff);
  }
  
  return true;
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

  while (command())
    ;

  poll();
  delay(100); // ms 
}

// FIN
