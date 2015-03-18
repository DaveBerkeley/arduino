
#include <SPI.h>
#include <Adb.h>

// Adb connection.
Connection * connection;

//int android_rd[2];

// Event handler for the shell connection. 
void adbEventHandler(Connection * connection, adb_eventType event, uint16_t length, uint8_t * data)
{
  // not used yet
  
  // Data packets contain two bytes, one for each servo, in the range of [0..180]
  if (event == ADB_CONNECTION_RECEIVE)
  {
    //android_rd[0] = data[0];
    //android_rd[1] = data[1];
  }
}

void init_adb(void)
{  
  // Initialise the ADB subsystem.  
  ADB::init();
  // Open an ADB stream to the phone's shell. Auto-reconnect
  connection = ADB::addConnection("tcp:4567", true, adbEventHandler);  
}

#define HEADER 0x6a

void send_adb(long data)
{
  uint8_t buff[6];
  
  buff[0] = HEADER;
  int crc = HEADER;

  for (int i = 0; i < 4; ++i)
  {
    const uint8_t c = data & 0xFF;
    buff[i+1] = c;
    data >>= 8;
    crc += c;
  }
  
  buff[5] = crc & 0xFF;
  
  connection->write(sizeof(buff), buff);
}

  /*
  *
  */

typedef enum STATE
{
  SYNC_HI = 1,
  SYNC_LO,
  BIT_HI,
  BIT_LO,
};

struct sonar_t
{
    STATE seq;      //  state machine
    unsigned long t_last;  // last change time in us
    int index;      //  bit being shifted in
    long data;      //  accumulated bits being shifted in
    long result;    //  32-bit signal from sonar
    int bit_hi;     //  last bit hi period in us
    int bit_lo;     //  last bit lo period in us
    int data_valid; //  set when all bits in
    // pins
    int pin;        //  signal pin
    int led_debug;  //  LED pin if debug required
};

  /*
  *  sonar irq handling
  */

extern void on_sonar_irq(struct sonar_t* sonar);

static struct sonar_t*  objs[2];

static void on_irq_0(void)
{
  on_sonar_irq(objs[0]);
}

static void on_irq_1(void)
{
  on_sonar_irq(objs[1]);
}

  /*
  *  initialise sonar
  */

int init_sonar(struct sonar_t* sonar, int pin, int led_debug)
{
  // init the pins
  pinMode(pin, INPUT);
  if (led_debug)
    pinMode(led_debug, OUTPUT);

  // init the data
  memset(sonar, 0, sizeof(sonar_t));
  sonar->seq = SYNC_HI;
  sonar->pin = pin;
  sonar->led_debug = led_debug;

  switch (pin)
  {
    case 2 :  
      objs[0] = sonar;
      attachInterrupt(0, on_irq_0, CHANGE);
      break;
    case 3 :  
      objs[1] = sonar;
      attachInterrupt(1, on_irq_1, CHANGE);
      break;
    default:  
      return -1; // error
  }

  return 0;
}

  /*
  *  sonar irq handler
  */

void on_sonar_irq(sonar_t* sonar)
{
  const unsigned long now = micros();
  const int hi = digitalRead(sonar->pin);

  // time since last edge
  const unsigned long t_diff = now - sonar->t_last;
  sonar->t_last = now;

  if (sonar->led_debug)
    digitalWrite(sonar->led_debug, hi);
  
  switch (sonar->seq)
  {
    case SYNC_HI :
    {
      // waiting for hi/lo transition at 2.8ms
      if (hi)
        return;
      if ((t_diff < 2700) || (t_diff > 2900))
        return;

      sonar->seq = SYNC_LO;
      break;
    }
    
    case SYNC_LO :
    {
      // waiting for lo/hi transition at 1ms
      sonar->seq = SYNC_HI; // assume a fail
      if (!hi)
        return;        
      if ((t_diff > 1100) || (t_diff < 900)) 
        return;

      sonar->seq = BIT_HI;
      sonar->index = 0;
      break;
    }
    
    case BIT_HI :
    {
      // measure the lo/hi/lo period
      sonar->seq = SYNC_HI; // assume a fail
      if (hi)
        return;
   
      sonar->bit_lo = t_diff;
      sonar->seq = BIT_LO;

      // index==0 is the start of the data stream
      // so we don't have any data yet.
      if (sonar->index == 0)
        break;

      // calculate the bit length from the last two periods.
      // verfiy it fits in the expected hi/lo bit period.
      const int bit_period = sonar->bit_hi + sonar->bit_lo;      
      if ((bit_period > 370) || (bit_period < 340))
        return;
      
      // add the data bit in
      sonar->data <<= 1;
      if (sonar->bit_hi < sonar->bit_lo)
      {
        sonar->data |= 1;
      }

      break;
    }

    case BIT_LO   : 
      // measure the hi/lo/hi period.
      sonar->seq = SYNC_HI; // assume a fail
      if (!hi)
        return;
   
      sonar->bit_hi = t_diff;      
      sonar->index += 1;
      
      // do we have all the bits?
      if (sonar->index > 32)
      {
        // all the bits have been shifted in.
        // mark data valid and wait for the next sequence.
        sonar->seq = SYNC_HI;
        sonar->result = sonar->data;
        sonar->data_valid = 1;
      }
      else
      {
        // get the next bit.
        sonar->seq = BIT_HI;
      }
      break;
  }  
}

  /*
  *
  */

int poll_sonar(struct sonar_t* sonar, unsigned long* data)
{
  if (!sonar->data_valid)
    return 0;
  *data = sonar->result;
  sonar->data_valid = 0;
  return 1;
}

  /*
  *
  */

// set pin numbers:
#define LED_PIN 13
#define SIG_PIN 2

static struct sonar_t sonar;

void setup() 
{  
  init_sonar(& sonar, SIG_PIN, LED_PIN);
  init_adb();

  Serial.begin(9600);
}

void loop()
{
  ADB::poll();

  unsigned long data;

  if (poll_sonar(& sonar, & data))
  {
    Serial.print(data, HEX);
    Serial.print("\n");
  
    send_adb(data);  
  }
}

// FIN
