
#ifndef SOLAR_H
#define SOLAR_H

  /*
  ;  System Timer
  */

#define MAX_HANDLERS 10

class SysTimer
{
private:
  typedef void (*handler)(void*);

  typedef struct 
  {
    handler  m_handler;
    void*    m_args;    
  } Handler;

  Handler m_handlers[MAX_HANDLERS];
public:

  SysTimer();

  bool add_handler(handler fn, void* args);
  void on_interrupt();
  void setup();
};

extern SysTimer sys_timer;

  /*
  ;  Period control
  */

class PeriodTimer
{
  unsigned long m_last_time;
  unsigned int m_period;

public:
  PeriodTimer(unsigned int millis);
  bool ready();
};

#endif // SOLAR_H

//  FIN
