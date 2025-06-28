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

#endif /* YNVISIBLE_7_SEGMENT_H_ */
