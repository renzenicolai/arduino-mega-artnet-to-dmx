#include "Arduino.h"
#include "dmx.h"
#include <avr/interrupt.h>

#define BREAKSPEED     100000
#define DMXSPEED       250000

#define Calcprescale(B)     ( ( (((F_CPU)/8)/(B)) - 1 ) / 2 )

RnDMXClass RnDMX;

/* Code for USART0 */
/*
#define SERIAL0_8N2  ((1<<USBS0) | (0<<UPM00) | (3<<UCSZ00))
#define SERIAL0_8E1  ((0<<USBS0) | (2<<UPM00) | (3<<UCSZ00))
#define BREAKFORMAT0    SERIAL0_8E1
#define DMXFORMAT0      SERIAL0_8N2

int     _dmxChannel0;
uint8_t  _dmxData0[RNDMX_MAX+1];
uint8_t *_dmxDataPtr0;

uint8_t *RnDMXClass::getBuffer0()
{
  return(_dmxData0);
}

void _RnDMXBaud0(uint16_t baud_setting, uint8_t format)
{
  UCSR0A = 0;
  UBRR0H = baud_setting >> 8;
  UBRR0L = baud_setting;
  UCSR0C = format;
}

void RnDMXClass::init0()
{
  _dmxChannel0 = 0;
  _dmxDataPtr0 = _dmxData0;
  for (int n = 0; n < RNDMX_MAX+1; n++) _dmxData0[n] = 0;
  UCSR0B = (1<<TXEN0) | (1<<TXCIE0);
  _RnDMXBaud0(Calcprescale(BREAKSPEED), BREAKFORMAT0);
  UDR0 = (uint8_t) 0;
}

void RnDMXClass::write0(uint16_t channel, uint8_t value)
{
  if (channel < 1) channel = 1;
  if (channel > RNDMX_MAX) channel = RNDMX_MAX;
  _dmxData0[channel] = value;
}

ISR(USART0_TX_vect)
{
  if (_dmxChannel0 == -1) {
    _RnDMXBaud0(Calcprescale(BREAKSPEED), BREAKFORMAT0);
    UDR0 = (uint8_t) 0;
    _dmxChannel0 = 0;

  } else if (_dmxChannel0 == 0) {
    _RnDMXBaud0(Calcprescale(DMXSPEED), DMXFORMAT0);
    UCSR0B = (1<<TXEN0) | (1<<UDRIE0);
    UDR0 = (uint8_t) 0;
    _dmxChannel0 = 1;
  }
}

ISR(USART0_UDRE_vect)
{
  UDR0 = _dmxData0[_dmxChannel0++];
  if (_dmxChannel0 > RNDMX_MAX) {
    _dmxChannel0   = -1;
    UCSR0B = (1<<TXEN0) | (1<<TXCIE0);
  }
}
*/
/* Code for USART1 */

#define SERIAL1_8N2  ((1<<USBS1) | (0<<UPM10) | (3<<UCSZ10))
#define SERIAL1_8E1  ((0<<USBS1) | (2<<UPM10) | (3<<UCSZ10))
#define BREAKFORMAT1    SERIAL1_8E1
#define DMXFORMAT1      SERIAL1_8N2

int     _dmxChannel1;
uint8_t  _dmxData1[RNDMX_MAX+1];
uint8_t *_dmxDataPtr1;

uint8_t *RnDMXClass::getBuffer1()
{
  return(_dmxData1);
}

void _RnDMXBaud1(uint16_t baud_setting, uint8_t format)
{
  UCSR1A = 0;
  UBRR1H = baud_setting >> 8;
  UBRR1L = baud_setting;
  UCSR1C = format;
}

void RnDMXClass::init1()
{
  _dmxChannel1 = 0;
  _dmxDataPtr1 = _dmxData1;
  for (int n = 0; n < RNDMX_MAX+1; n++) _dmxData1[n] = 0;
  UCSR1B = (1<<TXEN1) | (1<<TXCIE1);
  _RnDMXBaud1(Calcprescale(BREAKSPEED), BREAKFORMAT1);
  UDR1 = (uint8_t) 0;
}

void RnDMXClass::write1(uint16_t channel, uint8_t value)
{
  if (channel < 1) channel = 1;
  if (channel > RNDMX_MAX) channel = RNDMX_MAX;
  _dmxData1[channel] = value;
}

ISR(USART1_TX_vect)
{
  if (_dmxChannel1 == -1) {
    _RnDMXBaud1(Calcprescale(BREAKSPEED), BREAKFORMAT1);
    UDR1 = (uint8_t) 0;
    _dmxChannel1 = 0;

  } else if (_dmxChannel1 == 0) {
    _RnDMXBaud1(Calcprescale(DMXSPEED), DMXFORMAT1);
    UCSR1B = (1<<TXEN1) | (1<<UDRIE1);
    UDR1 = (uint8_t) 0;
    _dmxChannel1 = 1;
  }
}

ISR(USART1_UDRE_vect)
{
  UDR1 = _dmxData1[_dmxChannel1++];
  if (_dmxChannel1 > RNDMX_MAX) {
    _dmxChannel1   = -1;
    UCSR1B = (1<<TXEN1) | (1<<TXCIE1);
  }
}

/* Code for USART2 */

#define SERIAL2_8N2  ((1<<USBS2) | (0<<UPM20) | (3<<UCSZ20))
#define SERIAL2_8E1  ((0<<USBS2) | (2<<UPM20) | (3<<UCSZ20))
#define BREAKFORMAT2    SERIAL2_8E1
#define DMXFORMAT2      SERIAL2_8N2

int     _dmxChannel2;
uint8_t  _dmxData2[RNDMX_MAX+1];
uint8_t *_dmxDataPtr2;

uint8_t *RnDMXClass::getBuffer2()
{
  return(_dmxData2);
}

void _RnDMXBaud2(uint16_t baud_setting, uint8_t format)
{
  UCSR2A = 0;
  UBRR2H = baud_setting >> 8;
  UBRR2L = baud_setting;
  UCSR2C = format;
}

void RnDMXClass::init2()
{
  _dmxChannel2 = 0;
  _dmxDataPtr2 = _dmxData2;
  for (int n = 0; n < RNDMX_MAX+1; n++) _dmxData2[n] = 0;
  UCSR2B = (1<<TXEN2) | (1<<TXCIE2);
  _RnDMXBaud2(Calcprescale(BREAKSPEED), BREAKFORMAT2);
  UDR2 = (uint8_t) 0;
}

void RnDMXClass::write2(uint16_t channel, uint8_t value)
{
  if (channel < 1) channel = 1;
  if (channel > RNDMX_MAX) channel = RNDMX_MAX;
  _dmxData2[channel] = value;
}

ISR(USART2_TX_vect)
{
  if (_dmxChannel2 == -1) {
    _RnDMXBaud2(Calcprescale(BREAKSPEED), BREAKFORMAT2);
    UDR2 = (uint8_t) 0;
    _dmxChannel2 = 0;

  } else if (_dmxChannel2 == 0) {
    _RnDMXBaud2(Calcprescale(DMXSPEED), DMXFORMAT2);
    UCSR2B = (1<<TXEN2) | (1<<UDRIE2);
    UDR2 = (uint8_t) 0;
    _dmxChannel2 = 1;
  }
}

ISR(USART2_UDRE_vect)
{
  UDR2 = _dmxData2[_dmxChannel2++];
  if (_dmxChannel2 > RNDMX_MAX) {
    _dmxChannel2   = -1;
    UCSR2B = (1<<TXEN2) | (1<<TXCIE2);
  }
}
