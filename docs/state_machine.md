# State Machine Design

This project uses a hierarchical state machine (HSM) implemented with QP-Nano to manage application flow, user input, and real-time signal processing in a clean and non-blocking way.

The state machine ensures that UI updates, FFT processing, and rapid button/encoder interactions remain responsive and well organized.

---

## Motivation

A state machine was chosen to:

- Separate UI logic, signal processing, and user input handling
- Avoid blocking behavior during FFT computation
- Handle rapid button presses and encoder changes reliably
- Keep the system easy to extend and reason about

---

## Framework

- Implemented using QP-Nano
- A single Tuner active object drives the entire application
- All events are dispatched through the same state machine

---

## High-Level Structure

The tuner is organized as a top-level superstate with multiple child states:

- **Initial**
- **Welcome**
- **Idle**
- **Tuning**

The top-level `Tuner` state acts as a superstate, handling global events such as button presses and encoder input.

---

## States

### Initial

- Entry point at power-up
- Performs one-time hardware and DSP initialization
- Immediately transitions into the Welcome state

---

### Welcome

- Displays a simple welcome screen
- Remains active for a fixed number of tick events
- Transitions automatically to the Idle state after timeout

---

### Idle

- Draws static UI elements (labels, headers, reference pitch)
- Acts as a staging state before tuning begins
- Transitions to the Tuning state on the next tick

---

### Tuning

- Runs continuously once entered
- Executes one full FFT-based pitch detection per tick
- Updates frequency, note, cents, and debug data
- UI updates are throttled to reduce flicker and improve smoothness

---

## State Hierarchy

- **Tuner (superstate)**
  - Handles:
    - Button presses (Main, Debug, Calibration)
    - Encoder rotation and press events
  - Manages internal mode changes without changing states

- **Child states**
  - Welcome
  - Idle
  - Tuning

This hierarchy allows user input to be processed even while tuning is active.

---

## Transitions

The core state transitions are:

1. **Initial → Welcome**  
   (Startup)

2. **Welcome → Idle**  
   (After welcome timeout)

3. **Idle → Tuning**  
   (First periodic tick)

Once in the Tuning state, the system remains there and continues processing.

Mode changes (Main / Debug / Calibration) are handled via internal variables rather than state transitions.

---

## Events and Signals

- A periodic **TICK** signal drives the system
- Button and encoder events are dispatched asynchronously
- Events are handled at the superstate level to prevent missed inputs

---

## Design Benefits

- FFT processing never blocks UI responsiveness
- Rapid button presses are safely handled
- Clear separation between control flow and signal processing
- Easy to extend with additional screens or modes

---

## Diagram Scope

The state diagram intentionally shows only the high-level transitions to remain readable.

Detailed button, encoder, and internal variable behavior is documented in code and described here rather than drawn explicitly.
