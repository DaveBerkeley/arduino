
class LED {
private:
  void (*m_fn)(bool on);
  uint16_t m_count;
public:
  LED(void (*fn)(bool on))
  : m_fn(fn),
    m_count(0)
  {
  }

  void set(uint16_t count)
  {
    (*m_fn)(count > 0);
    m_count = count;
  }

  void poll(void)
  {
    if (m_count > 0)
    {
      if (!--m_count)
      {
        (*m_fn)(0);
      }
    }
  }

  // passed a -1 terminated array of LED pins
  static void init(int8_t* pins)
  {
      for (int i = 0; ; i++) {
        const int8_t p = pins[i];
        if (p == -1)
          break;
        pinMode(p, OUTPUT);
        digitalWrite(p, HIGH);
      }
  }
};

//  FIN
