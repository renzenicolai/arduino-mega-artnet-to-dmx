#ifndef dmx_h
#define dmx_h

#include <avr/io.h>

#define RNDMX_MAX 512 // max. number of supported DMX data channels

class RnDMXClass
{
  public:
    //void init0();
    //void write0(uint16_t channel, uint8_t value);   
    //uint8_t *getBuffer0(); 
    void init1();
    void write1(uint16_t channel, uint8_t value); 
    uint8_t *getBuffer1();
    void init2();
    void write2(uint16_t channel, uint8_t value);    
    uint8_t *getBuffer2();
  private:
    //Nothing.
};

extern RnDMXClass RnDMX;

#endif
