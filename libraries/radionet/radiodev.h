
#include <radionet.h>

class RadioDev {
private:
  typedef enum {
    START=0,
    SLEEP,
    SENDING,
    WAIT_FOR_ACK,
  } STATE;

  STATE state;
  byte my_node;
  uint16_t ack_id;
  uint8_t retries;
  Message message;
  uint32_t wait_until = 0;
  uint8_t gateway_id;

  static const int ACK_WAIT_MS = 100;
  static const int ACK_RETRIES = 5;

public:
  RadioDev(uint8_t gateway_id);

  virtual const char* banner() = 0;
  
  virtual void init(void);
  
  typedef enum {
    OK,
    TEST,
  }  LED;
  
  virtual void set_led(LED idx, bool state) = 0;

  static const uint16_t SLEEP_TIME = 32000;

  void sleep(uint16_t time);

   /*
    * Build a data Message 
    */
  
  virtual void append_message(Message* msg) = 0;

  void make_message(Message* msg, int msg_id, bool ack);

  virtual void loop(void) = 0;

  void radio_loop(uint16_t time);
};

//  FIN
