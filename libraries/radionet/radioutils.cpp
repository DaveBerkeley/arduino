
#include <avr/wdt.h>

#include <JeeLib.h>

#include "radioutils.h"

  /*
  *  Read Vcc using the 1.1V reference.
  *
  *  from :
  *
  *  https://code.google.com/p/tinkerit/wiki/SecretVoltmeter
  */

long read_vcc() {
  long result;
  // Read 1.1V reference against AVcc
  const uint16_t mux = ADMUX;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC))
    ;
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  ADMUX = mux; // restore mux setting
  return result;
}

    /*
     *  Force a reboot that looks like an external reset
     */

void reboot()
{
    wdt_enable(WDTO_15MS);
    noInterrupts();
    // Set the "external reset" flag
    // so the bootloader runs.
    while (true) 
        MCUSR |= _BV(EXTRF);
}

//  FIN
