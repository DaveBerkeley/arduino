
  /*
  *
  */

#define MAX_DATA 86 // TODO : what is the min this can be?

class Packet
{
public:
  int node;
  int length;
  unsigned char data[MAX_DATA];

  void reset()
  {
      node = 0;
      length = -1;
  }
};


    /*
    *   Bencode Parser : for host->radio communication
    */

class Parser
{
    enum PARSE_STATE
    {
        WAIT,
        NODE,
        LENGTH,
        DATA,
    };

public:
    PARSE_STATE state;
    unsigned char* next_data;
    int count;

    Parser();

    void reset(Packet* msg);
    int parse(Packet* msg, unsigned char c);

    static void to_host(int node, const uint8_t* data, int bytes);
};

//  FIN
