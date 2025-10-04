/*
 * segment_driver_tcal9539.h
 *
 *  Created on: Aug 20, 2025
 *      Author: nick
 */

#ifndef YNVISIBLE_7_SEGMENT_H_
#define YNVISIBLE_7_SEGMENT_H_

#include <stdint.h>

void positive_v_driver(uint8_t portSel,
                       uint8_t mask,
                       uint16_t t_set_ms);

