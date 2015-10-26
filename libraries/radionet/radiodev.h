
#include <radionet.h>

class RadioDev {
private:
  typedef enum {
    START=0,
    SLEEP,  // for sleepy nodes
    LISTEN, // for awake nodes
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
  uint16_t nsleep;
  uint16_t sleep_count;
  void (*debug_fn)(const char* to_print);

  static const int ACK_WAIT_MS = 100;
  static const int ACK_RETRIES = 5;

  void sleep(uint16_t time);
  void make_message(Message* msg, int msg_id, bool ack);
  void on_message_handler(Message* msg);

public:

  typedef enum {
    OK,
    TEST,
  }  LED;
  
protected:
  RadioDev(uint8_t gateway_id, uint16_t sleeps=1);

  virtual void set_led(LED idx, bool state){};

  // Build a data Message 
  virtual void append_message(Message* msg) = 0;

  // Rx a message
  virtual void on_message(Message* msg) { }

  // main loop
  void radio_loop(uint16_t time);

  // poll loop
  void radio_poll();

public:
  virtual const char* banner() = 0;
  virtual void init(void);  
  virtual void loop(void) = 0;

  void power_on();
  void req_tx_message();

  void set_debug(void (*fn)(const char* to_print));
  void debug(const char* text);
};

//  FIN
