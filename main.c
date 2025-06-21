#include <rf430frl152h.h>
#include <rf13m_rom_config.h>
#include <rf13m_rom_patch.h>
#include <tcal9539.h>
#include <stdint.h>
#include "event_queue.h"

// Error codes for NFC responses
#define ERROR_UPDATE_IN_PROGRESS 1
#define ERROR_UNEXPECTED_NFC_LENGTH 2

// Display update configuration
#ifndef SEGMENTS_PER_STEP_DEFAULT
#define SEGMENTS_PER_STEP_DEFAULT 2
#endif

// Global variables
volatile bool update_in_progress = false;
volatile uint8_t segments_per_step = SEGMENTS_PER_STEP_DEFAULT;


EventQueue event_queue;

void device_init(void);
void process_event(const Event *event);
void start_timer(uint16_t duration_ms);
void stop_watchdog_timer();
void wait_for_event();
void delay(uint16_t duration_ms);
void wakeup();

void userCustomCommand();

void update_display(uint8_t number);
void write_ones_digit(uint8_t segments);
void write_tens_digit(uint8_t segments);
void clear_tens_digit();

void clear_ones_digit_sequential(uint8_t segsPerStep);
void clear_tens_digit_sequential(uint8_t segsPerStep);
void set_tens_digit_sequential(uint8_t segsPerStep, uint8_t segments);
void set_ones_digit_sequential(uint8_t segsPerStep, uint8_t segments);
void clear_digit_sequential(uint8_t portSel, uint8_t segsPerStep);
void set_digit_sequential(uint8_t portSel,
                          uint8_t segsPerStep,
                          uint8_t segments);
void clear_group(uint8_t portSel,
                 uint8_t mask,
                 uint16_t t_clear_ms);
void set_group(uint8_t portSel,
               uint8_t mask,
               uint16_t t_set_ms);

/*
void drive_group(uint8_t portSel,
                 uint8_t mask,
                 uint16_t t_pre_ms,
                 uint16_t t_set_ms);
void write_digit_sequential(uint8_t segments, bool tensDigit);
*/

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


int main(void)
{
	
    stop_watchdog_timer();
	rf13m_setup();
    init_iso15693(CLEAR_BLOCK_LOCKS);  // clear all block locks
    device_init();
	event_queue_init(&event_queue);
	init_i2c();

	// Configure P1.3 to output ACLK for debugging purposes to figure out what's going on with clocks
	//P1DIR |= BIT3;         // Set P1.3 as output
	//P1SEL0 |= BIT3;        // Set P1.3 function select bits
	//P1SEL1 |= BIT3;        // Select quaternary function (ACLK output)

	while (1) {

	    wait_for_event();

        Event event;
        while(event_queue_get(&event_queue, &event)) {
            process_event(&event);
        }

    }
}

/**
 * Enter low power mode until interrupt occurs
 */
void wait_for_event() {
    __bis_SR_register(LPM0_bits | GIE);  // Enter LPM3
}

void stop_watchdog_timer() {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
}

void device_init(void)
{
    P1SEL0 = 0xF0; //keep JTAG
    P1SEL1 = 0xF0; //keep JTAG
//    P1SEL0 = 0x00; //no JTAG
//    P1SEL1 = 0x00; //no JTAG

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


// Start timer for the specified duration in milliseconds
void delay(uint16_t duration_ms) {
    // seems to be 64kHz, not sure why

    // Configure Timer_A
    TA0CCTL0 = CCIE;                           // Enable interrupt
    TA0CCR0 = duration_ms * 64;                // ACLK is sourced from VLO at 64 kHz?
    TA0CTL = TASSEL__ACLK | MC__UP | TACLR;    // ACLK, Up mode, clear TAR

    __bis_SR_register(LPM0_bits | GIE);

}

void wakeup() {
    // Configure Timer_A
    TA0CCTL0 = CCIE;                           // Enable interrupt
    TA0CCR0 = 64;                // ACLK is sourced from VLO at 64 kHz?
    TA0CTL = TASSEL__ACLK | MC__UP | TACLR;    // ACLK, Up mode, clear TAR
}

// Timer_A interrupt service routine
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A_ISR(void) {
    // Stop Timer_A
    TA0CTL = MC__STOP;
    TA0CCTL0 &= ~CCIE;
    TA0CCTL0 &= ~CCIFG;  // Clear interrupt flag

    // Create a timer event
    //Event event;
    //event.type = EVENT_TIMER;
    //event_queue_put(&event_queue, &event);

    // Exit LPM3
    __bic_SR_register_on_exit(LPM0_bits);
}


//#pragma CODE_SECTION(RF13M_ISR, ".fram_driver_code")  // comment this line for using ROM's RF13M ISR, uncomment next one, see .cmd file for details
#pragma CODE_SECTION(RF13M_ISR, ".rf13m_rom_isr")       // comment this line for creating a custom RF13M ISR that will exist in FRAM, bypassing ROM's, uncomment previous
#pragma vector = RF13M_VECTOR
__interrupt void RF13M_ISR(void)
{
    // Right now this vector is pointing to the ROMs firmware location that runs the RF stack.
    // Entering code here will, without changing the CODE_SECTION lines
    // above, will cause an error.
    // Changing the code section above will cause the ROM RF stack to be bypassed.  New handler will need to be created.
}


// Process events from the event queue
void process_event(const Event *event) {

    switch (event->type) {
        case EVENT_NFC:
            // Delayed response works with NFC Tools testing on Android
            // Would make it easier to indicate when display update is complete, don't have to poll
            RF13MTXF_L = 0x03;
            delay(100); // browning out sometimes, see if this helps
            update_display(event->data);
            break;
        case EVENT_TIMER:
        default:
            // Unknown event type
            break;
    }

}

void update_display(uint8_t number) {

    // could potentially represent 100 as 00 but for now will cap at 99
    if (number > 99) {
       // write segments as oF to indicate overflow
       //write_tens_digit(seven_segment_oF[0]);
       //write_ones_digit(seven_segment_oF[1]);
       clear_tens_digit_sequential(2);
       set_tens_digit_sequential(2, seven_segment_oF[0]);
       clear_ones_digit_sequential(2);
       set_ones_digit_sequential(2, seven_segment_oF[1]);
    } else {
       uint8_t tens = number / 10;      // Extract tens digit
       uint8_t ones = number % 10;      // Extract ones digit

       uint8_t tens_code = seven_segment_table[tens];  // Lookup segment code for tens digit
       uint8_t ones_code = seven_segment_table[ones];  // Lookup segment code for ones digit

       // don't display padded 0
       if (tens == 0)
       {
           // clear out any previous value in this case
           // TODO: only do this if needed, will be switching away from this way in future
           //clear_tens_digit();
           clear_tens_digit_sequential(2);
       } else {
           //write_tens_digit(tens_code);
           clear_tens_digit_sequential(2);
           set_tens_digit_sequential(2, tens_code);
       }
       //write_ones_digit(ones_code);
       clear_ones_digit_sequential(2);
       set_ones_digit_sequential(2, ones_code);
    }
}

// write segments sequentially, no refresh just full switch everything for now
void write_ones_digit(uint8_t segments) {

    // common electrode is on PORT0
    //set com to output, everything else to input
    write_tcal9539_register(CONFIG_PORT0_REG, 0x7F);

    //clear
    //set to outputs
    write_tcal9539_register(CONFIG_PORT1_REG, 0x00);
    //common electrode high, everything else low for -1.5V across segments
    write_tcal9539_register(OUTPUT_PORT0_REG, 0x80);
    write_tcal9539_register(OUTPUT_PORT1_REG, 0x00);
    delay(200);

    // after updates put into input mode (high impedance)
    write_tcal9539_register(CONFIG_PORT1_REG, 0xFF);
    write_tcal9539_register(CONFIG_PORT0_REG, 0xFF);
    // give time for caps to charge
    delay(100);
    // common electrode is on PORT0
    //set com to output, everything else to input
    write_tcal9539_register(CONFIG_PORT0_REG, 0x7F);


    // write all
    //common electrode low, segments high for 1.5V across segments
    write_tcal9539_register(OUTPUT_PORT0_REG, 0x00);
    //set segments as outputs
    write_tcal9539_register(CONFIG_PORT1_REG, ~segments);
    //common electrode low, segments high for 1.5V across segments
    write_tcal9539_register(OUTPUT_PORT1_REG, segments);
    delay(700); // was 700
    delay(200); // timer limit?

    // after updates put into input mode (high impedance)
    write_tcal9539_register(CONFIG_PORT1_REG, 0xFF);
    write_tcal9539_register(CONFIG_PORT0_REG, 0xFF);
    delay(100); // allow time for caps to charge before proceeding
}

void write_tens_digit(uint8_t segments) {
    //clear
    //set to outputs
    write_tcal9539_register(CONFIG_PORT0_REG, 0x00);
    //common electrode high, everything else low for -1.5V across segments
    write_tcal9539_register(OUTPUT_PORT0_REG, 0x80);
    delay(200);

    // write all
    //set common electrode and segments as outputs
    write_tcal9539_register(CONFIG_PORT0_REG, ~(BIT7 | segments));
    //common electrode low, segments high for 1.5V across segments
    write_tcal9539_register(OUTPUT_PORT0_REG, segments);
    delay(700);
    delay(200); // timer limit?
    // after updates put into input mode (high impedance)
    write_tcal9539_register(CONFIG_PORT0_REG, 0xFF);
    delay(100); // allow time for caps to charge before proceeding
}


void clear_tens_digit() {
    //clear
    //set to outputs
    write_tcal9539_register(CONFIG_PORT0_REG, 0x00);
    //common electrode high, everything else low for -1.5V across segments
    write_tcal9539_register(OUTPUT_PORT0_REG, 0x80);
    delay(200);

    // after updates put into input mode (high impedance)
    write_tcal9539_register(CONFIG_PORT0_REG, 0xFF);
    delay(100); // allow time for caps to charge before proceeding
}

/**
 * Clear every segment of the tens digit, but only <segsPerStep>
 * electrodes at a time to keep the in-rush low.
 *
 * segsPerStep = 1…7  (2 is a sensible value for NFC-powered updates)
 */
void clear_tens_digit_sequential(uint8_t segsPerStep)
{
    // tens digit PORT0 so portSel = 0
    clear_digit_sequential(0, segsPerStep);

}

void clear_ones_digit_sequential(uint8_t segsPerStep)
{
    // ones digit PORT1 so portSel = 1
    clear_digit_sequential(1, segsPerStep);

}


void clear_digit_sequential(uint8_t portSel, uint8_t segsPerStep) {

    uint8_t i = 0;
    uint8_t b = 0;

    for (i = 0; i < 7; i += segsPerStep)
    {
        uint8_t mask = 0;
        for (b = 0; b < segsPerStep && (i + b) < 7; ++b)
            mask |= (1u << (i + b));

        // TODO: adjust t_clear_ms proportional to number of segments being updated
        clear_group(portSel, mask, 75);
    }
}

void set_tens_digit_sequential(uint8_t segsPerStep, uint8_t segments)
{
    // tens digit PORT0 so portSel = 0
    set_digit_sequential(0, segsPerStep, segments);

}

void set_ones_digit_sequential(uint8_t segsPerStep, uint8_t segments)
{
    // tens digit PORT1 so portSel = 1
    set_digit_sequential(1, segsPerStep, segments);

}


/******************************************************************************
 *  Turn-on helper: sequentially SET the requested segments
 *
 *  @param portSel      0 → segments on PORT0 (tens digit)
 *                      1 → segments on PORT1 (ones digit)
 *  @param segsPerStep  1…7  - how many segments you want to switch at once
 *  @param segments     bit pattern you ultimately want ON (gfedcba)
 *
 *  Call order:
 *      clear_digit_sequential(portSel, segsPerStep);   // phase-1: clear all
 *      set_digit_sequential  (portSel, segsPerStep, pattern);  // phase-2: set
 ******************************************************************************/
void set_digit_sequential(uint8_t portSel,
                          uint8_t segsPerStep,
                          uint8_t segments)
{
    uint8_t i, b;

    for (i = 0; i < 7; i += segsPerStep)
    {
        /* build the bit-mask for this group (up to segsPerStep bits) */
        uint8_t mask = 0;
        for (b = 0; b < segsPerStep && (i + b) < 7; ++b)
            mask |= (1u << (i + b));

        /* keep only the bits we actually need to light in this group */
        uint8_t groupMask = mask & segments;
        if (groupMask == 0)
            continue;                       // nothing to do for this group

        /* TODO: scale t_set_ms with group size if desired               *
         * For now we use a fixed 150 ms pulse; tweak for your display.  */
        set_group(portSel, groupMask, 250);
    }
}


/**
 * Drive the selected segments low while COM stays high, then Hi-Z them.
 *
 *  portSel     – 0 for P0.x (tens digit), 1 for P1.x (ones digit)
 *  mask        – bit-mask of the one-or-two segments we’re clearing
 *  t_clear_ms  – how long to keep the field applied
 */
void clear_group(uint8_t portSel,
                 uint8_t mask,
                 uint16_t t_clear_ms)
{
    const uint8_t COM_MASK   = BIT7;                          // COM = P0.7
    const uint8_t cfgReg     = (portSel == 0) ? CONFIG_PORT0_REG
                                              : CONFIG_PORT1_REG;
    const uint8_t outReg     = (portSel == 0) ? OUTPUT_PORT0_REG
                                              : OUTPUT_PORT1_REG;

    // COM - common electrode is on PORT0
    if (portSel == 0) {
        /* --- drive: COM↑, segments↓ -------------------------------- */
        write_tcal9539_register(cfgReg, ~(COM_MASK | mask));      // COM + seg = outputs
        write_tcal9539_register(outReg, COM_MASK | ~mask);         // COM = high, segs = low (Vd = −1.5 V)
    } else {
       write_tcal9539_register(CONFIG_PORT0_REG, ~COM_MASK);
       write_tcal9539_register(cfgReg, ~mask);
       //common electrode high, everything else low for -1.5V across segments
       //always port 0
       write_tcal9539_register(OUTPUT_PORT0_REG, COM_MASK);
       write_tcal9539_register(outReg, ~mask);
    }

    delay(t_clear_ms);

    /* --- Hi-Z PORT1 first */
    if (portSel == 1) {
        write_tcal9539_register(cfgReg, 0xFF);
    }

    // will always need to make Hi-Z since COM always used
    write_tcal9539_register(CONFIG_PORT0_REG, 0xFF);          // all Hi-Z
    //delay(50);                                                // small pause
}

/**
 * Drive the selected segments low while COM stays high, then Hi-Z them.
 *
 *  portSel     – 0 for P0.x (tens digit), 1 for P1.x (ones digit)
 *  mask        – bit-mask of the one-or-two segments we’re clearing
 *  t_set_ms  – how long to keep the field applied
 */
void set_group(uint8_t portSel,
               uint8_t mask,
               uint16_t t_set_ms)
{
    const uint8_t COM_MASK   = BIT7;                          // COM = P0.7
    const uint8_t cfgReg     = (portSel == 0) ? CONFIG_PORT0_REG
                                              : CONFIG_PORT1_REG;
    const uint8_t outReg     = (portSel == 0) ? OUTPUT_PORT0_REG
                                              : OUTPUT_PORT1_REG;

    // COM - common electrode is on PORT0
    if (portSel == 0) {
        /* --- drive: COM low, segments high -------------------------------- */
        write_tcal9539_register(cfgReg, ~(COM_MASK | mask));  // COM + seg = outputs
        write_tcal9539_register(outReg, mask);                // COM low, segs high (Vd = 1.5 V)
    } else {
        write_tcal9539_register(CONFIG_PORT0_REG, ~COM_MASK);
        write_tcal9539_register(cfgReg, ~mask);
        //common electrode low, everything else high for 1.5V across segments
        //always port 0
        write_tcal9539_register(OUTPUT_PORT0_REG, ~COM_MASK);
        write_tcal9539_register(outReg, mask);
    }

    delay(t_set_ms);

    /* --- Hi-Z PORT1 first */
    if (portSel == 1) {
        write_tcal9539_register(cfgReg, 0xFF);
    }

    // will always need to make Hi-Z since COM always used
    write_tcal9539_register(CONFIG_PORT0_REG, 0xFF);                    // all Hi-Z
    //delay(50);                                                // small pause

}

/*******************************Driver/Patch Table Format*******************************/
/*
 *   Address    Value           Comment
 *
 *   0xFFCE     0xCECE          The driver table start key, always same address (0xFFCE)
 *
 *   0xFFCC     0x1B00          The command ID of the digital sensor sampling function
 *   0xFFCA     Address         The address of the driver sensor sampling function in FRAM
 *
 *   0xFFC8     0x0100          The digital sensor function driver initialization function
 *   0xFFC6     Address         The address of the driver function initialization in FRAM
 *
 *
 *   Optional:
 *   0xFFC4     ID              Another driver/patch function ID
 *   0xFFC2     Address         Address of the function above
 *
 *      *          *            Pairs
 *      *          *
 *
 *   End optional
 *
 *   0xFFC4     0xCECE          Ending key
 *****************************************************************************************/
  /* If start key not present in starting location, table does not exist
   *  If it does, a ROM routine will parse it and setup the calls to be made to the
   *  appropriate address when needed.
   */
 /*****************************************************************************************/

//Start key
#pragma RETAIN(START_KEY);
#pragma location = DRIVER_TABLE_START
const uint16_t START_KEY = DRIVER_TABLE_KEY;

////Custom Command
#pragma RETAIN(CustomCommandID);
#pragma location = CUSTOM_COMMAND                                                       // the location of the command ID
const uint16_t  CustomCommandID = USER_CUSTOM_COMMAND_ID;                                  // the function identifier

// Function address
#pragma RETAIN(CustomCommandAddress);
#pragma location = CUSTOM_COMMAND_ADDR                                                      // the location of the address
const DriverFunction CustomCommandAddress = (DriverFunction)&userCustomCommand;         // the location the function is in

//Ending key
#pragma RETAIN(END_KEY);
#pragma location = DRIVER_TABLE_END
const uint16_t END_KEY = DRIVER_TABLE_KEY;


/**************************************************************************************************************************************************
*  userCustomCommand
***************************************************************************************************************************************************
*
* Brief : This function is called by the RF stack whenever a custom command by its ID number is transmitted
*
* Param[in] :   None
*
* Param[out]:   None
*
* Return        None
*
* This is an example only, and the user if free to modify as needed.
*
* Operation: Example with TRF7970AEVM
* Use Test tab to send following sequence: 18 02 AA 07 10 10
* 18 - TRF7970AEVM Host command (omit for other readers - not sent out over RF)
* 02 - High speed mode selection (start of actual RF packet)
* AA - The actual custom command
* 07 - TI Manufacturer ID (need by this IC)
* 01 - Set Error LED to on  (0x00 to be off)
**************************************************************************************************************************************************/
// RFM13 ISR context
void userCustomCommand()
{
    uint8_t control;

    if( RF13MFIFOFL_L == CRC_LENGTH_IN_BUFFER + DATA_IN_LENGTH)         // CRC_LENGTH + 1 byte expected
    {

        control = RF13MRXF_L;  // pull one byte from the receive FIFO

        Event event;
        event.type = EVENT_NFC;
        event.data = control;
        event_queue_put(&event_queue, &event);

        //Device has 32 byte RX FIFO and 32 byte TX FIFO, this includes the CRC bytes
        //use RF13MRXF to receive two bytes
        //use RF13MRXF_L to receive one byte
        //to receive more than one byte simply continue to read the RF13MRXF_L register.
        //The limit is 32 bytes, but in reality it is less due to protocol overhead

        // Instead of immediate return here, wait until display has been updated to indicate success
        //RF13MTXF_L = 0x03;      // no error, send out
        //To transmit more than one byte repeatedly write data into RF13MTXF_L for each data byte/word and it will go into the FIFO to be transmitted

        // HACK: In order to get CPU out of low power power mode for the main loop to run we need to trigger a timer interrupt
        // This patch function is called by the RFM13 ISR but since this is a callback and not within the actual ISR
        // we can't use __bic_SR_register_on_exit(LPM0_bits); so workaround is to setup the timer to go off
        //RF13MTXF_L = 0x01;
        wakeup();

    }
    else
    {
       RF13MTXF_L = ERROR_UNEXPECTED_NFC_LENGTH;    // an error response
    }
}


