# tcg-counter-firmware
Embedded microcontroller firmware for tcg counter

## Documentation

[RF430FRL15xH Firmware Guide](https://www.ti.com/lit/ug/slau603b/slau603b.pdf)

[RF430FRL15xH Datasheet](https://www.ti.com/lit/ds/slas834c/slas834c.pdf)

[RF430FRL15xH Technical Reference Manual](https://www.ti.com/lit/ug/slau506/slau506.pdf)

## Design

Due to the limited power budget of harvested RF energy available, it will likely be necessary to switch each segment of the 7 segment display one at a time rather than all at once to not exceed current sourcing limits of the RF430FRL152H. The segments need to have 1.5V or -1.5V applied for a period of time to fully switch. This will require timing delays in the firmware. Considering the power limitations a software delay will likely not be feasible. Using TimerA0 and putting the CPU into a low power mode (LPM3) until the timer interrupt fires would be the preferred way to handle delays. 

### RF430FRL152H Firmware

An NFC stack implementation exists in the RF430FRL152H ROM. There are different modes of operation can be entered based on the value of the Firmware System Control register. Options include the RF stack, sensor sampling, and I2C/SPI communication. It is also possible to use no ROM firmware and interrupt vectors, etc. point to FRAM. The user application then has complete control. I would like to use the RF stack only but also be able to use Timer_A0 for delay requirements of updating the display. This would require a custom interrupt handler for Timer_A0. If Timer_A0 isn't used by the RF stack portion of the firmware, replacing the interrupt handler should be ok.

To investigate this, I wrote a custom NFC command (timer_register.c) to read the value of TA0CTL which should be 0000h after reset if unchanged. 
