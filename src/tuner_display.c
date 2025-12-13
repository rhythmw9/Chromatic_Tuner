#include "lcd.h"
#include "tuner_display.h"

// Layout constants
#define SCREEN_W     240
#define SCREEN_H     320

// Note box geometry
#define NOTE_BOX_X1  30
#define NOTE_BOX_Y1  40
#define NOTE_BOX_X2  209
#define NOTE_BOX_Y2  120

#define NOTE_TEXT_X  (NOTE_BOX_X1 + 70)
#define NOTE_TEXT_Y  (NOTE_BOX_Y1 + 20)

// Frequency text position
#define FREQ_X       40
#define FREQ_Y       150
#define FREQ_VAL_X 	(FREQ_X + 50)
#define FREQ_VAL_Y (FREQ_Y)
#define FREQ_VAL_W 100
#define FREQ_VAL_H 20

// Cents bar geometry
#define BAR_X1       20
#define BAR_X2       219
#define BAR_Y1       200
#define BAR_Y2       230
#define BAR_MID      ((BAR_X1 + BAR_X2) / 2)

// colors
#define BLACK   0,0,0
#define WHITE   255,255,255
#define RED     255,0,0
#define GREEN   0,255,0
#define BLUE    0,0,255
#define YELLOW  255,255,0
#define CYAN    0,255,255
#define MAGENTA 255,0,255

// calibration text
#define CAL_A4_X   40
#define CAL_A4_Y   120
#define CAL_A4_W   160
#define CAL_A4_H   24

// A4 reference label position/region
#define REF_X       70
#define REF_Y       24
#define REF_W       120
#define REF_H       16

// Debug info text region
#define DBG_FREQ_X   10
#define DBG_FREQ_Y   40
#define DBG_FREQ_W   220
#define DBG_FREQ_H   20
#define DBG_NOTE_X   10
#define DBG_NOTE_Y   60
#define DBG_CENTS_X  10
#define DBG_CENTS_Y  80

#define DBG_LINE_W   (SCREEN_W - 20)
#define DBG_LINE_H   14

// Spectrum area (under the text lines)
#define SPEC_X0      10
#define SPEC_X1      (SCREEN_W - 10)
#define SPEC_Y0      110
#define SPEC_Y1      (SCREEN_H - 10)

// Number of bins for the spectrum
#define DEBUG_NBINS 64

static int prev_cents_x = -1;

extern struct _current_font cfont;
extern u8 BigFont[];




/* ======================================================*/
/*                    Home/Main Screen                   */
/* ======================================================*/

/* Draws the startup welcome screen on the LCD */
void tuner_draw_welcome_screen(void) {
    // full screen clear
    setColor(BLACK);
    clrScr();

    // clear "main mode" text
    setColor(BLACK);
    fillRect(0, 0, SCREEN_W - 1, 19);

    // big title text
    setFont(BigFont);
    setColor(WHITE);  

    const char *title = "CHROMATIC";
    const char *title2 = "TUNER";

    int len1 = 0, len2 = 0;
    while (title[len1]  != '\0') len1++;
    while (title2[len2] != '\0') len2++;

    int text_w1 = cfont.x_size * len1;
    int text_w2 = cfont.x_size * len2;
    int text_h  = cfont.y_size;

    int x1 = (SCREEN_W - text_w1) / 2;
    int x2 = (SCREEN_W - text_w2) / 2;
    int y1 = 100;
    int y2 = y1 + text_h + 10;

    lcdPrint((char *)title,  x1, y1);
    lcdPrint((char *)title2, x2, y2);

    setFont(SmallFont);
    setColor(200, 200, 200); 	// light grey
    lcdPrint("Welcome!", 80, 200);
}

/* Draws the main tuner UI elements that don’t change */
void tuner_draw_home_static(void){

	// background
    setColor(BLACK);   
    clrScr();

    // title
    setFont(SmallFont);
    setColor(WHITE);
    
    // note box
    setColor(40, 40, 40);   // dark gray background for the note box
    fillRect(NOTE_BOX_X1, NOTE_BOX_Y1, NOTE_BOX_X2, NOTE_BOX_Y2);

    // frequency label 
    setFont(SmallFont);
    setColor(WHITE);
    lcdPrint("Freq:", FREQ_X, FREQ_Y);
    lcdPrint("---.-- Hz", FREQ_X + 50, FREQ_Y);

    // cents bar (outline)
    setColor(80, 80, 80);  // outline color (darker grey)
    fillRect(BAR_X1, BAR_Y1, BAR_X2, BAR_Y1+1);      // top line
    fillRect(BAR_X1, BAR_Y2, BAR_X2, BAR_Y2+1);      // bottom line
    fillRect(BAR_X1, BAR_Y1, BAR_X1+1, BAR_Y2);      // left line
    fillRect(BAR_X2, BAR_Y1, BAR_X2+1, BAR_Y2);      // right line

    // center line (0 cents marker)
    setColor(WHITE);
    fillRect(BAR_MID, BAR_Y1, BAR_MID+1, BAR_Y2);
}

/* Updates the displayed A4 reference frequency label on the home screen */
void tuner_draw_ref_label(float ref_a4_hz) {
    char buf[24];

    // clear the A4 label region
    setColor(BLACK);   // background
    fillRect(REF_X, REF_Y, REF_X + REF_W, REF_Y + REF_H);

    setFont(SmallFont);
    setColor(WHITE);  

    if (ref_a4_hz < 1.0f) {
        lcdPrint("A4 = ---- Hz", REF_X, REF_Y);
        return;
    }

    // integer Hz is fine for reference pitch
    sprintf(buf, "A4 = %3.0f Hz", ref_a4_hz);
    lcdPrint(buf, REF_X, REF_Y);
}

/* Clears and redraws the detected note + octave centered in the note box */
void tuner_draw_note_label(const char *noteName, int octave){
	char label[5]; 	// enough for note text (e.g. A#10)
	int i = 0;

	// build label string
	while(noteName[i] != '\0' && i < 3){
		label[i] = noteName[i];
		i++;
	}

	// append the octave digit
	label[i++] = (char)('0' + octave);
	label[i] = '\0';

	setFont(BigFont);
	setColor(WHITE);

	// compute text width in pixals
	int len = 0;
	while(label[len] != '\0'){
		len++;
	}

	int box_w = NOTE_BOX_X2 - NOTE_BOX_X1 + 1;
	int box_h = NOTE_BOX_Y2 - NOTE_BOX_Y1 + 1;

	int text_w = cfont.x_size * len;
	int text_h = cfont.y_size;

	int x = NOTE_BOX_X1 + (box_w - text_w) / 2;
	int y = NOTE_BOX_Y1 + (box_h - text_h) / 2;


	// clear old note first, then draw new label
	tuner_clear_note_box();
	lcdPrint(label, x, y);
}

/* Updates the numeric frequency readout region */
void tuner_draw_frequency(float f){
	char buf[32];

	// Clear only the old value
	tuner_clear_freq_value();

	setFont(SmallFont);
	setColor(WHITE);  

	if (f < 1.0f) {
		// No meaningful pitch
		lcdPrint("---.-- Hz", FREQ_VAL_X, FREQ_VAL_Y);
		return;
	}

	// Format with 2 decimal places
	sprintf(buf, "%6.2f Hz", f);
	lcdPrint(buf, FREQ_VAL_X, FREQ_VAL_Y);
}

/* Draws the cents deviation indicator on the tuning bar, clearing the previous position */
void tuner_draw_cents(int cents){
    // clamp input
    if (cents < -50) cents = -50;
    if (cents >  50) cents =  50;

    // define a slightly inset interior region
    int inner_left = BAR_X1 + 2;
    int inner_right = BAR_X2 - 2;
    int inner_mid = (inner_left + inner_right) / 2;
    int span_half = inner_right - inner_mid;   // half span for +50c

    // map cents to an x position
    // -50c near inner_left, +50c near inner_right
    int x = inner_mid + (cents * span_half) / 50;

    // clear previous indicator (if any)
    if (prev_cents_x >= 0) {
        // clear with background color
        setColor(BLACK);
        fillRect(prev_cents_x - 1, BAR_Y1 + 2, prev_cents_x + 1, BAR_Y2 - 2);
    }

    // redraw center marker (0 cents)
    setColor(WHITE);
    fillRect(BAR_MID, BAR_Y1, BAR_MID + 1, BAR_Y2);

    // draw new cents indicator
    setColor(GREEN);
    fillRect(x - 1, BAR_Y1 + 2, x + 1, BAR_Y2 - 2);

    prev_cents_x = x;
}

/* Convenience wrapper that updates the frequency readout and cents indicator together */
void tuner_update_display(float freq_hz, int cents){
    // Draw the numeric frequency
    tuner_draw_frequency(freq_hz);

    // Draw the cents indicator
    tuner_draw_cents(cents);
}

/* Clears the note box to its background color */
void tuner_clear_note_box(void){
	setColor(40, 40, 40); // grey
	fillRect(NOTE_BOX_X1, NOTE_BOX_Y1, NOTE_BOX_X2, NOTE_BOX_Y2);
}

/* Clears the interior of the cents bar and resets the previous indicator state */
void tuner_clear_cents_bar(void){
	// Clear only the inside of the bar
	setColor(BLACK);  // screen background color

	// Shrink by 2 px on each side so we don't erase outline
	fillRect(BAR_X1 + 2, BAR_Y1 + 2, BAR_X2 - 2, BAR_Y2 - 2);

	prev_cents_x = -1;
}




/* ======================================================*/
/*                    Headers/Mode Screens               */
/* ======================================================*/

/* Draws the top header bar for main mode */
void tuner_draw_main_header(void) {
    // clear top bar
	setColor(BLACK);
    fillRect(0, 0, SCREEN_W - 1, 19);

    // draw text
    setFont(SmallFont);
    setColor(WHITE);
    lcdPrint("MAIN MODE", 5, 4);
}

/* Draws the top header bar for debug mode */
void tuner_draw_debug_header(void) {

    // clear top bar
	setColor(BLACK);
	fillRect(0, 0, SCREEN_W - 1, 19);

	// draw text
	setFont(SmallFont);
	setColor(YELLOW);
	lcdPrint("DEBUG MODE", 5, 4);
}

/* Draws the top header bar for calibration mode */
void tuner_draw_cal_header(void) {

    // clear top bar
	setColor(BLACK);
	fillRect(0, 0, SCREEN_W - 1, 19);

	// draw text
	setFont(SmallFont);
	setColor(CYAN);
	lcdPrint("CALIBRATION MODE", 5, 4);
}

/* Draws the calibration screen layout and triggers the A4 value update */
void tuner_draw_cal_screen(float baseAHz) {
    char buf[32];

    // clear everything below the header bar (y = 0–19)
    setColor(BLACK);
    fillRect(0, 20, SCREEN_W - 1, SCREEN_H - 1);

    // big title "CALIBRATION" in the upper-middle
    setFont(BigFont);
    setColor(CYAN);  // cyan

    const char *title = "CALIBRATION";
    int len = 0;
    while (title[len] != '\0') {
        len++;
    }

    int text_w = cfont.x_size * len;
    int text_h = cfont.y_size;

    int x = (SCREEN_W - text_w) / 2;
    int y = 60;   // somewhere above center

    lcdPrint((char *)title, x, y);

    // show current base tuning: "A4 = xxx Hz"
    setFont(SmallFont);
    setColor(WHITE);

    // hint line at bottom
    lcdPrint("Use encoder to adjust", 30, 200);

    // draw the current A4 reference value
	tuner_update_cal_ref(baseAHz);
}

/* Draws the debug mode 1 screen layout and initializes debug text */
void tuner_draw_debug_screen(void) {
	// Clear everything below the header bar
	setColor(BLACK);
	fillRect(0, 20, SCREEN_W - 1, SCREEN_H - 1);

	// Draw the debug header in the top bar
	tuner_draw_debug_header();

	// one static label above the live lines
	setFont(SmallFont);
	setColor(WHITE);
	lcdPrint("DEBUG MODE 1 (fft spectrum)", 10, 24);

	tuner_debug_update(440.0f, "--", 0, 0);
}

/* Draws the debug mode 2 screen layout */
void tuner_draw_debug2_screen(void) {
    // Clear everything below the header bar
    setColor(BLACK);
    fillRect(0, 20, SCREEN_W - 1, SCREEN_H - 1);

    // Re-use the same debug header at the top
    tuner_draw_debug_header();

    // Label for this page
    setFont(SmallFont);
    setColor(WHITE);
    lcdPrint("DEBUG MODE 2 (DSP info)", 10, 24);
}



/* ===== Updates the A4 reference value shown on the calibration screen ===== */
void tuner_update_cal_ref(float a4_hz) {
    char buf[32];

    tuner_clear_cal_ref_box();

    setFont(SmallFont);
    setColor(CYAN);
    sprintf(buf, "A4 = %3.0f Hz", a4_hz);
    lcdPrint(buf, CAL_A4_X, CAL_A4_Y);
}




/* ======================================================*/
/*                    Debug Live Updates                 */
/* ======================================================*/

/* Updates debug mode 1 text lines */
void tuner_debug_update(float freq_hz, const char* noteName, int octave, int cents){
	char buf[32];

	setFont(SmallFont);

	// Frequency line
	setColor(BLACK);  // clear with background
	tuner_debug_clear_line(DBG_FREQ_X, DBG_FREQ_Y);
	setColor(WHITE);   // now white for text
	if (freq_hz < 1.0f) {
		lcdPrint("F = ---.-- Hz", DBG_FREQ_X, DBG_FREQ_Y);
	} else {
		sprintf(buf, "F = %6.2f Hz", freq_hz);
		lcdPrint(buf, DBG_FREQ_X, DBG_FREQ_Y);
	}

	// Note line
	setColor(BLACK);
	tuner_debug_clear_line(DBG_NOTE_X, DBG_NOTE_Y);
	setColor(BLACK);
	if (noteName && noteName[0] != '-') {
		sprintf(buf, "Note = %s%d", noteName, octave);
	} else {
		sprintf(buf, "Note = --");
	}
	lcdPrint(buf, DBG_NOTE_X, DBG_NOTE_Y);

	// Cents line
	setColor(BLACK);
	tuner_debug_clear_line(DBG_CENTS_X, DBG_CENTS_Y);
	setColor(WHITE);
	sprintf(buf, "Cents = %+d", cents);
	lcdPrint(buf, DBG_CENTS_X, DBG_CENTS_Y);
}

/* Updates debug mode 2 text lines with DSP/FFT parameters and current readings */
void tuner_debug2_update(float freq_hz, int cents, float fs_eff_hz, float bin_hz, float peak_mag){
	char buf[32];

	setFont(SmallFont);

	// Line 1 (y=40): effective sample rate
	setColor(BLACK); // clear background
	tuner_debug_clear_line(10, 40);
	setColor(WHITE); // white text
	sprintf(buf, "Fs_eff = %5d Hz", (int)(fs_eff_hz + 0.5f));
	lcdPrint(buf, 10, 40);

	// Line 2 (y=60): bin spacing
	setColor(BLACK);
	tuner_debug_clear_line(10, 60);
	setColor(WHITE);
	sprintf(buf, "Bin Hz  = %4.1f", bin_hz);
	lcdPrint(buf, 10, 60);

	// Line 3 (y=80): FFT config
	setColor(BLACK);
	tuner_debug_clear_line(10, 80);
	setColor(WHITE);
	sprintf(buf, "N = %d  dec = %d", 512, 4);
	lcdPrint(buf, 10, 80);

	// Line 4 peak mag
	setColor(BLACK);
	tuner_debug_clear_line(10, 100);
	setColor(WHITE);
	sprintf(buf, "PkMag: %.3e", peak_mag);
	lcdPrint(buf, 10, 100);

	// Line 5 (y=100): current freq / cents
	setColor(BLACK);
	tuner_debug_clear_line(10, 120);
	setColor(WHITE);
	if (freq_hz < 1.0f) {
		lcdPrint("F = ---.-- Hz   C=0c", 10, 120);
	} else {
		sprintf(buf, "F=%4d Hz  C=%3dc", (int)(freq_hz + 0.5f), cents);
		lcdPrint(buf, 10, 120);
	}
}

/* Renders a bar-graph spectrum from magnitude bins in the debug spectrum region */
void tuner_debug_draw_spectrum(const float *mag, int n_bins){
	if (!mag || n_bins <= 0) {
		// just clear the area if something's off
		setColor(BLACK);
		fillRect(SPEC_X0, SPEC_Y0, SPEC_X1, SPEC_Y1);
		return;
	}

	// clear spectrum area
	setColor(BLACK);
	fillRect(SPEC_X0, SPEC_Y0, SPEC_X1, SPEC_Y1);

	// build a log-compressed magnitude array
	float max_v = 0.0f;
	static float vbuf[DEBUG_NBINS];

	int bins = n_bins;
	if (bins > DEBUG_NBINS) {
		bins = DEBUG_NBINS;
	}

	for (int i = 0; i < bins; ++i) {
		float m = mag[i];
		if (m < 0.0f) m = 0.0f;

		// log compression
		float v = logf(m + 1.0f); 
		vbuf[i] = v;
		if (v > max_v) {
			max_v = v;
		}
	}

	if (max_v <= 0.0f) {
		// nothing useful to draw
		return;
	}

	int width = SPEC_X1 - SPEC_X0 + 1;
	int height = SPEC_Y1 - SPEC_Y0 + 1;

	// fit all bins across the width
	int bar_spacing = (bins > 0) ? (width / bins) : 1;
	if (bar_spacing < 1) bar_spacing = 1;

	for (int i = 0; i < bins; ++i) {
		// normalize 0..1 from log-compressed values
		float norm = vbuf[i] / max_v;
		if (norm < 0.0f) norm = 0.0f;
		if (norm > 1.0f) norm = 1.0f;

		int bar_h = (int)(norm * (height - 2));
		if (bar_h <= 0) continue;

		int x0 = SPEC_X0 + i * bar_spacing;
		int x1 = x0 + bar_spacing - 1;
		if (x0 > SPEC_X1) break;
		if (x1 > SPEC_X1) x1 = SPEC_X1;

		int y1 = SPEC_Y1;
		int y0 = SPEC_Y1 - bar_h;

		setColor(GREEN);   // green bars
		fillRect(x0, y0, x1, y1);
	}
}




/* ======================================================*/
/*                    Private Helpers                    */
/* ======================================================*/

/* Clears only the frequency value region so it can be redrawn cleanly */
static void tuner_clear_freq_value(void){

	// black background to match
	setColor(BLACK);
	fillRect(FREQ_VAL_X, FREQ_VAL_Y, FREQ_VAL_X + FREQ_VAL_W, FREQ_VAL_Y + FREQ_VAL_H);
}

/* Clears the calibration A4 value region before redrawing it */
static void tuner_clear_cal_ref_box(void) {
    setColor(BLACK);  // black background
    fillRect(CAL_A4_X, CAL_A4_Y, CAL_A4_X + CAL_A4_W, CAL_A4_Y + CAL_A4_H);
}

/* Clears a single debug text line region at a given position */
static void tuner_debug_clear_line(int x, int y) {
    // clear a horizontal band for one debug line
    fillRect(x, y, x + DBG_LINE_W, y + DBG_LINE_H);
}

