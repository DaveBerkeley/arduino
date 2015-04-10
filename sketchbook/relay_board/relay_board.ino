

// http://playground.arduino.cc/Code/Timer1
#include <TimerOne.h>

// set pin numbers:
static const int pins[4] = { 2, 3, 4, 5 };

static int state[4];
static long pulses[4];
static bool next_state[4];

static void relay(int i, bool set)
{
  state[i] = set;
  pulses[i] = 0;
  digitalWrite(pins[i], !set);
}

  /*
  *
  */

static void xpulse(int device, long period, bool on)
{
  relay(device, on);
  noInterrupts();
  pulses[device] = period;
  next_state[device] = !on;
  interrupts();
}

static void on_timer()
{
  for (int i = 0; i < 4; i++) {
    if (!pulses[i])
      continue;
    if (--pulses[i])
      continue;
    relay(i, next_state[i]);
  }
}

  /*
  *
  */

static long read_int(const char* txt)
{
  long num = 0;
  
  while (*txt) {
    const char c = *txt++;
    if ((c < '0') || (c > '9'))
      return -1;
    num *= 10;
    num += c - '0';    
  }
  
  return num;
}

static int read_command(const char* cmd)
{
  if (*cmd != 'R')
    return -1;
  cmd++;
    
  int device = *cmd - '0';
  if (!((device >= 0) && (device <= 4)))
    return -1;
  cmd++;

  if (*cmd != '=')
    return -1;
  cmd++;

  const int c = *cmd++;
  switch (c) {
    case '?' :  //  query
      break;
      
    case 'P' :  //  pulse
    case 'N' :  //  neg-pulse
    {
      const long p = read_int(cmd);
      if (p == -1)
        return -1;
      cmd = "";
      xpulse(device, p, c == 'P');
      break;
    } 
    case 'T' :  //  toggle
      if (*cmd != '\0')
        return -1;
      relay(device, !state[device]);
      break;
      
    case '0' :  //  off
    case '1' :  //  on
      if (*cmd != '\0')
        return -1;
      relay(device, c - '0');
      break;
      
    default  :
      return -1;
  }

  Serial.print("R");
  Serial.print(device);
  Serial.print("=");
  Serial.print(state[device]);
  Serial.print("\r\n");
}

static int get_serial()
{
  static char buff[16];
  static int idx = 0;

  if (!Serial.available())
    return -1;

  const int c = Serial.read();
  Serial.print(char(c));

  buff[idx++] = c;
  if (idx >= (sizeof(buff)-1)) {
    // too many chars
    idx = 0;
    return -1;
  }

  // read a line
  if ((c != '\r') && (c != '\n'))
    return -1;

  // null out the \n
  buff[idx-1] = '\0';
  idx = 0;
  return read_command(buff);
}

  /*
  *
  */

void setup()
{
  Serial.begin(57600);

  for (int i = 0; i < 4; ++i) {
    pinMode(pins[i], OUTPUT);
    relay(i, 0);
  }
  
  Timer1.initialize(1000);
  Timer1.attachInterrupt(on_timer, 1000);
}

void loop()
{
  get_serial();
}

// FIN
