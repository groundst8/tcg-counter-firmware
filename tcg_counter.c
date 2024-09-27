#include <rf430frl152h.h>
#include <stdint.h>
#include "rom.h"

uint8_t segment_to_bits(uint8_t segment);

/* We need to specify the Firmware Control Byte, which decides such
   things as the block size and whether the ROM is handling the sensor
   acquisition, or just the networking stack. */
__attribute__((section(".firmwarecontrolbyte")))
const uint8_t firmwarecontrolbyte = 0x7F; //8-byte pages, no ROM sensor support.

/* 
   This vendor function allows for a raw read of words from an
   address.  It is roughly like the A3 command found in the
   RF430TAL152H devices.

   02 - High speed mode selection (start of actuall RF packet)
   a3 - The actual custom command
   07 - TI Manufacturer ID (need by this IC)
   01 - Set f segment to on  (0x00 to be off)

   To try it, 'make hello.txt' and then flash it using GoodV.  Then
   use NFC Tools to send "02:a3:07:01" by NfcV, which ought to
   result in "00:12:50".  The value at FFFE is 5012.
*/

uint16_t __attribute__ ((noinline)) cmd_a3(){
  uint8_t control;

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
  // Port 1 Bits: abcdegfz (z is common electrode)
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

  // CRC_LENGTH + 1 byte expected
  if( RF13MFIFOFL_L == 2 + 1) {

    // pull one byte from the recieve FIFO
    control=RF13MRXF_L;

    // set output direction
    P1DIR |= BIT0 & BIT1;

    if (control) {
      // f segment 1.5V
      // z common electrode 0V
      P1OUT |= BIT1;
      P1OUT &= ~BIT0;
    } else {
      // f segment 0V
      // z common electrode 1.5V
      P1OUT |= BIT0;
      P1OUT &= ~BIT1;
    }

    //Zero byte for success.
    RF13MTXF_L = 0;
    
    // delay then put in input mode for high z state
    // TODO: figure out how to do delay
    //P1DIR &= ~BIT0 & ~BIT1;
    
  } else {
    RF13MRXF=1;
  }

  return 0;
}

uint16_t __attribute__ ((noinline)) cmd_segment_on(){
  uint8_t segment;
  // CRC_LENGTH + 1 byte expected
  if( RF13MFIFOFL_L == 2 + 1) {

    // pull one byte from the recieve FIFO
    segment=RF13MRXF_L;

    uint8_t bits = segment_to_bits(segment);

    // set output direction
    P1DIR |= bits | BIT0;
    // set specified segment 1.5V, set segment z common electrode = 0V
    P1OUT = (P1OUT & ~BIT0) | bits;
    
    //Zero byte for success.
    RF13MTXF_L = 0;
    
  } else {
    // Error
    RF13MRXF=1;
  }

  return 0;
}

uint16_t __attribute__ ((noinline)) cmd_segment_off(){
  uint8_t segment;

  // CRC_LENGTH + 1 byte expected
  if( RF13MFIFOFL_L == 2 + 1) {

    // pull one byte from the recieve FIFO
    segment=RF13MRXF_L;

    uint8_t bits = segment_to_bits(segment);
    
    // set output direction
    P1DIR |= bits | BIT0;
    // set specified segment 0V, set segment z common electrode = 1.5V
    P1OUT = (P1OUT & ~bits) | BIT0;

    //Zero byte for success.
    RF13MTXF_L = 0;
    
  } else {
    // Error
    RF13MRXF=1;
  }

  return 0;
}

uint16_t __attribute__ ((noinline)) cmd_segment_high_z(){
  uint8_t segment;

  // CRC_LENGTH + 1 byte expected
  if( RF13MFIFOFL_L == 2 + 1) {

    // pull one byte from the recieve FIFO
    segment=RF13MRXF_L;

    uint8_t bits = segment_to_bits(segment);
    
    // put in input mode for high z state
    P1DIR &= ~bits & ~BIT0;

    //Zero byte for success.
    RF13MTXF_L = 0;
    
  } else {
    // Error
    RF13MRXF=1;
  }

  return 0;
}

// expects segments 1-7
uint8_t segment_to_bits(uint8_t segment) {
  return 1 << (segment-1);
}



/* And we need a patch table, and it has very strict rules about
   placement and alignment.

   1) The patch table is read backward from the start marker CECE at 0xFFCE.
   2) It must end with CECE after all valid records. (Lower address.)
   3) Entries  < 0x0100 are command hooks.
   4) Entries >= 0x0100 are patches to the ROM hook table.
*/

__attribute__((section(".rompatch")))
const uint16_t patchtable[0x12] =
  {
   0xCECE, 
   0xCECE, 0xCECE,
   0xCECE, 0xCECE,
   0xCECE, 0xCECE,
   0xCECE, 0xCECE,
   0xCECE, 0xCECE,
   (uint16_t) cmd_segment_high_z, 0x00A3, //Handler address and command number.
   (uint16_t) cmd_segment_on, 0x00A0, //Handler address and command number.
   (uint16_t) cmd_segment_off, 0x00A1, //Handler address and command number.
   0xCECE,  //This ABSOLUTELY MUST be at 0xFFCE or your patch won't load.
  };


