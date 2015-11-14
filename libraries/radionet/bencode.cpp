
#include <stdint.h>

#include "Arduino.h"

#include "bencode.h"

    /*
    *   Bencode Parser : for host->radio communication
    */

Bencode::Bencode()
: state(WAIT)
{
}

void Bencode::reset(Packet* msg)
{
    msg->reset();
    count = 0;
    next_data = 0;
    state = WAIT;
}

int Bencode::parse(Packet* msg, unsigned char c)
{
    switch (state)
    {
      case WAIT : {
        if (c == 'l') {
          state = NODE;
          return 0;
        }
 
        reset(msg);      
        return 0;
      }
      case NODE : {
        if (c == 'i') {
          msg->node = 0;
          return 0;
        }
        if ((c >= '0') && (c <= '9')) {
          msg->node *= 10;
          msg->node += c - '0';
          return 0;
        }
        if (c == 'e') {
          msg->length = 0;
          state = LENGTH;
          return 0;
        }
        
        reset(msg);
        return 0;
      }
      
      case LENGTH : {
        if (c == ':') {
          if (msg->length >= MAX_PACKET_DATA) {
            reset(msg);
            return 0;
          }
          state = DATA;
          count = 0;
          next_data = msg->data;
          return 0;
        }
        if ((c >= '0') && (c <= '9')) {
          msg->length *= 10;
          msg->length += c - '0';
          return 0;
        }
        
        reset(msg);
        return 0;
      }
      
      case DATA : {
        if (count < msg->length) {
          *next_data++ = c;
          count++;
          return 0;
        }
        
        // done : message is complete
        return 1;
      }
    }
    return 0;
}

void Bencode::to_host(int node, const uint8_t* data, int bytes)
{
    // send the packet in Bencode to the host        
    Serial.print("li");
    Serial.print(node);
    Serial.print("e");
    Serial.print(bytes);
    Serial.print(":");
    for (int i = 0; i < bytes; ++i)
        Serial.print((char) data[i]);
    Serial.print("e");
}

//  FIN
