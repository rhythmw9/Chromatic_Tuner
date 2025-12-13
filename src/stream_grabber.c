/*
 * stream_grabber.c
 *
 *  Created on: Nov 10, 2017
 *      Author: davidmc
 */


/*
 * The stream grabber is a hardware module that listens to the digital audio signal (from the ADC). It's a "burst recorder."
 * After being triggered, the SG records the next N samples into a buffer, stops, and waits for the CPU to read them.
 */


#include <stdint.h>
#include "xparameters.h"
#include "stream_grabber.h"

/* Necessary registers for the stream grabber hardware block */

// write (anything) to trigger a recording (write only)
static volatile uint32_t* const reg_start = (uint32_t*)(XPAR_STREAM_GRABBER_0_BASEADDR);

// read the captured samples (read only)
static volatile uint32_t* const reg_samples_captured = (uint32_t*)(XPAR_STREAM_GRABBER_0_BASEADDR);

// write to specify a sample index
static volatile uint32_t* const reg_readout_addr = (uint32_t*)(XPAR_STREAM_GRABBER_0_BASEADDR+4);

// read the value of the sample at the specified index
static volatile int32_t* const reg_readout_value = (int32_t*)(XPAR_STREAM_GRABBER_0_BASEADDR+8);

// read for timing/debugging
static volatile uint32_t* const reg_seq_counter = (uint32_t*)(XPAR_STREAM_GRABBER_0_BASEADDR+12);

// read the sequence counter at some hardware event
static volatile uint32_t* const reg_seq_counter_latched = (uint32_t*)(XPAR_STREAM_GRABBER_0_BASEADDR+16);



/* Functions */

// call to let the stream grabber start the recording
void stream_grabber_start(){
	*reg_start = 0; //Value doesn't matter, the act of writing does the magic
}

// returns how many samples have been captured so far in the current capture
unsigned stream_grabber_samples_sampled_captures() {
	return *reg_samples_captured;
}

// this is effectively a blocking wait
void stream_grabber_wait_enough_samples(unsigned required_samples){
	while((*reg_samples_captured)<required_samples) {}
}

// return the value of a specific sample
int stream_grabber_read_sample(unsigned which_sample)
{
	*reg_readout_addr = which_sample;
	return *reg_readout_value;
}

// return internal value of sequence counter
unsigned stream_grabber_read_seq_counter() {
	return *reg_seq_counter;
}


// return a latched snapshot of that counter (at a hardware event)
unsigned stream_grabber_read_seq_counter_latched() {
	return *reg_seq_counter_latched;
}
