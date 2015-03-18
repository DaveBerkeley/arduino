

    /*
     *  TODO : Tidier interface to the RF12 library
     */

class Radio
{
public:
    Radio();

    typedef enum {
        MHz_868,
    }   Band;

    bool init(uint8_t node, uint8_t group, Band band, uint8_t wait);

    void poll();

    virtual void on_rx(uint8_t node, uint8_t* msg, uint8_t size) { }
    virtual void on_tx(uint8_t node) {};

    void send(uint8_t node, uint8_t* msg, uint8_t size);
};

// FIN
