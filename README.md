# Klaus
Personal vibration-based metronome project to help musicians stay in sync. Built with STM32CubeIDE and STM32CubeMX 

# Purpose / Background Info
I designed Klaus for my string quartet. We had trouble navigating a difficult piece because the metronome was too quiet. I initially designed an LED metronome (claw shaped - hence the name, Klaus), which failed to help. By using vibrations, Klaus has solved our problem, and I hope that it can be used to help other ensembles.

# Features
- LRA generates strong click sensations
- Adjustable tempo via rotary encoder
- Wireless syncing with multiple devices (nrf24l01)
- Battery powered, charging
- Expandable for various ensemble sizes

# Parts
- STM32F411CEU6 Blackpill Board
- 18650 Battery
- TP4056 Charging Breakout Board
- 5V Boost Converter
- DRV2605L Haptic Breakout Board
- ELV1411A LRA
- nRF24L01 + PA + LNA Board

# Peripherals / Functionality
- I2C - communication with DRV2605L
- SPI - communication with nRF24L01
- UART - communication with a serial adapter for debugging
- Timers - generate precise beats
- GPIO External Interrupts - rotary encoder to change metronome tempo

# Tradeoffs and Design Choices
- ELV1411A - LRAs generate stronger "click" sensations for musicians
- nRF24L01 + PA + LNA - Overkill for small ensemble settings, but enhances reliability in larger setups

# System Overview / Diagram
- Block diagram to be added

# Driver Information
DRV2605L Driver
- Follows an initialization procedure from the TI datasheet
- Sets up registers for auto-calibration
- Accesses a built-in waveform library to drive the LRA

nRF24L01 Driver
- Not added yet

# What I Learned
- Using a multimeter to troubleshoot circuits and navigate missing documentation
- How to safely solder parts and avoid damage
- Navigating datasheets, manipulating registers with I2C and SPI
- How I2C, SPI, UART, and RF communication work
- How to avoid electromagnetic noise, smoothen out voltage ripples, and avoiding sags
- Bit masking, interrupt management, optimization
