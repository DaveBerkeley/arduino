

#include <Servo.h> 

#define SERVO_PIN 9
 
Servo servo;
 
void setup() 
{ 
  Serial.begin(9600);

  servo.attach(SERVO_PIN);
} 

static void process_line(const char* line)
{
  int n = 0;
  char c;
  
  while (c = *line++) {
    if ((c < '0') || (c > '9'))
      break;
    n *= 10;
    n += c - '0';
  }
  
  Serial.print(n);
  Serial.print("\r\n");
  
  servo.write(n);
}

void loop() 
{ 
  int pos;  
  static char buff[16];
  static char end;
  static int idx = 0;

  int c = Serial.read();
  
  if (c == -1)
    return;
  
  if (idx >= sizeof(buff)) {
    //Serial.print("overrun\n");
    buff[idx = 0] = '\0';
    return;
  }
  
  buff[idx++] = c;
  buff[idx] = '\0';

  if (c == '\r') {
    process_line(buff);
    buff[idx = 0] = '\0';
  }
} 

//  FIN
