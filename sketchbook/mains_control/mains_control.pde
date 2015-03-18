
// set pin numbers:
const int ledPin = 13;
const int sigPin = 12;
const int pwrPin = 11;

static const int fields = 12;

// CLOCK set empirically to match captured data
//#define CLOCK  81
//#define SHORT  (CLOCK*4)
//#define LONG   (CLOCK*(16-4))
//#define SYNC   (CLOCK*(128-4))

#define SHORT  353
#define LONG   973
#define SYNC   10238

static void pulse(int mark, int space)
{
  digitalWrite(sigPin, HIGH);
  delayMicroseconds(mark);
  digitalWrite(sigPin, LOW);
  delayMicroseconds(space);
}

static void send_sync(void)
{
  pulse(SHORT, SYNC);
}

enum {
  B_ZERO = 0,
  B_ONE = 1,
  B_FLOAT = 2,
}  BIT;

static void tx_bit(int bit)
{
  static const int zero[] = {  SHORT, LONG, SHORT, LONG };
  static const int one[] =  {  LONG, SHORT, LONG, SHORT };
  static const int flt[] =  {  SHORT, LONG, LONG, SHORT };

  const int* seq = 0;
  
  switch (bit)
  {
    case B_ZERO  : seq = zero; break;
    case B_ONE   : seq = one;  break;
    case B_FLOAT : seq = flt;  break;
    default      : seq = flt;  break; // shouldn't happen!
  }
  
  pulse(seq[0], seq[1]);
  pulse(seq[2], seq[3]);
}

static void tx_word(long data, int repeats)
{
  noInterrupts();

  for (int repeat = 0; repeat < repeats; ++repeat)
  {
    for (int i = 0; i < fields; ++i)
    {
      const int shift = (fields - i - 1) * 2;
      const int d = (data >> shift) & 0x03;
      tx_bit(d);
    }  
    send_sync();
  }
  
  interrupts();
}

  /*
  *
  */
  
static void ack(void)
{
  Serial.write(0xA5);
}

static void nak(void)
{
  Serial.write(0x11);
}

  /*
  *
  */
  
static long get_serial(int* repeats)
{
  if (!Serial.available())
    return -1;

  static unsigned char buff[5];
  static int index = 0;
  const int data = Serial.read();

  buff[index++] = (unsigned char) data;

  if (index < sizeof(buff))
    return -1;

  int okay = true;

  if ((buff[0] & 0xF0) != 0xA0)
    okay = false;
    
  const unsigned char cs = buff[0] + buff[1] + buff[2] + buff[3];
  if (buff[4] != cs)
    okay = false;

  if (!okay)
  {
    // discard oldest byte and wait for next one
    index = sizeof(buff) - 1; // last entry
    for (int i = 0; i < index; ++i)
      buff[i] = buff[i+1];
      
    return -2;
  }

  index = 0;
  *repeats = buff[0] & 0x0F;
  
  long result = 0;
  for (int i = 1; i < (sizeof(buff)-1); ++i)
  {
    result <<= 8;
    result += buff[i];
  }
  return result;
}

  /*
  *
  */

void setup() {
  pinMode(ledPin, OUTPUT);      
  pinMode(sigPin, OUTPUT);      

  // enable the DC-DC converter
  pinMode(pwrPin, OUTPUT);      
  digitalWrite(sigPin, LOW);

  Serial.begin(9600);
}

void loop()
{
  int repeats;
  const long data = get_serial(& repeats);

  if (data == -1)
    return;
  if (data == -2)
  {
    return;
    nak();
  }

  digitalWrite(ledPin, HIGH);
  tx_word(data, repeats);
  digitalWrite(ledPin, LOW);

  ack();
}

// FIN
