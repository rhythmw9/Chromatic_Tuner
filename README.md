# Chromatic Tuner (FPGA + MicroBlaze)

A real-time chromatic tuner implemented on an FPGA using a MicroBlaze soft processor.  
The system performs FFT-based pitch detection, maps frequencies to musical notes, and displays tuning information on an LCD with a smooth, product-style user interface.

This project was developed as part of UC Santa Barbara's Hardware/Software Interface course (ECE 153A) and was extended beyond the base requirements with additional DSP processing, UI polish, and multiple operating modes.

---

## Demo

**Full Demo Video:**  
https://youtube.com/shorts/XuDOSCFFoCc?si=rEiC-t--6U6aCzS7

The demo shows:
- Real-time tuning with cents display
- Low and high frequency note detection
- Debug spectrum visualization
- Calibration mode (adjustable A4 reference)
- Smooth UI transitions and welcome screen

---

## Features

- Real-time chromatic pitch detection
- FFT-based frequency estimation
- Hann windowing, DC offset removal, and decimation
- Smooth cents bar and numeric frequency display
- Debug modes with live FFT spectrum and DSP parameters
- Calibration mode for adjusting reference pitch (A4)
- Hierarchical state machine for input handling
- Polished UI with reduced flicker and controlled redraws
- Startup welcome screen

---

## System Overview

Audio is captured from a microphone, preprocessed, and transformed using an FFT to estimate the fundamental frequency.  
The detected frequency is then mapped to the nearest musical note and cents offset, which are displayed on an LCD in real time.

The system is event-driven and designed to remain responsive under rapid button presses and encoder input while maintaining stable visual output.

---

## Hardware Platform

- Nexys A7 FPGA development board with MicroBlaze soft processor
- Microphone input
- SPI-connected color LCD
- Push buttons and rotary encoder for user input

The MicroBlaze runs bare-metal C code and interfaces with hardware peripherals through memory-mapped I/O.

---

## DSP Approach

The FFT algorithm itself was provided by the lab framework, but I designed and implemented the surrounding signal-processing pipeline and significantly optimized how the FFT was used, reducing latency and improving real-time performance.

Key steps include:
- **DC offset removal** to eliminate microphone bias
- **Decimation** to improve low-frequency resolution
- **Hann windowing** to reduce spectral leakage
- **FFT magnitude analysis** to identify the dominant frequency
- **Calibration scaling** to align detected frequencies with musical reference tuning

These steps significantly improved detection stability and accuracy across a wide frequency range.

---

## User Interface Design

The UI was designed to feel responsive and readable rather than flashy.

Key design choices:
- Partial redraws instead of full screen clears
- UI update throttling to reduce flicker
- Incremental cents bar drawing
- Clear separation between static and dynamic screen elements
- Multiple modes (Main, Debug, Calibration) with distinct layouts

The result is a smooth, readable interface suitable for real-time tuning.

---

## Control & State Management

The application uses a **hierarchical state machine (QP-Nano)** to manage modes and user input.

This approach allows:
- Clean separation between tuning, debug, calibration, and welcome states
- Reliable handling of rapid button presses
- Encoder input without blocking or missed events
- Deterministic behavior even under frequent UI updates

All user interaction is event-driven rather than polled.

---

## Results & Accuracy

- Correct note detection across the lab-required frequency range
- Accurate cents display for sharp and flat tones
- Proper handling of enharmonic equivalents (e.g., Bb shown as A#)
- Stable behavior during silence and noisy environments
- Minor limitations only at extreme frequency edges, consistent with FFT resolution constraints

Overall performance meets and exceeds the project requirements.

---

## What I Learned

- Practical FFT-based pitch detection on embedded hardware
- DSP tradeoffs between frequency resolution and responsiveness
- Designing flicker-resistant embedded UIs
- Event-driven state machines for real-time systems
- Integrating DSP, hardware, and UI into a cohesive product

---

## Notes

This repository focuses on application logic, DSP integration, and UI design.  
Vendor-generated files, build artifacts, and platform-specific boilerplate are intentionally excluded.

---

*Developed by Rhythm Winicour-Freeman*
