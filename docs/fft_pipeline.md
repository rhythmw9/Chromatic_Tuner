# FFT Signal Processing Pipeline

This document describes the signal processing pipeline used in my chromatic tuner project.  
While the FFT algorithm itself was provided by the lab framework, I designed, implemented, and optimized the entire signal processing pipeline around it to achieve accurate and real-time pitch detection.

The goal of this pipeline was not just to “run an FFT,” but to reliably extract a musical fundamental frequency from noisy microphone input with low latency and stable UI behavior.

---

## 1. Overview

The FFT pipeline converts raw microphone samples into a clean frequency estimate suitable for note and cents detection. Each stage in the pipeline addresses a specific real-world issue encountered when working with live audio data, such as DC offset, spectral leakage, poor low-frequency resolution, and excessive computation time.

At a high level, the pipeline is:

**Microphone → DC removal → Decimation → Windowing → FFT → Peak detection → Frequency estimate**

---

## 2. Raw Audio Acquisition

Audio samples are captured from the onboard microphone using the provided AXI stream grabber.  
Instead of processing a single FFT-sized block directly, multiple raw blocks are captured and combined.

This approach allows:
- Better averaging for DC offset removal
- More flexibility in downsampling (decimation)
- Improved stability for low-frequency signals

---

## 3. DC Offset Removal

The raw microphone signal contains a DC offset due to hardware biasing.  
If left uncorrected, this produces a large spike at 0 Hz in the FFT, which interferes with peak detection.

To fix this:
- I compute the average value of the raw samples
- Subtract this DC component from each sample before further processing

This significantly improved FFT stability and eliminated false low-frequency peaks.

---

## 4. Decimation

After DC removal, the signal is decimated by a fixed factor.

Decimation reduces the effective sampling rate, which:
- Improves frequency resolution for low notes
- Reduces computational load
- Makes musical pitch detection more reliable in the lower octaves

A fixed decimation factor was chosen for simplicity and reliability.  
Variable decimation could further improve low-frequency performance, but was avoided to prevent aliasing issues at higher frequencies.

---

## 5. Windowing (Hann Window)

Before performing the FFT, a Hann window is applied to the time-domain samples.

This addresses spectral leakage, which occurs when the signal does not align perfectly with FFT bin boundaries.

The Hann window was chosen because:
- It provides strong sidelobe suppression
- It balances frequency resolution and leakage reduction
- It significantly stabilized peak detection in real audio tests

After adding windowing, the detected frequency became noticeably more consistent, especially when notes were slightly detuned.

---

## 6. FFT Execution and Precomputation

The FFT algorithm itself was provided by the lab framework. However, I significantly improved its performance by restructuring how it was used.

Key improvements:
- FFT twiddle factors were precomputed once during initialization
- FFT calls were isolated to only necessary processing steps
- Unnecessary recomputation and memory overhead were removed

These changes drastically reduced FFT runtime and enabled real-time operation.

---

## 7. Peak Detection and Frequency Estimation

After the FFT:
- Magnitudes are computed from the real and imaginary components
- The strongest spectral peak (excluding DC) is identified
- The peak index is converted into a frequency using the effective sample rate

A minimum peak magnitude threshold is used to:
- Reject background noise
- Prevent false note detection when the room is quiet

---

## 8. Performance Improvements

One of the major accomplishments of this project was reducing FFT processing time:

- Initial implementation: **~1.1 seconds per FFT**
- After early optimizations: **~156 ms**
- Further improvements: **~73 ms**
- Final optimized pipeline: **~17 ms per FFT**

This improvement was critical for:
- Smooth UI updates
- Stable note detection
- A responsive, product-like user experience

---

## 9. Summary

By designing a robust signal processing pipeline around the provided FFT, I transformed a basic lab implementation into a real-time, reliable chromatic tuner.

Key contributions include:
- DC offset removal
- Decimation for low-frequency accuracy
- Hann windowing to reduce spectral leakage
- FFT precomputation and performance optimization
- Practical peak detection with noise rejection

Together, these decisions enabled accurate pitch detection across the full required frequency range while maintaining a smooth and responsive UI.
