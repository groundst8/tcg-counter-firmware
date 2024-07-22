# tcg-counter-firmware
Embedded microcontroller firmware for tcg counter

## Documentation

[RF430FRL15xH Firmware Guide](https://www.ti.com/lit/ug/slau603b/slau603b.pdf)

[RF430FRL15xH Datasheet](https://www.ti.com/lit/ds/slas834c/slas834c.pdf)

## Design

Due to the limited power budget of harvested RF energy available, it will likely be necessary to switch each segment of the 7 segment display one at a time rather than all at once to not exceed current sourcing limits of the RF430FRL152H. The segments need to have 1.5V or -1.5V applied for a period of time to fully switch. This will require timing delays in the firmware. Considering the power limitations a software delay will likely not be feasible. Using TimerA0 and putting the CPU into a low power mode (LPM3) until the timer interrupt fires would be the preferred way to handle delays. 
