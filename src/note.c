#include "note.h"
#include <math.h>
//#include "lcd.h"

#define LN2_F 0.69314718056f

//array to store note names for findNote
static char notes[12][3]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

// Equal temperament names, MIDI-style (for findNoteDetailed)
static const char *noteNames[12] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
};

//finds and prints note of frequency and deviation from note
void findNote(float f) {
	float c=261.63;
	float r;
	int oct=4;
	int note=0;
	//determine which octave frequency is in
	if(f >= c) {
		while(f > c*2) {
			c=c*2;
			oct++;
		}
	}
	else { //f < C4
		while(f < c) {
			c=c/2;
			oct--;
		}
	
	}

	//find note below frequency
	//c=middle C
	r=c*root2;
	while(f > r) {
		c=c*root2;
		r=r*root2;
		note++;
	}

   /*
	//determine which note frequency is closest to
	if((f-c) <= (r-f)) { //closer to left note
		WriteString("N:");
		WriteString(notes[note]);
		WriteInt(oct);
		WriteString(" D:+");
		WriteInt((int)(f-c+.5));
		WriteString("Hz");
	}
	else { //f closer to right note
		note++;
		if(note >=12) note=0;
		WriteString("N:");
		WriteString(notes[note]);
		WriteInt(oct);
		WriteString(" D:-");
		WriteInt((int)(r-f+.5));
		WriteString("Hz");
	}
   */
}


void findNoteDetailed(float freq, float a4_ref_hz, const char** noteNameOut, int* octaveOut, int* centsOut){
	// very low or invalid frequency, treat as "no note"
	if (freq < 10.0f) {
		if (noteNameOut) *noteNameOut = "--";
		if (octaveOut) *octaveOut = 0;
		if (centsOut) *centsOut = 0;
		return;
	}

	// frequency -> MIDI note number
	float midi = 69.0f + 12.0f * (logf(freq / a4_ref_hz) / logf(2.0f));

	// round to nearest semitone
	int midi_round = (int)(midi + 0.5f);

	// clamp to a sane range (A0..C8)
	if (midi_round < 21)  midi_round = 21;   // A0
	if (midi_round > 108) midi_round = 108;  // C8

	// ideal frequency of that rounded MIDI note
	float nearestFreq = a4_ref_hz *
		powf(2.0f, (float)(midi_round - 69) / 12.0f);

	// cents offset between measured freq and ideal
	float cents_f = 1200.0f * (logf(freq / nearestFreq) / logf(2.0f));
	int cents = (int)(cents_f + (cents_f >= 0 ? 0.5f : -0.5f)); // round

	// clamp cents a bit just to avoid crazy values if FFT glitches
	if (cents < -99) cents = -99;
	if (cents >  99) cents =  99;

	// derive note index and octave from MIDI
	int noteIndex = midi_round % 12;
	int octave = midi_round / 12 - 1;  // standard MIDI octave scheme

	// write outputs
	if (noteNameOut) *noteNameOut = noteNames[noteIndex];
	if (octaveOut) *octaveOut = octave;
	if (centsOut) *centsOut = cents;
}


