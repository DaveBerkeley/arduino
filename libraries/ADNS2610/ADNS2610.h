
    /*
     *  Class to control ADNS2610 Optical Mouse chip.
     */

class MouseCam 
{
private:
    uint8_t m_sdio, m_sck;

    void writeRegister(byte addr, byte data);
    byte readRegister(byte addr);

public:
    const static int FRAMELENGTH = 18 * 18;

    MouseCam(uint8_t sdio, uint8_t sck);

    bool init();
    uint8_t getId();
    bool readFrame(byte *frame, void (*idle)(void*)=0, void* arg=0);
};

//  FIN
