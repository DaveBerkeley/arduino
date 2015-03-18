/*
 Arduino code to phase control a triac

 Copyright (C) 2012 Dave Berkeley projects@rotwang.co.uk

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 USA
*/

// set pin numbers:
const int ledPin = 13;
const int optoPowerPin = 3;
const int triacPin = 4;
const int optoPin = 2;
const int BtStatePin = 5;
const int BtVccPin = 6;
const int BtEnablePin = 7;

 /*
  * The timer is set to ck/8 prescalar. At 16MHz this gives
  * 500us per count. With 50Hz mains, half cycle is 10ms long.
  * Therefore 20000 timer counts (10000us) equals one half cycle.
  */

#define COUNTS(n) ((n) << 1) // convert us to counts

static int percent;
static long time;
static long start_t = 0;

//  empirically determined overhead for state switch
#define OVERHEAD 18

// Triac pulse train
#define PWM_MARK (100 - OVERHEAD)
#define PWM_SPACE (250 - OVERHEAD)

 /*
  *  Timer / counter config
  */

void clear_timer()
{
  TIMSK1 = 0; // no interrupts
  TCCR1A = 0; // counter off
  TCCR1B = 0;
  TCCR1C = 0;
  TCNT1 = 0;
  OCR1A = 0;
  OCR1B = 0;
}

void set_timer(unsigned long us)
{
  TCCR1A = 0;
  // divide by 8 prescaling
  TCCR1B = 0 << CS22 | 1 << CS21 | 0 << CS20;
  TCCR1B |= 0 << WGM13 | 1 << WGM12; // CTC mode

  OCR1A = COUNTS(us);
  // enable output compare A match interrupt
  TIMSK1 = 1 << OCIE1A;
}

  /*
  *  Auto generated table of % power to timer count.
  * 
  *  Uses http://www.ditutor.com/integrals/integral_sin_squared.html
  *  curve to divide the power into even bins.
  */

static int power_lut_50Hz[100] = {
  0,      1161,   1472,   1694,   1872,   2022,   2155,   2277,   2388,   2494,
  2594,   2683,   2772,   2855,   2933,   3011,   3088,   3161,   3233,   3299,
  3366,   3433,   3494,   3555,   3616,   3677,   3738,   3794,   3855,   3911,
  3966,   4022,   4077,   4133,   4183,   4238,   4288,   4344,   4394,   4444,
  4500,   4550,   4600,   4650,   4700,   4750,   4800,   4850,   4900,   4949,
  5000,   5055,   5105,   5155,   5205,   5255,   5305,   5355,   5405,   5455,
  5505,   5561,   5611,   5661,   5716,   5766,   5822,   5872,   5927,   5983,
  6038,   6094,   6150,   6211,   6266,   6327,   6388,   6450,   6511,   6572,
  6638,   6705,   6772,   6844,   6916,   6994,   7072,   7149,   7233,   7322,
  7411,   7511,   7616,   7727,   7850,   7983,   8133,   8311,   8533,   8844,
};

int percent_to_count(int percent)
{
  if (percent == 100)
    return 0;
  if (percent == 0)
    return 10000;
  return power_lut_50Hz[100 - percent];
}

 /*
  *  State Machine
  */

typedef enum { 
  ZERO,    // zero crossing period
  PHASE,   // start of cycle
  PWM_HI,  // pulse the triac
  PWM_LO   // rest the triac
} triac_state;

static int cycle_state;

void set_state(int next)
{
  clear_timer();
  const unsigned long now = micros();

  switch (next) {
    case ZERO :
      digitalWrite(triacPin, 0);
      digitalWrite(ledPin, 0);
      start_t = now;

    case PHASE :
      digitalWrite(triacPin, 0);
      digitalWrite(ledPin, 1);
      set_timer(percent_to_count(percent));
      break;

    case PWM_HI :
      digitalWrite(triacPin, 1);
      digitalWrite(ledPin, 0);
      set_timer(PWM_MARK);

      if (cycle_state == PHASE) {
        time = now - start_t;
      }
      break;

    case PWM_LO :
      digitalWrite(triacPin, 0);
      digitalWrite(ledPin, 0);
      set_timer(PWM_SPACE);
      break;
  }

  cycle_state = next;
}

  /*
  *  Timer Compare interrupt
  */

ISR(TIMER1_COMPA_vect)
{
  switch (cycle_state)
  {
    case PWM_LO :
    case PHASE :
      set_state(PWM_HI);
      break;
    case PWM_HI :
      set_state(PWM_LO);
      break;
  }
}

 /*
  *  Zero crossing interrupt handler
  */

void on_change()
{
  const int p = digitalRead(optoPin);

  if (p) 
  {
    // end of cycle
    set_state(ZERO);
  } else {
    // start of cycle.
    set_state(PHASE);
  }
}

 /*
  *  Bluetooth subsystem
  */

class BlueTooth {

  int state_pin;
  int bt_connected;
  long last_low_t;

public:
  
  BlueTooth(int state_p)
  : state_pin(state_p),
    bt_connected(0),
    last_low_t(0)
  {
  }

protected:

  virtual void on_connected(void)
  {
    Serial.print("connected\n");
  }

  virtual void on_disconnected(void)
  {
    Serial.print("lost connection\n");
  }

public:

  int connected(void)
  {
    return bt_connected;
  }

  void poll(void)
  {  
    // The state pin is high if BT is connected,
    // but flashes at ~ 4.7Hz otherwise (~210ms).
    const int state = digitalRead(BtStatePin);
    const long now = millis();
    const static int disconnected_max_hi = 300;
  
    if (!state) {
      if (bt_connected)
        on_disconnected();
      bt_connected = 0; // definitely not connected
      last_low_t = now;
    } else {
      const long elapsed = now - last_low_t;
      if (elapsed > disconnected_max_hi) {
        // must be up
        if (!bt_connected)
          on_connected();
        bt_connected = 1;
      }
    }
  }
};

 /*
  * Serial command interface
  */

static int read_command(char* buff)
{
  // command protocol :
  // sxxx set the percent
  if (*buff++ != 's')
    return -1; // bad command
  
  int percent = 0; 

  while (*buff) {
    const char c = *buff++;
    if ((c < '0') || (c > '9'))
      return -1; // bad number
    percent *= 10;
    percent += c - '0';
  }

  if (percent > 100)
    return -1;

  return percent;
}

static int get_serial()
{
  static char buff[16];
  static int idx = 0;

  if (!Serial.available())
    return -1;

  const int c = Serial.read();

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

BlueTooth bt(BtStatePin);

void setup() 
{
  pinMode(ledPin, OUTPUT);
  pinMode(optoPin, INPUT);
  pinMode(optoPowerPin, OUTPUT);
  pinMode(triacPin, OUTPUT);
  pinMode(BtStatePin, INPUT);
  pinMode(BtVccPin, OUTPUT);
  pinMode(BtEnablePin, OUTPUT);

  // initialise the triac control system
  clear_timer();
  percent = 0;
  set_state(ZERO);

  attachInterrupt(0, on_change, CHANGE);

  static int test = 0;

  if (!test) {
    // switch on the zero crossing detector
    digitalWrite(optoPowerPin, 1);
    // switch on the BlueTooth device
    digitalWrite(BtVccPin, 1);
    // and enable it
    digitalWrite(BtEnablePin, 0);
  }

  Serial.begin(9600);
}

void loop()
{
  bt.poll();

  const int p = get_serial();
  if (p != -1)
      percent = p;

  static long last = 0;
  const long now = millis();

  if ((last + 1000) > now)
    return;

  last = now;
  Serial.print(percent);
  Serial.print("\r\n");
}

// FIN
