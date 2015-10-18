

// node address
#define GATEWAY_ID 31

class Message {
  typedef struct {
    uint8_t m_id;       // non-zero message id
    uint8_t m_dest;     // destination node address
    uint16_t m_flags;   // admin flags and present data
  } header;

  typedef struct {
    header m_hdr;
    uint8_t m_data[50];
  }  message;

private:
  message m_msg;
  uint8_t* m_next;
  uint8_t m_size;

public:

  enum {
    ACK             = 0x8000,   // set if ACK is required
    ADMIN           = 0x4000,   // set if 'hello' requested
    // Data Payloads
    TEXT            = 0x2000,   // payload is 'ident' string
    //NOT_SUPPORTED   = 0x1000,   // unsupported command error
    FLASH           = 0x0800,   // payload is FLASH message
    USER_MASK       = 0x7FF,    // Mask for device specific messages
  };

  Message(int msg_id, int dest)
  {
    m_msg.m_hdr.m_id = msg_id;
    m_msg.m_hdr.m_dest = dest;
    m_msg.m_hdr.m_flags = 0; 
    reset();
  }

  Message(const void* msg)
  {
    if (msg)
      memcpy(& m_msg, msg, sizeof(m_msg));
    else
      memset(& m_msg, 0, sizeof(m_msg));
    reset();
  }

  void reset()
  {
    m_next = m_msg.m_data;
    m_size = 0;
  }

  uint8_t get_dest() const
  {
    return m_msg.m_hdr.m_dest;
  }

  void set_dest(uint8_t dest)
  {
    m_msg.m_hdr.m_dest = dest;
  }

  uint16_t get_mid() const
  {
    return m_msg.m_hdr.m_id;
  }

  void set_mid(uint8_t mid)
  {
    m_msg.m_hdr.m_id = mid;
  }

  bool get_ack() const
  {
    return m_msg.m_hdr.m_flags & ACK;
  }

  void set_ack()
  {
    m_msg.m_hdr.m_flags = ACK;
  }

  bool get_admin() const
  {
    return m_msg.m_hdr.m_flags & ADMIN;
  }

  void set_admin()
  {
    m_msg.m_hdr.m_flags = ADMIN;
  }

  uint16_t get_flags() const
  {
    return m_msg.m_hdr.m_flags;
  }
 
  void append(int mask, const void* data, int size)
  {
    memcpy(m_next, data, size);
    m_next += size;
    m_size += size;
    m_msg.m_hdr.m_flags |= mask;
  }

  bool extract(int mask, void* data, int size) 
  {
    if (!(m_msg.m_hdr.m_flags & mask))
      return false;
    memcpy(data, m_next, size);
    m_next += size;
    return true;
  }

  int size() const
  {
    return sizeof(header) + m_size;
  }

  void* data()
  {
    return & m_msg;
  }

  void* payload()
  {
    return m_next;
  }
};

// Generate a unique message id
uint8_t make_mid();

// Show message contents via Serial interface
void show_message(Message* msg, const char* text, uint8_t my_node);

// Send Message over RF12
void send_message(Message* msg);

// Send text over RF12
void send_text(const char* text, int msg_id, bool ack);

// FIN
