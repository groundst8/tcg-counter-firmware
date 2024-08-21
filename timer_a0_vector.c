#include <rf430frl152h.h>
#include <stdint.h>
#include "rom.h"

/* We need to specify the Firmware Control Byte, which decides such
   things as the block size and whether the ROM is handling the sensor
   acquisition, or just the networking stack. */
__attribute__((section(".firmwarecontrolbyte")))
const uint8_t firmwarecontrolbyte = 0x7F; //8-byte pages, no ROM sensor support.

/* 
   Read TimerA0 interrupt vector value

   02 - High speed mode selection (start of actuall RF packet)
   a3 - The actual custom command
   07 - TI Manufacturer ID (need by this IC)

   No parameter, just return timer A0 control register value
*/

uint16_t __attribute__ ((noinline)) cmd_a3(){
  uint16_t *adr;

  //Zero byte for success.
  RF13MTXF_L = 0;

  //return timer A0 interrupt vector
  adr=(uint16_t*) 0xFFF8;
  RF13MTXF = *adr;

  return 0;
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
   0xCECE, 0xCECE,
   0xCECE, 0xCECE,
   
   (uint16_t) cmd_a3, 0x00A3, //Handler address and command number.
   0xCECE,  //This ABSOLUTELY MUST be at 0xFFCE or your patch won't load.
  };
