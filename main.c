#include <rf430frl152h.h>
#include <stdint.h>
#include <string.h>  // For memset

#define CLEAR_BLOCK_LOCKS                            	BIT3
#define FRAM_LOCK_BLOCK_AREA_SIZE  						38
#define FRAM_LOCK_BLOCKS								0xF840  //Address of ISO15693 lock blocks

// macro instead of linker config for now
#define DS (*(volatile uint8_t *)0x1C00)


/* Firmware System Control Byte
 *
 *     Bit 0: 	ISOBlockSize				0 - 4 byte,		1 - 8 byte
 *     Bit 1:	Page						0 - page 1, 	1 - page 0 (Effective only for 4-byte block mode)
 *     Bit 2: 	ROMEUSCISupportEnabled		0 - disabled, 	1 - enabled (Forced to 0 on RF430FRL153H)
 *     Bit 3-5: ReservedISO
 *     Bit 6: 	NFCBridgeDisable  			0 - enabled, 	1 - disabled (see note below)
 *     Bit 7:   ROMSensorSupportEnable		0 - disabled, 	1 - enabled (Forced to 0 on RF430FRL154H)
 *
 *     NFC bridge is recommended to be disabled in this project.  Unexpected behaviour can occur,
 *     trying to use it, due to the configuration being setup here.
 *
 *     If eUSCI host controller portion is needed along with the RF functionality, the default project
 *     must be used.  That is NFC cannot be supported in that application (because the I2C/SPI host controller
 *     control registers are in the same place that the NFC file needs to be).  However the rest of the FRAM
 *     memory can be used for storing and reading using ISO15693.
 */

//This project is based on the RF430FRL152H.  However it will work as well on the RF430FRL154H.
//However ROM_SENSOR_SUPPORT_DISABLED (or ROMSensorSupportEnable see above for both )must be set in the firmware system control register.  This is forced automatically on the RF430FRL154H.
//This setting is needed to disable the ROM which uses block 0... as virtual registers, however this memory is needed for NDEF purposes.
__attribute__((section(".firmwarecontrolbyte")))
const uint8_t firmwarecontrolbyte = 0x7F;

void initISO15693(uint16_t parameters)
{
    RF13MCTL |= RF13MTXEN + RF13MRXEN + RF13MRFTOEN;  // Set up RX and TX functionality on RF13M module
    // Enable interrupts, ** Do not change the following two lines, needed for proper RF stack operation
    RF13MINT |= RF13MRXIE + RX13MRFTOIE;  // Enable interrupts on RX, timeout, and over/underflow checking

    if (parameters & CLEAR_BLOCK_LOCKS)
    {
        memset((uint8_t *)FRAM_LOCK_BLOCKS, 0xFF, FRAM_LOCK_BLOCK_AREA_SIZE);  // Block is locked with a zero bit, clears FRAM and RAM lock blocks
    }
}

void DeviceInit(void)
{
//	P1SEL0 = 0xF0; //keep JTAG
//	P1SEL1 = 0xF0; //keep JTAG
	P1SEL0 = 0x00; //no JTAG
	P1SEL1 = 0x00; //no JTAG

    // TODO: figure out why not all set to inputs
    P1DIR &= ~0xEF;
    P1REN = 0;

    CCSCTL0 = CCSKEY;                        // Unlock CCS

    CCSCTL1 = 0;                             // do not half the clock speed
    CCSCTL4 = SELA_1 + SELM_0 + SELS_0;      // Select VLO for ACLK and select HFCLK/DCO for MCLK, and SMCLK
    CCSCTL5 = DIVA_2 + DIVM_1 + DIVS_1;      // Set the Dividers for ACLK (4), MCLK, and SMCLK to 1
    CCSCTL6 = XTOFF;                         // Turns of the crystal if it is not being used
    CCSCTL8 = ACLKREQEN + MCLKREQEN + SMCLKREQEN; //disable clocks if they are not being used

    CCSCTL0_H |= 0xFF;                       // Lock CCS

    return;
}


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer

    // ROM RF13M module setup
    DS = 1;  // ROM variable needs to be initialized here

    // Call ROM functions using inline assembly compatible with msp430-gcc
    __asm__("CALL #0x5CDA");  // Initialize function pointers in ROM
    __asm__("CALL #0x5CAC");  // Check part configuration in ROM

    // Initialize ISO15693 with clearing block locks
    initISO15693(CLEAR_BLOCK_LOCKS);
    // Alternatively, leave block locks as they are set in FRAM
    // initISO15693(0);

    // JTAG is set to be disabled in this function call
    DeviceInit();

    // set output direction
    P1DIR |= BIT0 | BIT1;

    while (1)
    {
        // generate test square waves on P1.0 and P1.1
        P1OUT |= BIT1;
        P1OUT &= ~BIT0;
        __delay_cycles(200000);
        P1OUT |= BIT0;
        P1OUT &= ~BIT1;
        __delay_cycles(200000);

        // Enter application code here...
        // Enter low-power mode 3 (LPM3) with interrupts enabled
        //__bis_SR_register(LPM3_bits | GIE);
    }

    // Return statement to prevent compiler warnings
    return 0;
}