#include "fft.h"
#include "complex.h"
#include "trig.h"

#define MAX_M 9

#define F_MIN  80.0f    // E2 ~ 82.4 Hz
#define F_MAX 4200.0f   // C7 ~ 4186 Hz


static float new_[512];
static float new_im[512];

// Precomputed twiddle factors: [stage][k]
static float W_real_stage[MAX_M][256];
static float W_imag_stage[MAX_M][256];

// peak magnitue from last FFT call
static float last_peak_mag = 0.0f;



/* pre-computation for fft */
void fft_init(int n, int m){
	int j, k;
	int b;

	// Precompute twiddles for each stage j = 0..m-1
	b = 1;
	for (j = 0; j < m; j++) {
		for (k = 0; k < b; k++) {
			float angle = -PI * k / b;
			W_real_stage[j][k] = cosine(angle);
			W_imag_stage[j][k] = sine(angle);
		}
		b *= 2;
	}
}


/* FFT Algorithm */
float fft(float* q, float* w, int n, int m, float sample_f) {
	int a,b,r,d,e,c;
	int k,place;
	a=n/2;
	b=1;
	int i,j;
	float real=0,imagine=0;
	float max,frequency;

	// Ordering algorithm
	for(i=0; i<(m-1); i++){
		d=0;
		for (j=0; j<b; j++){
			for (c=0; c<a; c++){	
				e=c+d;
				new_[e]=q[(c*2)+d];
				new_im[e]=w[(c*2)+d];
				new_[e+a]=q[2*c+1+d];
				new_im[e+a]=w[2*c+1+d];
			}
			d+=(n/b);
		}		
		for (r=0; r<n;r++){
			q[r]=new_[r];
			w[r]=new_im[r];
		}
		b*=2;
		a=n/(2*b);
	}
	//end ordering algorithm



    // FFT stages: use precomputed twiddles + butterflies + reorder
    b = 1;
    for (j = 0; j < m; j++) {

        int blockSize = n / b;

        for (k = 0; k < b; k++) {

            int start = k * blockSize;
            int end = start + blockSize;

            float Wr = W_real_stage[j][k];
            float Wi = W_imag_stage[j][k];

            for (i = start; i < end; i += 2) {

                // input sample: (a + j*b_im)
                float a = q[i + 1];
                float b_im = w[i + 1];

                // (a + j*b_im) * (Wr + j*Wi)
                float real = a * Wr - b_im * Wi;
                float imagine = a * Wi + b_im * Wr;

                new_[i] = q[i] + real;
                new_im[i] = w[i] + imagine;
                new_[i + 1] = q[i] - real;
                new_im[i+1] = w[i] - imagine;
            }
        }

        // Copy back
        for (i = 0; i < n; i++) {
            q[i] = new_[i];
            w[i] = new_im[i];
        }

        // Reorder (same as before)
        for (i = 0; i < n / 2; i++) {
            new_[i] = q[2 * i];
            new_[i + (n / 2)]  = q[2 * i + 1];
            new_im[i] = w[2 * i];
            new_im[i + (n/2)] = w[2 * i + 1];
        }
        for (i = 0; i < n; i++) {
            q[i] = new_[i];
            w[i] = new_im[i];
        }

        b *= 2;
    }

    // bin spacing in Hz for this FFT call
    float bin_spacing = sample_f / (float)n;

    // convert desired freq range to bin indices
    int start_bin = (int)(F_MIN / bin_spacing + 0.5f);
    int end_bin = (int)(F_MAX / bin_spacing + 0.5f);

    if (start_bin < 1) start_bin = 1;        // skip DC
    if (end_bin > (n/2 - 1)) end_bin   = (n/2 - 1);

    max = 0.0f;
    place = start_bin;

    // Peak magnitude
    for (i = start_bin; i <= end_bin; ++i) {
        float re = q[i];
        float im = w[i];
        float mag2 = re*re + im*im;
        new_[i] = mag2;    

        if (mag2 > max) {
            max = mag2;
            place = i;
        }
    }

    // store peak magnitude
    last_peak_mag = max;

    // if no significant energy, exit
    if (max <= 0.0f) {
        return 0.0f;
    }

    // bin spacing in Hz
    bin_spacing = sample_f / (float)n;

    // make sure we have neighbors
    if (place <= start_bin || place >= end_bin) {
        // use bin center if no neighbor
        return bin_spacing * (float)place;
    }

    // 3-point parabolic interpolation around the peak
    float y1 = new_[place - 1];
    float y2 = new_[place];
    float y3 = new_[place + 1];

    float denom = (y1 - 2.0f * y2 + y3);
    float delta = 0.0f;  // fractional bin offset

    if (denom != 0.0f) {
        delta = 0.5f * (y1 - y3) / denom;   // typically in [-1, +1]

        // clamp just in case noise makes it crazy
        if (delta < -1.0f) delta = -1.0f;
        if (delta >  1.0f) delta =  1.0f;
    }

    // final frequency estimate (bin index + fractional offset)
    frequency = ( (float)place + delta ) * bin_spacing;

    return frequency;
}


/* Getter */
float fft_get_last_peak_mag(void){
	return last_peak_mag;
}


