#define F_CPU 16000000 /* Set clock frequency for delay timer */
#define BLINK_DELAY_MS 1000

#include <avr/io.h>
#include <util/delay.h>

int main(void) {
  /* set pin 5 of PORTB for output */
  DDRB |= _BV(DDB5);

  while(1) {
    /* set pin 5 high to turn led on */
    PORTB |= _BV(PORTB5);
    _delay_ms(BLINK_DELAY_MS);

    /* set pin 5 low to turn led off */
    PORTB &= ~_BV(PORTB5);
    _delay_ms(BLINK_DELAY_MS);
  }
}