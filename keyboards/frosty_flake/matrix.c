/*
  Copyright 2014 Ralf Schmitt <ralf@bunkertor.net>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include "print.h"
#include "debug.h"
#include "util.h"
#include "matrix.h"

#ifndef CONFIG_SPECIFIC_H
#define CONFIG_SPECIFIC_H

#define CONFIG_LED_IO \
  DDRB |= (1<<7); \
  DDRC |= (1<<5) | (1<<6);

#define USB_LED_CAPS_LOCK_ON    PORTC &= ~(1<<5)
#define USB_LED_CAPS_LOCK_OFF   PORTC |=  (1<<5)
#define USB_LED_NUM_LOCK_ON     PORTB &= ~(1<<7)
#define USB_LED_NUM_LOCK_OFF    PORTB |=  (1<<7)
#define USB_LED_SCROLL_LOCK_ON  PORTC &= ~(1<<6)
#define USB_LED_SCROLL_LOCK_OFF PORTC |=  (1<<6)

#define CONFIG_MATRIX_IO   \
  /* Column output pins */ \
  DDRD  |=  0b01111011;    \
  /* Row input pins */     \
  DDRC  &= ~0b10000000;    \
  DDRB  &= ~0b01111111;    \
  PORTC |=  0b10000000;    \
  PORTB |=  0b01111111;

#define MATRIX_ROW_SCAN                      \
  (PINC&(1<<7) ? 0 : ((matrix_row_t)1<<0)) | \
  (PINB&(1<<5) ? 0 : ((matrix_row_t)1<<1)) | \
  (PINB&(1<<4) ? 0 : ((matrix_row_t)1<<2)) | \
  (PINB&(1<<6) ? 0 : ((matrix_row_t)1<<3)) | \
  (PINB&(1<<1) ? 0 : ((matrix_row_t)1<<4)) | \
  (PINB&(1<<2) ? 0 : ((matrix_row_t)1<<5)) | \
  (PINB&(1<<3) ? 0 : ((matrix_row_t)1<<6)) | \
  (PINB&(1<<0) ? 0 : ((matrix_row_t)1<<7))

#define MATRIX_ROW_SELECT                                     \
  case  0: PORTD = (PORTD & ~0b01111011) | 0b00011011; break; \
  case  1: PORTD = (PORTD & ~0b01111011) | 0b01000011; break; \
  case  2: PORTD = (PORTD & ~0b01111011) | 0b01101010; break; \
  case  3: PORTD = (PORTD & ~0b01111011) | 0b01111001; break; \
  case  4: PORTD = (PORTD & ~0b01111011) | 0b01100010; break; \
  case  5: PORTD = (PORTD & ~0b01111011) | 0b01110001; break; \
  case  6: PORTD = (PORTD & ~0b01111011) | 0b01100001; break; \
  case  7: PORTD = (PORTD & ~0b01111011) | 0b01110000; break; \
  case  8: PORTD = (PORTD & ~0b01111011) | 0b01100000; break; \
  case  9: PORTD = (PORTD & ~0b01111011) | 0b01101000; break; \
  case 10: PORTD = (PORTD & ~0b01111011) | 0b00101011; break; \
  case 11: PORTD = (PORTD & ~0b01111011) | 0b00110011; break; \
  case 12: PORTD = (PORTD & ~0b01111011) | 0b00100011; break; \
  case 13: PORTD = (PORTD & ~0b01111011) | 0b01111000; break; \
  case 14: PORTD = (PORTD & ~0b01111011) | 0b00010011; break; \
  case 15: PORTD = (PORTD & ~0b01111011) | 0b01101001; break; \
  case 16: PORTD = (PORTD & ~0b01111011) | 0b00001011; break; \
  case 17: PORTD = (PORTD & ~0b01111011) | 0b00111011; break;

#endif

#ifndef DEBOUNCING_DELAY
#   define DEBOUNCING_DELAY 0
#endif
static uint8_t debouncing = DEBOUNCING_DELAY;

static matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_debouncing[MATRIX_ROWS];

static matrix_row_t scan_row(void);
static void select_row(uint8_t row);

inline uint8_t matrix_rows(void) {
  return MATRIX_ROWS;
}

inline uint8_t matrix_cols(void) {
  return MATRIX_COLS;
}

void matrix_init(void) {
  CONFIG_MATRIX_IO;

  for (uint8_t i=0; i < MATRIX_ROWS; i++)  {
    matrix[i] = 0;
    matrix_debouncing[i] = 0;
  }

  matrix_init_quantum();
}

uint8_t matrix_scan(void) {
  for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
    select_row(row);
    _delay_us(3);
    matrix_row_t row_scan = scan_row();
    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
      bool prev_bit = matrix_debouncing[row] & ((matrix_row_t)1<<col);
      bool curr_bit = row_scan & (1<<col);
      if (prev_bit != curr_bit) {
        matrix_debouncing[row] ^= ((matrix_row_t)1<<col);
        debouncing = DEBOUNCING_DELAY;
      }
    }
  }

  if (debouncing) {
    if (--debouncing)
      _delay_ms(1);
    else
      for (uint8_t i = 0; i < MATRIX_ROWS; i++)
        matrix[i] = matrix_debouncing[i];
  }

  matrix_scan_quantum();
  return 1;
}

bool matrix_is_modified(void) {
  if (debouncing)
    return false;
  else
    return true;
}

inline bool matrix_is_on(uint8_t row, uint8_t col) {
  return (matrix[row] & ((matrix_row_t)1<<col));
}

inline matrix_row_t matrix_get_row(uint8_t row) {
  return matrix[row];
}

void matrix_print(void) {
  print("\nr/c 01234567\n");
  for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
    matrix_row_t row_scan = matrix_get_row(row);
    xprintf("%02X: ", row);
    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
      bool curr_bit = row_scan & (1<<col);
      xprintf("%c", curr_bit ? '*' : '.');
    }
    print("\n");
  }
}

uint8_t matrix_key_count(void) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
    count += bitpop32(matrix[i]);
  }
  return count;
}

static matrix_row_t scan_row(void) {
  return MATRIX_ROW_SCAN;
}

static void select_row(uint8_t row) {
  switch (row) {
    MATRIX_ROW_SELECT;
  }
}