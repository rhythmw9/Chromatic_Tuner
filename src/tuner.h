#ifndef TUNER_H
#define TUNER_H

#include "qpn_port.h"


// Signals (events) for the tuner
enum TunerSignals {
	TICK_SIG = Q_USER_SIG,
	ROT_CW_SIG,
	ROT_CCW_SIG,
	BTN_MAIN_SIG,
	BTN_DEBUG_SIG,
	BTN_CAL_SIG,
	FFT_READY_SIG,
	TERMINATE_SIG
};


// High-level tuner modes
typedef enum {
    TUNER_MODE_MAIN = 0,   // normal chromatic tuner
    TUNER_MODE_DEBUG,      // debug / FFT view 
    TUNER_MODE_CAL,        // calibration / alt tuning 
    TUNER_MODE_MAX         // sentinel for wrap-around
} TunerMode;


// tuner active object type
typedef struct TunerTag {
	QHsm super;

	float freq_hz;
	int note_index;
	int octave;
	int volume_level;
	TunerMode mode;
	float ref_a4_hz;
	int debug_page; 	// 0 = spectrum, 1 = debug page 2
	int welcome_ticks;
} Tuner;


// global instance
extern Tuner HSM_Tuner;

// methods
void Tuner_ctor(void);
void BSP_display(char const *msg);
void BSP_exit(void);



#endif
