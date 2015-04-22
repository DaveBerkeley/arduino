
#ifndef DCMOTOR_H
#define DCMOTOR_H

    /*
     ;  PID Controller
     */

class PID
{
  int m_kp;
  float m_ki;
  int m_kd;
  int m_max_out;

  int m_setpoint;
  int m_integral;

public:
  PID(int kp, float ki, int kd, int max_out);

  void set_setpoint(int sp) { m_setpoint = sp; }
  int get_setpoint() const { return m_setpoint; }
  int get_max() const { return m_max_out; }

  int calc(int input, int output);
};

  /*
  ;  Quad Opto Detector.
  */

class QuadDetector
{
private:
  int  m_a;
  int  m_b;
  int  m_count;

public:
  QuadDetector();

  bool open(int a, int b);
  void handler();
  int count() const;
  void set_count(int count);
};

extern QuadDetector quad_0;
extern QuadDetector quad_1;

  /*
  ;  DC Motor control class
  */

class DcMotor
{
private:
  int  m_pwm_pin;
  int  m_dir_pin;
  int  m_en_pin;
  QuadDetector* m_quad;
  PID* m_pid;
  int  m_speed;
  unsigned char m_next_ack;
  unsigned char m_ack;

public:
  DcMotor(QuadDetector* quad, PID* pid);

  bool open(int pwm_pin, int dir_pin, int en_pin, int a, int b);

  void set_speed(int speed);
  void move_to(int count, unsigned char ack);

  void enable();
  void disable();
  int count() const;
  int speed() const { return m_speed; }
  int target() const { return m_pid->get_setpoint(); }
  void set_count(int count);
  unsigned char done();

  void on_interrupt();
};

#endif // DCMOTOR_H

// FIN
