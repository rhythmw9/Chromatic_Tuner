# Hardware Overview

This project is implemented on a Xilinx FPGA development board using a MicroBlaze soft processor as the main control core. The system integrates custom IP blocks, vendor peripherals, and bare-metal C firmware to form a real-time chromatic tuner with audio input, user interaction, and graphical output.

## Processing Platform

- **MicroBlaze Soft Processor**
  - Acts as the central controller for the system
  - Runs bare-metal C code (no operating system)
  - Manages the tuner state machine, DSP pipeline coordination, and UI updates
  - Communicates with peripherals through memory-mapped I/O

## Audio Input Path

- **On-board / External Microphone Interface**
  - Audio samples are captured in hardware and streamed into the system
  - A custom AXI Stream Grabber IP block buffers incoming samples
  - The firmware reads fixed-size sample frames from the stream grabber for processing

This hardware buffering allows deterministic sample acquisition without blocking the main control loop.

## Display Subsystem

- **SPI-Driven Color LCD**
  - Controlled via SPI from the MicroBlaze processor
  - Used to render:
    - Main tuner UI (note, cents bar, frequency)
    - Debug views (spectrum and internal parameters)
    - Calibration and welcome screens
  - Screen updates are rate-limited in software to reduce flicker and improve visual stability

## User Input Devices

- **Push Buttons**
  - Used to switch between tuner modes (Main, Debug, Calibration)
  - Handled through board support package (BSP) input polling

- **Rotary Encoder**
  - Used to adjust the reference tuning frequency (A4) in calibration mode
  - Encoder rotation and press events are decoded in firmware
  - Integrated cleanly into the state machine without blocking execution

## Timing and Control

- **Software-Driven Tick**
  - A periodic software tick drives the state machine and tuning loop
  - Each tick advances the tuner through sampling, processing, and UI update phases
  - This approach keeps the design simple while remaining responsive

## System Design Philosophy

The hardware architecture deliberately keeps complex control logic in software while using hardware primarily for:
- Deterministic data capture
- High-bandwidth I/O
- Display and user interaction

This separation allows rapid iteration on signal processing, UI behavior, and system tuning without modifying the underlying hardware design.
