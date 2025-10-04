/*
 * ynvisible_7_segment.h
 *
 *  Created on: Jun 28, 2025
 *      Author: nick
 */

#ifndef YNVISIBLE_7_SEGMENT_H_
#define YNVISIBLE_7_SEGMENT_H_

#include <stdint.h>

extern const uint8_t seven_segment_table[10];
extern const uint8_t seven_segment_oF[2];

void classify_segments(uint8_t prevDigit,
                       uint8_t nextDigit,
                       uint8_t *switchOnMask,
                       uint8_t *switchOffMask,
                       uint8_t *refreshMask);

/* Prototype of a low-level driver that energises one “bucket” of segments */
typedef void (*SegmentDriverFn)(uint8_t port_sel,
                                uint8_t segment_mask,
                                uint8_t pulse_ms);


/**
 * bucket_fill_sequential_segment_driver
 *
 * Collects up to @p segsPerStep bits from @p segments, issues a pulse
 * through @p driver, and repeats until every bit has been serviced.
 *
 * @param portSel       Identifies the COM or GPIO port (passed verbatim to driver)
 * @param segsPerStep   Maximum number of segments driven in one pulse (1 … 7)
 * @param segments      Bit-mask (gfedcba) of segments to act on
 * @param driver        Callback that toggles the pins and delays for @p pulse_time_ms
 * @param pulse_time_ms Pulse width in milliseconds
 *
 * @return              Number of pulses issued (ceil(popcount(segments)/segsPerStep))
 *
 * Bits outside 0x7F are silently ignored so the same mask can be OR’d with
 * a future decimal-point bit without breaking this helper.
 */
uint8_t bucket_fill_sequential_segment_driver(uint8_t        portSel,
                                              uint8_t        segsPerStep,
                                              uint8_t        segments,
                                              SegmentDriverFn driver,
                                              uint8_t        pulse_time_ms);

#endif /* YNVISIBLE_7_SEGMENT_H_ */
