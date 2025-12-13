# UI Design

The tuner’s user interface was designed to feel responsive, stable, and product-like, despite running on a resource-constrained embedded system. The primary goals were clarity, low visual noise, and smooth real-time updates.

---

## Design Goals

- Clearly display pitch, note, and tuning error
- Minimize flicker and unnecessary redraws
- Remain readable during quiet or noisy input
- Support multiple modes without clutter
- Respond immediately to buttons and encoder input

---

## Display Modes

### Main Tuner Mode

The primary user-facing mode displays:

- Detected frequency (Hz)
- Musical note and octave
- Cents deviation bar for tuning feedback
- Reference pitch (A4)

When no valid signal is present, all dynamic elements are cleared to avoid false readings or flicker.

---

### Debug Mode

Debug mode provides visibility into the signal processing pipeline and is split into two pages:

#### Debug Page 1 – Spectrum View
- Real-time FFT magnitude spectrum
- Detected frequency, note, and cents
- Useful for validating peak detection and noise behavior

#### Debug Page 2 – Processing Information
- Effective sample rate after decimation
- FFT bin resolution
- Peak magnitude strength
- Current cents deviation

This mode is intended for development, validation, and demonstration rather than normal use.

---

### Calibration Mode

Calibration mode allows the user to adjust the reference pitch:

- Reference A4 frequency is adjusted using the rotary encoder
- Changes are applied immediately
- After a period of inactivity, the system automatically returns to Main mode

This avoids persistent modal states while keeping calibration quick and intuitive.

---

## Welcome Screen

On startup, a simple welcome screen is displayed briefly before entering the main tuner interface.

- Confirms successful boot
- Provides a polished, professional feel
- Automatically transitions without user input

---

## Flicker Reduction Techniques

Several strategies were used to reduce flicker and improve visual stability:

- **UI throttling**: Screen updates occur only every few processing cycles
- **State-aware drawing**: Static UI elements are drawn once per mode
- **Explicit clearing**: Dynamic elements are cleared only when signal loss is confirmed
- **Smoothing**: Frequency and cents values are smoothed before display

These choices significantly improve perceived smoothness without sacrificing responsiveness.

---

## Handling Quiet and Noisy Input

- A peak magnitude threshold determines when a signal is considered valid
- When input drops below this threshold:
  - Note, frequency, and cents displays are cleared
  - Spectrum display is erased in debug mode
- Prevents random noise from triggering visual updates

---

## Input Responsiveness

- Button presses immediately switch modes
- Encoder rotation is processed continuously in calibration mode
- UI updates remain responsive even during FFT processing due to non-blocking design

---

## Overall Approach

The UI is tightly integrated with the state machine and signal processing pipeline but remains logically separated in code. This results in:

- Clean transitions between modes
- Stable visual output
- A user experience similar to a dedicated hardware tuner

The final UI prioritizes usability and polish while remaining simple and efficient.
