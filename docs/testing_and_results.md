# Testing and Results

This project was tested using a combination of controlled tone generation, real-time audio input, and interactive user input to validate accuracy, stability, and responsiveness across the full system.

The goal of testing was not only to confirm correctness, but also to ensure the tuner behaved reliably under real-world conditions.

---

## Test Setup

Testing was performed using:

- Digitally generated sine-wave test tones
- External speakers placed near the onboard microphone
- Manual interaction with buttons and rotary encoder
- Visual inspection of LCD output in all modes

Tone frequencies were displayed directly on the tone generator to allow direct comparison with the tuner output.

---

## Frequency Accuracy

The tuner was tested across a wide range of musical notes. Results showed:

- Accurate note identification across the lab-required frequency range
- Correct octave detection
- Cents deviation displayed in the correct direction (sharp vs flat)
- Stable readings once a signal was present

Enharmonic equivalents were handled correctly (e.g., Bb displayed as A#), which is musically valid.

---

## Sample Test Cases

| Test Note | Expected | Result |
|----------|---------|--------|
| E2 | E2 | Correct |
| F3 (-22¢) | F3 (flat) | Correct |
| G4 (+37¢) | G4 (sharp) | Correct |
| A3 (-11¢) | A3 (flat) | Correct |
| D5 | D5 | Correct |
| Bb5 | A#5 | Correct (enharmonic) |

All other tested notes produced correct pitch and cents behavior.

---

## Low-Frequency Performance

- Low-frequency notes were tested using both live sound and test tones
- The tuner correctly detected fundamentals within the expected range
- Performance degrades gracefully below the practical limits of the FFT resolution

These limitations are expected given the fixed FFT size and decimation factor and were deemed acceptable for the project scope.

---

## Noise and Silence Handling

When no valid signal is present:

- No note or frequency is displayed
- Cents bar is cleared
- Debug spectrum is erased

Random noise and small background sounds do not produce false note readings, improving usability.

---

## UI Stability and Responsiveness

- UI flicker was significantly reduced through update throttling
- Dynamic elements update smoothly during sustained notes
- Button presses and encoder changes are handled immediately
- Mode transitions remain responsive even during continuous FFT processing

---

## Performance Improvements

Although the FFT algorithm itself was provided, substantial performance improvements were achieved by optimizing the surrounding signal processing pipeline.

FFT execution time was reduced through incremental improvements:

- Original implementation: ~1.1 seconds per FFT
- Intermediate optimizations: ~156 ms → ~73 ms
- Final optimized pipeline: ~17 ms per FFT

This enabled real-time pitch tracking and a smooth UI experience.

---

## Overall Results

The final system:

- Meets all functional lab requirements
- Accurately detects pitch and cents in real time
- Handles noise and silence gracefully
- Provides a polished and responsive user interface
- Operates efficiently within embedded system constraints

The tuner performs reliably as both a functional instrument tuner and a demonstrable embedded signal processing system.
