
#include "Arduino.h"

#include "solar.h"
#include "dcmotor.h"

  /*
  ;  PID module
  */

PID::PID(int kp, float ki, int kd, int max_out)
: m_kp(kp),
  m_ki(ki),
  m_kd(kd),
  m_max_out(max_out),
  m_setpoint(0),
  m_integral(0)
{
}

int PID::calc(int input, int output)
{
  const int error = m_setpoint - input;

  if (error && (abs(error) < m_max_out))
    m_integral += error;
  else
    m_integral = 0;

  const float s = (m_kp * float(error)) + (m_integral * m_ki);
  if (s < 0.0)
  {
    if (s < -m_max_out)
      return -m_max_out;
  } else {
    if (s > m_max_out)
      return m_max_out;
  }
  return int(s); 
}

  /*
  ;  Quad Opto Detector.
  */

// 2 interrupt handlers are required because the
// fn attachInterrupt() does not have a void* parameter
// that you can use to set to "this"
void on_interrupt_0()
{
  quad_0.handler();
}

void on_interrupt_1()
{
  quad_1.handler();
}

QuadDetector::QuadDetector()
: m_a(0),
  m_b(0),
  m_count(0)
{
}

bool QuadDetector::open(int a, int b)
{
  m_a = a;
  m_b = b;
  pinMode(m_a, INPUT);    
  pinMode(m_b, INPUT);

  switch (m_a)
  {
    // interrupts only available on these pins:
    case 2: attachInterrupt(0, on_interrupt_0, CHANGE); break;
    case 3: attachInterrupt(1, on_interrupt_1, CHANGE); break;
    default : return false;
  }

  return true;
}

void QuadDetector::handler()
{
  //  Interrupt handler
  const bool a = digitalRead(m_a);
  const bool b = digitalRead(m_b);

  if (a ^ b)
    m_count -= 1;
  else
    m_count += 1;
}

int QuadDetector::count() const
{
  return m_count;
}

void QuadDetector::set_count(int count)
{
  m_count = count;
}

QuadDetector quad_0;
QuadDetector quad_1;

  /*
  ;  DC Motor control class
  */

void motor_irq(void* arg)
{
  DcMotor* motor = (DcMotor*) arg;
  motor->on_interrupt();
}

DcMotor::DcMotor(QuadDetector* quad, PID * pid)
: m_pwm_pin(0),
  m_dir_pin(0),
  m_en_pin(0),
  m_quad(quad),
  m_pid(pid)
{
}

bool DcMotor::open(int pwm_pin, int dir_pin, int en_pin, int a, int b)
{
  m_pwm_pin = pwm_pin;
  m_dir_pin = dir_pin;
  m_en_pin = en_pin;

  // set all pins as outputs
  pinMode(m_pwm_pin, OUTPUT);
  pinMode(m_dir_pin, OUTPUT);
  pinMode(m_en_pin, OUTPUT);

  disable();
  set_speed(0);
    
  if (m_quad)
    if (!m_quad->open(a, b))
        return false;
  
  sys_timer.add_handler(motor_irq, this);
  return true;
}

void DcMotor::set_speed(int speed)
{    
  int s = speed;

  // clip to valid range
  if (m_pid)
  {
    const int max = m_pid->get_max();
    if (s > max)
      s = max;
    else if (s < -max)
      s = -max;
  }

  m_speed = s;
 
  const bool direction = s >= 0;
  // PWM needs a range of 0..255
  const int value = (s >= 0) ? s : (256 + s);

  analogWrite(m_pwm_pin, value);
  digitalWrite(m_dir_pin, direction ? LOW : HIGH);  
}

void DcMotor::enable()
{
  digitalWrite(m_en_pin, HIGH);
}

void DcMotor::disable()
{
  digitalWrite(m_en_pin, LOW);  
}

int DcMotor::count() const
{
  return m_quad ? m_quad->count() : 0;
}

void DcMotor::set_count(int count)
{
  if (m_quad)
    m_quad->set_count(count);
}

void DcMotor::move_to(int count, unsigned char ack)
{
  m_ack = 0;
  m_next_ack = ack;
  m_pid->set_setpoint(count);
}

void DcMotor::on_interrupt()
{
  const int s = m_pid->calc(count(), speed());
  set_speed(s);
}

unsigned char DcMotor::done()
{
  if (m_speed)
    return 0;
  if (count() != m_pid->get_setpoint())
    return 0;
  m_ack = m_next_ack;
  return m_ack;
}

// FIN
