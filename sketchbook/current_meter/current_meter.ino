
/*
 Arduino code to implement a mains power meter

 Copyright (C) 2012 Dave Berkeley projects@rotwang.co.uk
 http://www.rotwang.co.uk/projects/projects.html 

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

#include <SPI.h>

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

#define COUNTS(x) ((x) << 1)

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
  *  Queue
  */

class Queue
{
  int m_size;
  int m_item_size;
  int m_in, m_out;
  char* m_data;
public:
  Queue(int size, int item_size)
  : m_size(size),
    m_item_size(item_size),
    m_in(0),
    m_out(0)
  {
    m_data = (char*) calloc(size, item_size);
  }

  ~Queue()
  {
    free(m_data);
  }

  int put(void* data)
  {
    int next = m_in + 1;
    if (next >= m_size)
      next = 0;
    if (next == m_out)
      return -1;
    memcpy(& m_data[m_in * m_item_size], data, m_item_size);
    m_in = next;
    return 0;
  }

  int get(void* data)
  {
    if (m_in == m_out)
      return -1;
    int next = m_out + 1;
    if (next >= m_size)
      next = 0;
    memcpy(data, & m_data[m_out * m_item_size], m_item_size);
    m_out = next;
    return 0;    
  }

  int empty()
  {
    return m_in == m_out;
  }
};

 /*
  *  SPI base class
  */

class SPI_core
{
private:
  static int spi_done;
  
protected:
  int m_enable;
  int m_pin;

  SPI_core(int pin, int active_hi)
  : m_enable(active_hi),
    m_pin(pin)
  {
    if (!spi_done)
      SPI.begin();
    spi_done += 1;
  }

public:
  void init(void)
  {
    digitalWrite(m_pin, !m_enable);
    pinMode(m_pin, OUTPUT);
  }

  unsigned int write16(int b1, int b2)
  {
    digitalWrite(m_pin, m_enable);
    const int hi = SPI.transfer(b1);
    const int lo = SPI.transfer(b2);
    digitalWrite(m_pin, !m_enable);
    return lo + (hi << 8);
  }
};

int SPI_core::spi_done = false;

 /*
  * Programmable Gain Amplifier MCP6S9X
  */

class MCP6S9X_PGA : public SPI_core
{
public:
  MCP6S9X_PGA(int cs_pin, int active_hi) 
  : SPI_core(cs_pin, active_hi)
  {     
  }

  void set_chan(int chan)
  {
    write16(0x41, chan);
  }

  int set_gain(int gain)
  {
    int code = 0;
    switch (gain)
    {
      case 1:  code = 0;  break;
      case 2:  code = 1;  break;
      case 4:  code = 2;  break;
      case 8:  code = 4;  break;
      case 16: code = 6;  break;
      case 32: code = 7;  break;
      default: return -1;
    }
    write16(0x40, code);
    return  0;
  }
};

 /*
  * MCP300x SPI 10-bit ADC running over opto interface
  */

class MCP300x : public SPI_core
{
  // port to enable pull-up on opto-SPI
  int m_opto_pin;

public:
  MCP300x(int pin, int active_hi, int opto_pin) 
  : SPI_core(pin, active_hi),
    m_opto_pin(opto_pin)
  {
    digitalWrite(m_opto_pin, 1);
  }

  unsigned int convert(int chan)
  {
    // enable the opto-interface pull-up
    pinMode(m_opto_pin, OUTPUT);

    // delay start of conversion by one clock
    const int result = write16(0x60 + ((chan & 0x01) << 4), 0);
    pinMode(m_opto_pin, INPUT);

    return result & 0x3FF;
  }
};

 /*
  *  Built in ADC 
  */

class BuiltInAdc
{
public:

  void init()
  {
    // prescale = /128. 16MHz/128 = 125kHz
    ADCSRA = 0x87; 
    ADCSRB = 0;
  }

  void set_chan(int chan)
  {
    // Set the mux channel
    // 0x40 = AVcc reference, right justify
    ADMUX = 0x40 + chan;
  }

  void start_conversion()
  {
    ADCSRA |= (1 << ADSC);
  }

  unsigned int read()
  {
    // wait for the conversion to complete
    while (ADCSRA & (1 << ADSC))
      ;
    // must read ADCL first, which freezes ADCH
    const int lo = ADCL;
    const int hi = ADCH;
    return lo + (hi << 8);
  }
};

 /*
  *
  */

#define ADC_CE_PIN 8
#define OPTO_CE_PIN 9

#define PGA_CS_PIN 5
#define BT_POWER 4

// channel select for SPI ADC (volts)
#define V_MAINS 0
#define V_REF 1
// channel select for CT (current)
#define I_MAINS 0
#define I_REF 1

// Hardware objects

static MCP6S9X_PGA i_amp(PGA_CS_PIN, 0);
static MCP300x v_adc(ADC_CE_PIN, 0, OPTO_CE_PIN);
static BuiltInAdc i_adc;

// Phase Locked loop used to lock samples to multiple of mains cycle

#define TIMER_SHIFT 4
#define TIMER(n)    ((n) << TIMER_SHIFT)
#define SAMPLES (128 + 1) // one ref sample
#define RATE int(20000.0/SAMPLES)

static long timer = TIMER(RATE);

//  Queue used to send i&v readings

typedef struct 
{
  int v;
  int i;
  unsigned char sample;
  unsigned char cycle;
}  data_sample;

#define NUM_SAMPLES SAMPLES
Queue samples(NUM_SAMPLES, sizeof(data_sample));

  /*
  *  CT input AGC
  */

static int i_gain_shift = 0;
#define I_GAIN_MAX 5 // 1<<N == 32
// threshholds for current gain adjust
#define I_HIGH_READING 1000 // turn gain down
#define I_LOW_READING 768  // turn gain up

  /*
  *  Timer Interrupt
  */

ISR(TIMER1_COMPA_vect)
{
  static int v_ref = 512;
  static int i_ref = 512;
  static int i_gain = 0;
  static int i_max= 0;
  static unsigned char sample;
  static unsigned char cycle = 0;

  // TODO : v & i_ref reading

  set_timer(timer >> TIMER_SHIFT);
  sample += 1;

  // read the voltage sample
  const int v = v_adc.convert(V_MAINS);

  // read ADC started on previous cycle.
  const int i = i_adc.read();

  // zero crossing detect
  static int last_v = 0;
  const int zero_crossing = (v > v_ref) && (last_v <= v_ref);

  if (zero_crossing)
  {
    //pll(sample);
    sample = 0;
    cycle += 1;
    if (cycle >= 50)
      cycle = 0;
  }

  // store the gain for the push
  const int gain = i_gain;

  if (i > i_max)
    i_max = i;
    
  if (zero_crossing)
  {  
    if (i_max < I_LOW_READING)
    {
      if (i_gain < 5)
      {
        i_gain += 1;
        i_amp.set_gain(1 << i_gain);
      }
    }
    
    i_max = 0;
  }
  else if (i > I_HIGH_READING)
  {
    // gain too high
    if (i_gain > 0)
    {
      i_gain -= 1;
      i_amp.set_gain(1 << i_gain);
      i_max = 0;
    }
  }

  // kick off a slow conversion. Read it next interrupt.
  i_adc.start_conversion();

  // 10-bit ADC + 5-bit of gain, still < 16-bit
  const int current_x32 = (i - i_ref) << (5 - gain);
  // push sample data
  data_sample s = { last_v - v_ref, current_x32, sample, cycle };
  samples.put(& s);
  last_v = v;
}

 /*
  *  Setup
  */

void setup()
{
  Serial.begin(9600);

  // The MCP3002 needs ck < 250kHz, so slow the SPI clock down.
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  // PGA CS
  digitalWrite(PGA_CS_PIN, 1);
  pinMode(PGA_CS_PIN, OUTPUT);
  // Bluetooth power
  digitalWrite(BT_POWER, 1);
  pinMode(BT_POWER, OUTPUT);

  v_adc.init();

  i_adc.init();
  i_adc.set_chan(I_MAINS);  

  i_amp.init();
  i_amp.set_chan(0);
  i_amp.set_gain(1);
  set_timer(timer >> TIMER_SHIFT);
}

 /*
  *  Send power data for each cycle
  */

static void send_power_cycle(void* v)
{
  data_sample* s = (data_sample*) v;
  static int count = 0;
  static long power = 0;

  // 10-bit volts, 15-bit current, 128 samples (7-bit) == 32-bits signed
  power += s->v * long(s->i);
  count += 1;

  if (s->sample != 0)
    return;

  Serial.print(s->cycle);
  Serial.print(" ");
  // divide by number of samples, and remove *32 gain.
  Serial.print(float(power) / (count << 5));  
  Serial.print("\n");
  count = 0;
  power = 0;
}

 /*
  *  Main Loop
  */

void loop(void)
{
  data_sample s;
  if (samples.get(& s) == -1)
    return;

  static int show_wave = 0;

  if (Serial.available())
  {
    const int c = Serial.read();
    switch (c)
    {
      case 'W'  :  show_wave = 1;  break;
      case 'P'  :  show_wave = 0;  break;
      default   :  break;
    }
  }

  if (!show_wave)
  {
    send_power_cycle(& s);
    return;
  }

  static const int target_cycle = 0;    
  if (s.cycle != target_cycle)
    return;
    
  static int voltage[SAMPLES];
  static long current[SAMPLES];
  static int in = 0, out = 0;
  
  // wait for zero crossing 
  if ((in == 0) && (out == 0))
    if (s.sample != 0)
      return;

  if (in < SAMPLES)
  {
    voltage[in] = s.v;
    current[in] = s.i;
    in += 1;
    return;
  }

  Serial.print("*********** ");
  Serial.print(s.cycle);
  Serial.print("\n");

  while (out < SAMPLES)  
  {
    Serial.print(voltage[out]);
    Serial.print(" ");
    Serial.print(current[out]/32.0);
    Serial.print("\n");
    out += 1;
  }

  // flush the input
  while (samples.get(& s) == 0)
    ;
  // and start again
  in = out = 0;
}

// FIN
