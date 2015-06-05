
#define FRAMELENGTH (18*18)

class MouseCam 
{
public:
    uint8_t m_sdio, m_sck;
public:
    MouseCam(uint8_t sdio, uint8_t sck);
    bool init();
    uint8_t getId();
    void writeRegister(byte addr, byte data);
    byte readRegister(byte addr);
    void readFrame(byte *arr, void (*idle)(void*), void* arg);
};

