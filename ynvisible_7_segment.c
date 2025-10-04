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

uint8_t lowest_one(uint8_t x)
{
    return x & (uint8_t)(-x);          // two’s-complement trick
}


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


/**
 * bucket_fill_sequential_segment_driver()
 *
 *  • Always drives ≤ segsPerStep segments at once → flat inrush current.
 *  • Greedy “lowest bits first” packing → minimum pulse count.
 *  • Returns the pulse count so callers/tests can verify work done.
 */
uint8_t bucket_fill_sequential_segment_driver(uint8_t        portSel,
                                              uint8_t        segsPerStep,
                                              uint8_t        segments,
                                              SegmentDriverFn driver,
                                              uint8_t        pulse_time_ms)
{
    uint8_t pending   = segments & 0x7F;   /* mask off DP or stray bits     */
    uint8_t pulses    = 0;

    while (pending)
    {
        uint8_t groupMask = 0;
        uint8_t filled    = 0;

        /* Pull up to segsPerStep lowest bits into this bucket  */
        while (pending && filled < segsPerStep)
        {
            uint8_t bit  = lowest_one(pending);
            groupMask   |= bit;
            pending     &= (uint8_t)~bit;   /* remove from todo-set */
            ++filled;
        }

        driver(portSel, groupMask, pulse_time_ms);
        ++pulses;
    }
    return pulses;
}
