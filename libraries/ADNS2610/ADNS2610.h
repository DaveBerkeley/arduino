
#define SDIO 5
#define SCLK 4

#define FRAMELENGTH 324

void mouseInit();
void dumpDiag();
void readFrame(byte *arr);
void writeRegister(byte addr, byte data);
byte readRegister(byte addr);

