
#include <avr/interrupt.h>

#include "Arduino.h"
//#include "pins_arduino.h"

#include "solar.h"

  /*
  ;  System Timer
  */

SysTimer::SysTimer()
{
  for (int i = 0; i <  MAX_HANDLERS; ++i)
    m_handlers[i].m_handler = 0;
}
  
bool SysTimer::add_handler(SysTimer::handler fn, void* args)
{
  for (int i = 0; i < MAX_HANDLERS; ++i)
  {
    Handler* h = & m_handlers[i];
    if (!h->m_handler)
    {
        h->m_handler = fn;
        h->m_args = args;
        return true;
    }
  }  
  return false;  
}

void SysTimer::on_interrupt()
{
  for (int i = 0; i <  MAX_HANDLERS; ++i)
  {
    Handler* h = & m_handlers[i];
    if (h->m_handler)
        h->m_handler(h->m_args);
  }    
}
  
void SysTimer::setup()
{
  //Timer2 Overflow Interrupt Enable  
  TIMSK2 = 1 << TOIE2;
}

SysTimer sys_timer;

ISR(TIMER2_OVF_vect) 
{
  sys_timer.on_interrupt();
};

  /*
  ;  Period control
  */

PeriodTimer::PeriodTimer(unsigned int millis)
: m_last_time(0),
  m_period(millis)
{
}

bool PeriodTimer::ready()
{
  const unsigned long t = millis();

  if (t >= m_last_time)  
    if ((m_last_time + m_period) >  t)
      return false;

  if (m_last_time == 0)
    m_last_time = t;  
  else
    m_last_time += m_period;
  return true;
}

//  FIN
