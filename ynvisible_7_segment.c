/*
 * ynvisible_7_segment.c
 *
 *  Created on: Jun 28, 2025
 *      Author: nick
 */

#include "ynvisible_7_segment.h"

// 7-segment encoding lookup table
// Each element represents the segments needed to display a digit
//     a
//    ___
// f | g | b
//    ---
// e |   | c
//    ---
// z   d
// |
// Port Bits: gfedcba (z is common electrode)
const uint8_t seven_segment_table[10] = {
  0b0111111,  // 0
  0b0000110,  // 1
  0b1011011,  // 2
  0b1001111,  // 3
  0b1100110,  // 4
  0b1101101,  // 5
  0b1111101,  // 6
  0b0000111,  // 7
  0b1111111,  // 8
  0b1101111   // 9
};

// oF to represent overflow
const uint8_t seven_segment_oF[2] = {
  0b1011100,  // o
  0b1110001,  // F
};


/**
 *  Generate bit masks to classify segments by update type required to transition from
 *  previous to next digit
 *
 *  prevDigit  – bitmap of the currently lit segments for digit  (bits 0-6 = g-f-e-d-c-b-a)
 *  nextDigit  – bitmap of the segments for the next digit
 *
 *  switchOnMask  – segments that must receive a +VON pulse (OFF → ON)
 *  switchOffMask – segments that must receive a –VOFF pulse (ON  → OFF)
 *  refreshMask   – segments that stay ON and therefore need a refresh pulse
 *
 *  Any segment that is 0 in all three masks needs no action.
 */
void classify_segments(uint8_t prevDigit,
                       uint8_t nextDigit,
                       uint8_t *switchOnMask,
                       uint8_t *switchOffMask,
                       uint8_t *refreshMask)
{

    // OFF → OFF
    // No operation

    // OFF → ON
    *switchOnMask  = ~prevDigit & nextDigit;

    // ON → OFF
    *switchOffMask = prevDigit & ~nextDigit;

    // ON → ON  (refresh)
    *refreshMask   = prevDigit & nextDigit;
}
