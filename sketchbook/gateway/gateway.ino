
#include <JeeLib.h>

#include <radionet.h>
#include <led.h>

    /*
    *
    */

class Buffer
{
    char* m_buff;
    uint8_t m_size;
    uint8_t m_in;
    uint8_t m_out;
public:
    Buffer(char* buff, uint8_t size)
    : m_buff(buff), m_size(size), m_in(0), m_out(0)
    {
    }

    void reset()
    {
        m_in = m_out = 0;
    }

    bool put(char c)
    {
        uint8_t next = m_in + 1;
        if (next >= m_size)
            next = 0;
        if (next == m_out)
            return false;

        m_buff[m_in] = c;
        m_in = next;

        return true;
    }

    int get()
    {
        if (m_in == m_out)
            return -1;
        uint8_t next = m_out + 1;
        if (next >= m_size)
            next = 0;
        const char c = m_buff[m_out];
        m_out = next;
        return c;
    }

    uint8_t size()
    {
        return m_in - m_out;
    }
};

    /*
    *
    */

#define TX_LED 4
#define RX_LED 5

static Port leds(2);

static void tx_led(bool on) 
{
  leds.digiWrite(on);
}

static void rx_led(bool on) 
{
  leds.digiWrite2(!on);
}

static LED rx(rx_led);
static LED tx(tx_led);

    /*
    *
    */

static char hex(uint8_t n)
{
  if (n < 10)
    return '0' + n;
  return n - 10 + 'A';
}

static void print_hex(uint8_t n)
{
  Serial.print(hex(n >> 4));
  Serial.print(hex(n & 0x0F));
}

static void to_host(int node, uint8_t* data, int bytes)
{
  // send the packet to the host        
  print_hex(node);
  Serial.print(" :");

  for (int i = 0; i < bytes; ++i)
  {
    Serial.print(" ");
    print_hex(data[i] & 0xFF);
  }

  Serial.print("\r\n");
}

static void packet_to_host(void)
{
  to_host((int) rf12_hdr, (uint8_t*) rf12_data, (int) rf12_len);
}

    /*
    *
    */

#define MAX_PAYLOAD 40

class Parser
{
    char buff_data[MAX_PAYLOAD];
    Buffer buffer;
    uint8_t node;
    int number;

    typedef enum {
        NODE,
        BYTE,
        SENDING,
    }   State;

    State state;

public:
    Parser() 
    : buffer(buff_data, sizeof(buff_data))
    {
        reset();
    }

    void reset()
    {
        buffer.reset();
        state = NODE;
        number = -1;
        node = 0;
    }

    static bool nibble(uint8_t* n, char c)
    {
        if ((c >= '0') && (c <= '9'))
        {
            *n = c - '0';
            return true;
        }

        if ((c >= 'A') && (c <= 'F'))
        {
            *n = c - 'A' + 10;
            return true;
        }

        if ((c >= 'a') && (c <= 'f'))
        {
            *n = c - 'a' + 10;
            return true;
        }

        return false;
    }

    bool on_number(char c)
    {
        uint8_t n;
        if (!nibble(& n, c))
            return false;

        if (number == -1)
        {
            number = n;
        }
        else
        {
            number *= 16;
            number += n;
        }
        return true;
    }

    void send()
    {
        // TODO : send the message
        const uint8_t size = buffer.size();

        Serial.print("tx: ");
        print_hex(node);
        Serial.print(" :");
        char* p = buff_data;
        for (uint8_t i = 0; i < size; i++)
        {
            Serial.print(" ");
            print_hex(*p++);
        }
        Serial.print("\r\n");

        set_nodeid(node);
        rf12_sendStart(buff_data[0], & buff_data[1], size-1);
    }

    bool sending()
    {
        return state == SENDING;
    }

    void parse(char c)
    {
        if (state == SENDING)
        {
            // BUSY error
            return;
        }

        if (on_number(c))
            return;

        if (state == NODE)
        {
            if (number == -1)
                return;
            node = number;
            number = -1;
            state = BYTE;
            return;
        }

        if (number == -1)
            return;

        if (!buffer.put(number))
        {
            //  Buffer FULL error
            reset();
            return;
        }
        number = -1;

        if (c == '\r')
        {
            if ((node == 0) || !buffer.size())
            {
                reset();
            }
            else
            {
                state = SENDING;
            }
        }
    }
};

    /*
    *
    */

static uint8_t my_node;

void setup()
{
  tx.set(0);
  rx.set(0);
  leds.mode(OUTPUT);  
  leds.mode2(OUTPUT);  

  Serial.begin(57600);
  rf12_initialize(my_node = GATEWAY_ID, RF12_868MHZ, 6);
}

#define FLASH_PERIOD 8000

static Parser parser;

void loop()
{
  rx.poll();
  tx.poll();

  // send any rx data to the host  
  if (rf12_recvDone() && rf12_crc == 0) {
    rx.set(FLASH_PERIOD);
    packet_to_host();
    // TODO : ack the messages?
  }

  while (Serial.available())
  {
    if (parser.sending())
        break;

    tx.set(FLASH_PERIOD);
    parser.parse(Serial.read());
  }

  if (parser.sending())
  {
    if (rf12_canSend())
    {
      parser.send();
      parser.reset();
      rf12_sendWait(0);
    }
  }
}

//  FIN
