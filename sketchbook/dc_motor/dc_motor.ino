
#include "solar.h"
#include "dcmotor.h"

  /*
  ;
  */

#define MOTOR_EN    7
#define LED_DRIVE   13

#define OPTO_A_0    2
#define OPTO_B_0    4
#define END_STOP_0  8
#define MOTOR_DIR_0 10
#define MOTOR_PWM_0 6

#define OPTO_A_1    3
#define OPTO_B_1    5
#define END_STOP_1  12
#define MOTOR_DIR_1 11
#define MOTOR_PWM_1 9

//#define MAX_SPEED 250 // 24V motor
#define MAX_SPEED 200 // 24V motor

#define PID_KP 90
#define PID_KI 0.001
#define PID_KD 0.0

static PID pid_0(PID_KP, PID_KI, PID_KD, MAX_SPEED);
static PID pid_1(PID_KP, PID_KI, PID_KD, MAX_SPEED);

static DcMotor dc_motor_0(& quad_0, & pid_0);
static DcMotor dc_motor_1(& quad_1, & pid_1);

static DcMotor* motors[] = {
  & dc_motor_0,
  & dc_motor_1,
};

static const int end_stops[] = {
  END_STOP_0,
  END_STOP_1,
};

#define NUM_MOTORS 2

  /*
  ;  Setup
  */

void setup() 
{ 
  sys_timer.setup();

  //Serial.begin(9600);
  Serial.begin(115200);

  //  DC Motor control
  bool okay[2];
  okay[0] = motors[0]->open(MOTOR_PWM_0, MOTOR_DIR_0, MOTOR_EN, OPTO_A_0, OPTO_B_0);
  okay[1] = motors[1]->open(MOTOR_PWM_1, MOTOR_DIR_1, MOTOR_EN, OPTO_A_1, OPTO_B_1);

  for (int i = 0; i < NUM_MOTORS; ++i)
  {
    if (!okay[i])
    {
      Serial.print("error opening motor ");
      Serial.print(i);
      Serial.print("\r\n");
      return;
    }
  }

  // Turn on the opto LEDs
  pinMode(LED_DRIVE, OUTPUT);
  digitalWrite(LED_DRIVE, 1);

  Serial.print("starting ...\r\n");
  // TODO : does this need a delay because of glitches
  // caused by the opto sensor LED turning on?

  for (int i = 0; i < NUM_MOTORS; ++i)
  {
    pinMode(end_stops[i], INPUT);
    motors[i]->enable();
    motors[0]->set_count(i);
  }
} 

  /*
  ;  Decode movement commands.
  */

#define COMMAND 0xFF
 
void command_handler()
{
  typedef struct
  {
    unsigned char  m_start;
    unsigned char  m_command;
    unsigned char  m_ack;
    unsigned char  m_index;
    int            m_value;
  }  Message;

  static unsigned char buff[sizeof(Message)];
  static int index = 0;

  if (!Serial.available())
    return;

  const int c = Serial.read();

  buff[index++] = c;

  if (index != sizeof(Message))
    return;
    
  index = 0;
  Message* message = (Message*) buff;
    
  if (message->m_start != 0xFF)
    return;

  DcMotor* motor = motors[message->m_index];

  switch (message->m_command)
  {
    case 0:  
      motor->move_to(message->m_value, message->m_ack);  
      break;
    case 1:  
      motor->set_count(message->m_value); 
      break;
  }
}

  /*
  ;
  */

PeriodTimer opto_timer(50);

void loop() 
{ 
  command_handler();
  
  if (!opto_timer.ready())
    return;

  int value;

  Serial.print("e");
  for (int i  = 0;  i < NUM_MOTORS; ++i)
  {
    value = ! digitalRead(end_stops[i]);
    Serial.print(",");  
    Serial.print(value);
  }

  Serial.print(",c");
  for (int i  = 0;  i < NUM_MOTORS; ++i)
  {
    value = motors[i]->count();
    Serial.print(",");
    Serial.print(value);
  }

  Serial.print(",s");
  for (int i  = 0;  i < NUM_MOTORS; ++i)
  {
    value = motors[i]->speed();
    Serial.print(",");
    Serial.print(value);
  }

  Serial.print(",t");
  for (int i  = 0;  i < NUM_MOTORS; ++i)
  {
    value = motors[i]->target();
    Serial.print(",");
    Serial.print(value);
  }

  Serial.print(",d");
  for (int i  = 0;  i < NUM_MOTORS; ++i)
  {
    value = motors[i]->done();
    Serial.print(",");
    Serial.print(value);
  }

  Serial.print("\r\n");  
}

//  FIN
