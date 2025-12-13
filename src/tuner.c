#include <stdio.h>
#include "qpn_port.h"
#include "tuner.h"
#include "fft.h"
#include "note.h"
#include "stream_grabber.h"
#include "xil_printf.h"
#include "tuner_display.h"
#include <math.h>
#include <string.h>
#include <stdint.h>

#define SAMPLES 512 // AXI4 Streaming Data FIFO has size 512
#define M 9 
#define CLOCK 100000000.0 //clock speed

#define MODE_MAIN   0
#define MODE_DEBUG  1
#define MODE_CAL    2

#define DEBUG_NBINS 64

#define RAW_BLOCKS     4
#define RAW_SAMPLES    (RAW_BLOCKS * SAMPLES)   // 4 * 512 = 2048
#define DECIM_FACTOR 4

#define FREQ_CAL (440.0f / 453.0f) 	// about 0.971

#define PKMAG_MIN 3.0f

// Layout constants
#define SCREEN_W     240
#define SCREEN_H     320

#define SPEC_X0      10
#define SPEC_X1      (SCREEN_W - 10)
#define SPEC_Y0      110
#define SPEC_Y1      (SCREEN_H - 10)

#define WELCOME_TICKS 500


int int_buffer[SAMPLES];
static float q[SAMPLES];
static float w[SAMPLES];

static float dbg_mag[DEBUG_NBINS];

static int32_t raw_int[RAW_SAMPLES];

static float sample_f = 0.0f;

static int have_note_displayed = 0;

static float freq_display = 0.0f;
static float cents_display = 0.0f;
static int   frame_div     = 0;

// simple UI smoothing
static float cents_smooth = 0.0f;
static int   smooth_inited = 0;


// Forward QP declarations
static QState Tuner_initial (Tuner *me);
static QState Tuner_welcome(Tuner *me);
static QState Tuner_tuner   (Tuner *me);
static QState Tuner_tuning(Tuner *me);
static QState Tuner_idle    (Tuner *me);

static void Tuner_runOnce(void);    // one complete tuner iteration


// forward declaration for helper to read from stream grabber
static void read_fsl_values(float *q, int n);

// forward declaration for one-time init
static void Tuner_hwInit(void);


// global instance
Tuner HSM_Tuner;

/* One time DSP Initialization */
static void Tuner_hwInit(void) {
    // 100 MHz clock / 2048 decimation
    sample_f = CLOCK / 2048.0f;

    fft_init(SAMPLES, M);

    //xil_printf("Tuner_hwInit: sample_f = %d Hz\r\n", (int)(sample_f + 0.5f));
}



/* Constructs the HSM and initializes extended state variables  */
void Tuner_ctor(void) {
    // hook QP to initial state handler
    QHsm_ctor(&HSM_Tuner.super, (QStateHandler)&Tuner_initial);

    // init extended state
    HSM_Tuner.mode = TUNER_MODE_MAIN;
    HSM_Tuner.freq_hz = 0.0f;
    HSM_Tuner.ref_a4_hz = 440.0f;          // default reference pitch
    HSM_Tuner.debug_page = 0; 	// start with debug page 0 (spectrum)
}


/* Initial Psudo-State*/
static QState Tuner_initial(Tuner *me) {
    BSP_display("Tuner: INITIAL\n");
    Tuner_hwInit();
    return Q_TRAN(&Tuner_tuner); 
}


/* Displays Welcome Screen */
static QState Tuner_welcome(Tuner *me){
	switch(Q_SIG(me)){
        case Q_ENTRY_SIG : {
            BSP_display("welcome: ENTRY\n");
            me->welcome_ticks = 0;
            tuner_draw_welcome_screen();
            return Q_HANDLED();
        }
        case TICK_SIG : {
            me->welcome_ticks++;
            if(me->welcome_ticks >= WELCOME_TICKS){
                BSP_display("welcome: done -> idle\n");
                return Q_TRAN(&Tuner_idle);
            }
            return Q_HANDLED();
        }
	}
	return Q_SUPER(&Tuner_tuner);
}


/* Top-level superstate that handles mode switching and user input events (button/encoder) */
static QState Tuner_tuner(Tuner *me) {
    switch (Q_SIG(me)) {
        case Q_ENTRY_SIG: {
            BSP_display("Tuner: ENTRY\n");
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            BSP_display("Tuner: EXIT\n");
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            BSP_display("Tuner: INIT -> welcome\n");
            me->mode = MODE_MAIN;
            return Q_TRAN(&Tuner_welcome);
        }
        case BTN_MAIN_SIG: {
            me->mode = MODE_MAIN;
            tuner_draw_home_static();
            tuner_draw_main_header();
            tuner_draw_ref_label(me->ref_a4_hz);   // show current A4
            xil_printf("Mode -> MAIN\r\n");
            return Q_HANDLED();
        }
        case BTN_DEBUG_SIG: {
            xil_printf("BTN_DEBUG pressed\r\n");
            if (me->mode != TUNER_MODE_DEBUG) {
                // entering debug mode from MAIN or CAL
                me->mode = TUNER_MODE_DEBUG;
                me->debug_page = 0;              // start on spectrum page
                tuner_draw_debug_screen();
                xil_printf("Mode -> DEBUG (page 0: spectrum)\r\n");
            } else {
                // already in DEBUG: toggle between page 0 and page 1
                me->debug_page ^= 1;             // 0 -> 1, 1 -> 0
                if (me->debug_page == 0) {
                    tuner_draw_debug_screen();
                    xil_printf("DEBUG: page 0 (spectrum)\r\n");
                } else {
                    tuner_draw_debug2_screen();
                    xil_printf("DEBUG: page 1 (mode 2)\r\n");
                }
            }
            return Q_HANDLED();
        }
        case BTN_CAL_SIG: {
            me->mode = MODE_CAL;
            tuner_draw_cal_header();
            xil_printf("Mode -> CAL\r\n");
            tuner_draw_cal_screen(me->ref_a4_hz);
            return Q_HANDLED();
        }
        case ROT_CW_SIG: {
            if (me->mode == TUNER_MODE_CAL) {
                // bump reference up, clamp at 450
                me->ref_a4_hz += 1.0f;
                if (me->ref_a4_hz > 460.0f) {
                    me->ref_a4_hz = 460.0f;
                }
                tuner_update_cal_ref(me->ref_a4_hz);
            }
            return Q_HANDLED();
        }
        case ROT_CCW_SIG: {
            if (me->mode == TUNER_MODE_CAL) {
                // bump reference down, clamp at 430
                me->ref_a4_hz -= 1.0f;
                if (me->ref_a4_hz < 420.0f) {
                    me->ref_a4_hz = 420.0f;
                }
                tuner_update_cal_ref(me->ref_a4_hz);
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&QHsm_top);
}

/* Active tuning state that runs one processing iteration every tick */
static QState Tuner_tuning(Tuner *me){
	switch(Q_SIG(me)){
        case Q_ENTRY_SIG: {
            BSP_display("tuning-ENTRY\n");
            return Q_HANDLED();
        }
        case TICK_SIG: {
            Tuner_runOnce();
            return Q_HANDLED();
        }
	}
	return Q_SUPER(&Tuner_tuner);
}

/* Idle state that draws main UI and transitions into tuning state */
static QState Tuner_idle(Tuner *me) {
    switch (Q_SIG(me)) {
        case Q_ENTRY_SIG: {
            BSP_display("idle: ENTRY\n");
            // Draw the main tuner UI
            tuner_draw_home_static();
            tuner_draw_main_header();
            tuner_draw_ref_label(me->ref_a4_hz);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            BSP_display("idle: EXIT\n");
            return Q_HANDLED();
        }
        case TICK_SIG: {  
            BSP_display("idle: TICK -> tuning\n");
            return Q_TRAN(&Tuner_tuning);
        }
    }
    return Q_SUPER(&Tuner_tuner);
}


// log2 helper
static float my_log2f(float x) {
    return logf(x) / 0.69314718f;   // ln(2) ≈ 0.69314718
}

/* Captures multiple raw sample blocks */
static void capture_raw_block(int32_t *dst, int total_samples) {
    int offset = 0;
    while (offset < total_samples) {
        int chunk = SAMPLES;  // 512 per capture

        stream_grabber_start();
        stream_grabber_wait_enough_samples(SAMPLES);

        for (int i = 0; i < chunk; ++i) {
            dst[offset + i] = stream_grabber_read_sample(i);
        }

        offset += chunk;
    }
}

/* Builts the FFT input frame using DC removal, decimation, scaling, and a Hann window */
static void build_fft_frame_from_raw(void) {
    // compute DC offset over RAW_SAMPLES
    int64_t sum = 0;
    for (int i = 0; i < RAW_SAMPLES; ++i) {
        sum += raw_int[i];
    }
    // RAW_SAMPLES = 2048 = 2^11, so divide by 2048 via shift
    int32_t dc = (int32_t)(sum >> 11);   // avg sample

    // decimate + convert to float volts + remove DC
    const float scale = 3.3f / 67108864.0f;

    for (int i = 0; i < SAMPLES; ++i) {
        int idx = i * DECIM_FACTOR;      // 0,4,8,... for DECIM_FACTOR=4
        if (idx >= RAW_SAMPLES) {
            // safety – shouldn’t happen if RAW_SAMPLES == SAMPLES*DECIM_FACTOR
            q[i] = 0.0f;
        } else {
            int32_t x = raw_int[idx] - dc;
            q[i] = (float)x * scale;
        }
        w[i] = 0.0f;   // imag part starts at 0
    }

    // apply Hann window to q[]
    for (int i = 0; i < SAMPLES; ++i) {
        float win = 0.5f - 0.5f * cosf(2.0f * 3.141592f * i / (SAMPLES - 1));
        q[i] *= win;
    }
}


/* Executes one full tuning cycle: sample capture, FFT/pitch estimate, validation/smoothing, 
   note/cents mapping, mode dependent UI updates */
static void Tuner_runOnce(void) {
    float frequency;
    int   i;

    // statics for smoothing and UI cycling
    static float freq_smooth = 0.0f;
    static int   ui_divider  = 0;        // how many times we've run since last UI update

    const float alpha = 0.30f;           // smoothing factor: 0.0 = very slow, 1.0 = no smoothing

    // check to see if LCD needs to be redrawn
    int do_ui = 0;
    ui_divider++;
    if(ui_divider >= 6){
    	ui_divider = 0;
    	do_ui = 1;
    }

    // effective sample rate after decimation
    float sample_f_eff = sample_f / (float)DECIM_FACTOR;

    //capture one frame from mic via stream grabber
    capture_raw_block(raw_int, RAW_SAMPLES);

    // build FFT frame
    build_fft_frame_from_raw();

    // run FFT
    frequency = fft(q, w, SAMPLES, M, sample_f_eff);
    float peak_mag = fft_get_last_peak_mag();
    frequency *= FREQ_CAL;

    // peak magnitude strength gate
    if (peak_mag < PKMAG_MIN) {
        // decay smoothed frequency toward zero
        freq_smooth *= 0.9f;
        if (freq_smooth < 1.0f) freq_smooth = 0.0f;
        HSM_Tuner.freq_hz = freq_smooth;
        if (do_ui) {
            if (HSM_Tuner.mode == MODE_MAIN) {
                if(have_note_displayed){
                	// clear main display: freq, note, cents
					tuner_update_display(0.0f, 0);
					tuner_clear_note_box();
					tuner_clear_cents_bar();
					have_note_displayed = 0;
                }
            } else if (HSM_Tuner.mode == MODE_DEBUG) {
            	float sample_f_eff = sample_f / (float)DECIM_FACTOR;
            	float bin_hz = sample_f_eff / (float)SAMPLES;
                if (HSM_Tuner.debug_page == 0) {
                    // page 0: basic debug text
                    tuner_debug_update(0.0f, "--", 0, 0);
                    // clear the spectrum
                    setColor(0, 0, 0);
                    fillRect(SPEC_X0, SPEC_Y0, SPEC_X1, SPEC_Y1);
                } else {
                    // page 1: show config + zero signal
                    tuner_debug2_update(0.0f, 0, sample_f_eff, bin_hz, 0.0f);   // pkMag = 0 → clearly "no signal"
                }
            }
        }

        //xil_printf("PkMag low (%.2f) -> no note\n", peak_mag);
        return;
    }

    // compute magnitudes for first DEBUG_NBINS bins (for the spectrum plot)
    int mag_bins = DEBUG_NBINS;
    if (mag_bins > SAMPLES / 2) {
        mag_bins = SAMPLES / 2;
    }
    for (i = 0; i < mag_bins; ++i) {
        float re = q[i];
        float im = w[i];
        dbg_mag[i] = sqrtf(re * re + im * im);
    }
    // if mag_bins < DEBUG_NBINS, zero the rest
    for (i = mag_bins; i < DEBUG_NBINS; ++i) {
        dbg_mag[i] = 0.0f;
    }

    // basic validity check on raw frequency
    if (frequency < 10.0f) {
        // too low / no signal – decay the smoothed value toward 0
        freq_smooth *= 0.9f;
        if (freq_smooth < 1.0f) {
            freq_smooth = 0.0f;
        }
        HSM_Tuner.freq_hz = freq_smooth;
    } else {
        // first valid reading, jump straight there
        if (freq_smooth <= 0.0f) {
            freq_smooth = frequency;
        } else {
            // exponential smoothing
            freq_smooth = (1.0f - alpha) * freq_smooth + alpha * frequency;
        }
        HSM_Tuner.freq_hz = freq_smooth;
    }
    // if we still don't have a valid frequency, show "no signal"
    if (freq_smooth < 10.0f) {
    	HSM_Tuner.freq_hz = freq_smooth;
		if (do_ui) {
			if (HSM_Tuner.mode == MODE_MAIN) {
				if(have_note_displayed){
					tuner_update_display(0.0f, 0);
					tuner_clear_note_box();
					tuner_clear_cents_bar();
					have_note_displayed = 0;
				}
			} else if (HSM_Tuner.mode == MODE_DEBUG) {
				float sample_f_eff = sample_f / (float)DECIM_FACTOR;
				float bin_hz = sample_f_eff / (float)SAMPLES;

				if (HSM_Tuner.debug_page == 0) {
					tuner_debug_update(0.0f, "--", 0, 0);
				} else {
					tuner_debug2_update(0.0f, 0, sample_f_eff, bin_hz, 0.0f);
				}
			}
		}

		//xil_printf("Freq = --- (no note)\r\n");
		return;
    }

    // turn smoothed frequency into note + cents
    const char *noteName;
    int octave;
    int cents;

    float ref = HSM_Tuner.ref_a4_hz;
    if (ref <= 0.0f) {
        ref = 440.0f;
    }

    findNoteDetailed(freq_smooth, ref, &noteName, &octave, &cents);

    // clamp cents for the bar
    if (cents < -50) cents = -50;
    if (cents >  50) cents =  50;

    // now display a valid note
    have_note_displayed = 1;

    // mode dependent UI update
    if(do_ui){
    	switch (HSM_Tuner.mode) {
			case MODE_MAIN:
				tuner_update_display(freq_smooth, cents);
				tuner_draw_note_label(noteName, octave);
				break;
			case MODE_DEBUG:
				if (HSM_Tuner.debug_page == 0) {
					// page 0: text + spectrum
					tuner_debug_update(freq_smooth, noteName, octave, cents);
					tuner_debug_draw_spectrum(dbg_mag, DEBUG_NBINS);
				} else {
					// page 1: processing params + peak mag
					{
						float bin_hz = sample_f_eff / (float)SAMPLES;
						tuner_debug2_update(freq_smooth, cents, sample_f_eff, bin_hz, peak_mag);
					}
				}
				break;
			case MODE_CAL:
				// no live UI updates here for now
				break;
		}
    }

    // debug print (rounded)
    xil_printf("Freq = %3d Hz, note %s%d, cents = %d\r\n", (int)(freq_smooth + 0.5f), noteName, octave, cents);
}
