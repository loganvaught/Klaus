<video src="https://github.com/user-attachments/assets/65eb8524-2594-48ac-9710-71b7d11167b9" controls width="100%"></video>

# Klaus
Wireless vibration-based metronome to help musicians stay in sync. Built with STM32CubeIDE and STM32CubeMX 
Designed for ensembles that struggle with hearing metronomes in loud environments/rehearsals.
## Features
- Linear resonant actuator for strong click sensations
- Adjustable tempo via rotary encoder
- Wireless syncing with multiple devices
- Battery powered, rechargeable

<img width="1986" height="1238" alt="Devices in Enclosures" src="https://github.com/user-attachments/assets/71dc42b9-95c3-4f01-a95e-4a1ca438d470" />

## Results and Feedback
- Tested with real ensembles to help guide the engineering process
- Very useful for large ensembles / rehearsals with rhythmically complex pieces
- Vibrations are difficult to feel with the ELV1411A, a stronger LRA will be needed in future versions
- Greatly improves ensemble comfort with long periods of silence

<video src="https://github.com/user-attachments/assets/50d66d34-7052-4124-8466-a2ae3fe2bc2e" controls width="100%"></video>

## Project Layout
- Klaus (Repo)
  - Klaus (Container folder)
    - Drivers
      - Custom: Header and source files for NRF24L01+, DRV2605L, and metronome code
    - Core
      - Src: Main.c here
  - Media: Images and videos of project
## Parts
- STM32F411CEU6 Blackpill Board
- 18650 Battery (With protection circuit)
- TP4056 Charging Board
- 5V Boost Converter

- AP2112K-3.3 LDO (or, use a NRF24L01 adapter board. Takes 5V)
- DRV2605L Haptic Board
- ELV1411A LRA
- NRF24L01+ (With PA and LNA)

<img width="2160" height="1651" alt="Devices with Open Enclosures" src="https://github.com/user-attachments/assets/c162a972-fed2-42db-9dc0-27c0166d2794" />

# Design
Note: Block diagram does not show the rotary encoder or the button for mode switching. Assumes useage of 3.3V LDO and no NRF adapter board
<img width="626" height="379" alt="System Block Diagram" src="https://github.com/user-attachments/assets/abc06887-0561-4997-bbaa-62e27158d7ce" />

## Peripherals / Functionality
- I2C: DRV2605L
- SPI: NRF24L01+
- UART: Serial debugging
- Timers: RF syncing and metronome beat generation, microsecond delay for driver setup / mode switching
- GPIO Interrupts: rotary encoder for tempo change, RF interrupt detection
## Design Choices / Tradeoffs
- ELV1411A: LRAs generate stronger "click" sensations for musicians
- PA + LNA version for the NRF may enhance reliability in larger setups.
- Disabled auto-ack/retransmit in the NRF driver to avoid collisions with multiple receivers.
- RF uses only one pipe, because only one is needed for this project
- DRV2605L driver specifically designed for setup with ELV1411A. Tradeoff for simplicity under time constraints
- AP2112K-3.3: Has a high max current for supplying the PA + LNA demands of of the NRF. Alternatively, an adapter board is much simpler to use.
## How to Make
- Clone the repo, and open the Klaus container folder (inside this repo) in STM32CubeIDE
- Use an ST-Link (or other method) to flash the STM32F411CEU6
- View pin mapping by opening .ioc file in STM32CubeMX
- View block diagram below for help putting device together. (Note: diagram does not show rotary encoder or mode-switching button)
- Supply 5v to the STM32 5v pin, and the DRV2605L haptic board. Supply 3.3v to the NRF24L01+ board; or, use an NRF24L01 adapter board, which takes 5V. 
