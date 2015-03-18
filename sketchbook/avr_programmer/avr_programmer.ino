

#include <avr/interrupt.h>
#include <avr/io.h>

// PORTD bits for charge pump
#define PUMP_SOURCE 4
#define PUMP_1      3
#define PUMP_2      2

#define PUMP_MASK   (_BV(PUMP_SOURCE) | _BV(PUMP_1) | _BV(PUMP_2))

static volatile uint16_t voltage = 400; // ~ 12V

#define  SCI     12    // Target Clock Input
#define  SDO     11    // Target Data Output
#define  SII     10    // Target Instruction Input
#define  SDI      9    // Target Data Input
#define  VCC      8    // Target VCC

#define  HFUSE  0x747C
#define  LFUSE  0x646C
#define  EFUSE  0x666E

// Define ATTiny series signatures
#define  ATTINY13   0x9007  // L: 0x6A, H: 0xFF             8 pin
#define  ATTINY24   0x910B  // L: 0x62, H: 0xDF, E: 0xFF   14 pin
#define  ATTINY25   0x9108  // L: 0x62, H: 0xDF, E: 0xFF    8 pin
#define  ATTINY44   0x9207  // L: 0x62, H: 0xDF, E: 0xFFF  14 pin
#define  ATTINY45   0x9206  // L: 0x62, H: 0xDF, E: 0xFF    8 pin
#define  ATTINY84   0x930C  // L: 0x62, H: 0xDF, E: 0xFFF  14 pin
#define  ATTINY85   0x930B  // L: 0x62, H: 0xDF, E: 0xFF    8 pin

// Define Direct I/O pins for Charge Pump
//#define P1  0x04  // Pin D2
//#define P2  0x08  // Pin D3
//#define PWR 0x10  // Pin D4
//#define GND 0x20  // Pin D5
//#define REF 404   // 12 volt reference

// Variables used by Charge pump
volatile char phase = 0;
volatile char onOff = 0;
volatile char pwrOn = 0;

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

  uint16_t read()
  {
    // wait for the conversion to complete
    while (ADCSRA & (1 << ADSC))
      ;
    // must read ADCL first, which freezes ADCH
    const uint8_t lo = ADCL;
    const uint16_t hi = ADCH;
    return lo + (hi << 8);
  }
};

static BuiltInAdc adc;

ISR(TIMER2_COMPA_vect) 
{
    const uint16_t value = adc.read();
    adc.start_conversion();

    if (onOff)
    {
        if (value < voltage)
        {
            static uint8_t phase = 0;
            if (phase)
            {
                PORTD &= ~PUMP_MASK;
                PORTD |= _BV(PUMP_SOURCE) | _BV(PUMP_1);
            }
            else
            {
                PORTD &= ~PUMP_MASK;
                PORTD |= _BV(PUMP_SOURCE) | _BV(PUMP_2);
            }
    
            DDRD |= PUMP_MASK;
    
            phase = !phase;
        }
        else
        {
            pwrOn = 1;
        }
        
    }
    else
    {
        DDRD &= ~PUMP_MASK;
        PORTD &= ~PUMP_MASK;
        pwrOn = 0;
    }
}

void do_program()
{
    Serial.print("setup\n");
    pinMode(SDO, OUTPUT);     // Set SDO to output
    digitalWrite(SDI, LOW);
    digitalWrite(SII, LOW);
    digitalWrite(SDO, LOW);
    onOff = 0;                // 12v Off
    digitalWrite(VCC, HIGH);  // Vcc On
    delayMicroseconds(20);
    onOff = 1;                // 12v On
    Serial.print("wait for power\n");
    while (pwrOn == 0)
      ;
    Serial.print("read sig.\n");
    delayMicroseconds(10);
    pinMode(SDO, INPUT);      // Set SDO to input
    delayMicroseconds(300);
    unsigned int sig = readSignature();
    Serial.print("Signature is: ");
    Serial.println(sig, HEX);
    readFuses();
    if (sig == ATTINY13) {
      writeFuse(LFUSE, 0x6A);
      writeFuse(HFUSE, 0xFF);
    } else if (sig == ATTINY24 || sig == ATTINY44 || sig == ATTINY84 ||
               sig == ATTINY25 || sig == ATTINY45 || sig == ATTINY85) {
      writeFuse(LFUSE, 0x62);
      writeFuse(HFUSE, 0xDF);
      writeFuse(EFUSE, 0xFF);
    }
    readFuses();
    digitalWrite(SCI, LOW);
    digitalWrite(VCC, LOW);    // Vcc Off
    onOff = 0;                 // 12v Off
}

byte shiftOut (byte val1, byte val2) {
  int inBits = 0;
  //Wait until SDO goes high
  while (!digitalRead(SDO))
    ;
  unsigned int dout = (unsigned int) val1 << 2;
  unsigned int iout = (unsigned int) val2 << 2;
  for (int ii = 10; ii >= 0; ii--)  {
    digitalWrite(SDI, !!(dout & (1 << ii)));
    digitalWrite(SII, !!(iout & (1 << ii)));
    inBits <<= 1;
    inBits |= digitalRead(SDO);
    digitalWrite(SCI, HIGH);
    digitalWrite(SCI, LOW);
  }
  return inBits >> 2;
}

void writeFuse (unsigned int fuse, byte val) {
  shiftOut(0x40, 0x4C);
  shiftOut( val, 0x2C);
  shiftOut(0x00, (byte) (fuse >> 8));
  shiftOut(0x00, (byte) fuse);
}

void readFuses () {
  byte val;
        shiftOut(0x04, 0x4C);  // LFuse
        shiftOut(0x00, 0x68);
  val = shiftOut(0x00, 0x6C);
  Serial.print("LFuse: ");
  Serial.print(val, HEX);
        shiftOut(0x04, 0x4C);  // HFuse
        shiftOut(0x00, 0x7A);
  val = shiftOut(0x00, 0x7E);
  Serial.print(", HFuse: ");
  Serial.print(val, HEX);
        shiftOut(0x04, 0x4C);  // EFuse
        shiftOut(0x00, 0x6A);
  val = shiftOut(0x00, 0x6E);
  Serial.print(", EFuse: ");
  Serial.println(val, HEX);
}

unsigned int readSignature () {
  unsigned int sig = 0;
  byte val;
  for (int ii = 1; ii < 3; ii++) {
          shiftOut(0x08, 0x4C);
          shiftOut(  ii, 0x0C);
          shiftOut(0x00, 0x68);
    val = shiftOut(0x00, 0x6C);
    sig = (sig << 8) + val;
  }
  return sig;
}

void init_timer()
{
    // Set TIMER 2
    TCCR2A = 0;
    // ck/ 1=1, 2=8, 3=32, 4=64, 
    TCCR2B = 0x03; // ck/8 (see p 162)
    // period
    OCR2A = 0x80;
    //OCR2A = 0x20;
    // compare A interrupt
    TIMSK2 = 0x02;
}

void setup() {
  pinMode(VCC, OUTPUT);
  pinMode(SDI, OUTPUT);
  pinMode(SII, OUTPUT);
  pinMode(SCI, OUTPUT);
  pinMode(SDO, OUTPUT);     // Configured as input when in programming mode
  Serial.begin(57600);
  // Setup timer interrupt for  charge pump
  analogReference(DEFAULT);
  
  init_timer();
  adc.init();
  adc.set_chan(0);
//    pump_on = 1;

  Serial.print("starting ...\n");
}

void loop() {
#if 0
   if (Serial.available() > 0)
   {
       Serial.read();
       do_program();
   }
#endif
   
   static int state = 0;
   
   onOff = state;
   state = !state;
   delay(100);   
}   
   
//  FIN
