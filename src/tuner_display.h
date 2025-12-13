#ifndef TUNER_DISPLAY_H
#define TUNER_DISPLAY_H


void tuner_draw_welcome_screen(void);
void tuner_draw_home_static(void);
void tuner_draw_ref_label(float ref_a4_hz);
void tuner_draw_note_label(const char *noteName, int octave);
void tuner_draw_frequency(float f);
void tuner_draw_cents(int cents);
void tuner_update_display(float freq_hz, int cents);
void tuner_clear_note_box(void);
void tuner_clear_cents_bar(void);

void tuner_draw_main_header(void);
void tuner_draw_debug_header(void);
void tuner_draw_cal_header(void);
void tuner_draw_cal_screen(float baseAHz);
void tuner_draw_debug_screen(void);
void tuner_draw_debug2_screen(void);

void tuner_update_cal_ref(float a4_hz);
void tuner_debug_update(float freq_hz, const char* noteName, int octave, int cents);
void tuner_debug2_update(float freq_hz, int cents, float fs_eff_hz, float bin_hz, float peak_mag);
void tuner_debug_draw_spectrum(const float *mag, int n_bins);



#endif
