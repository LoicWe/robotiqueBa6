#include "ch.h"
#include "hal.h"
#include <main.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <move.h>
#include <audio/microphone.h>
#include <audio_processing.h>
#include <fft.h>					// A NETTOYER LA FONCTION NON OPTI NAN ?
#include <arm_math.h>
#include <communications.h>			// POSSIBLEMENT A ENLEVER
#include <leds.h>

//semaphore
static BSEMAPHORE_DECL(sendToComputer_sem, TRUE); // @suppress("Field cannot be resolved")

//2 times FFT_SIZE because these arrays contain complex numbers (real + imaginary)
static float micBack_cmplx_input[2 * FFT_SIZE];
//Arrays containing the computed magnitude of the complex numbers
static float micBack_output[FFT_SIZE];

#define MIN_VALUE_THRESHOLD	15000
#define MIN_FREQ		10	//we don't analyze before this index to not use resources for nothing
#define MAX_FREQ		40	//we don't analyze after this index to not use resources for nothing
#define FREQ_THRESHOLD	1
#define NB_SOUND_ON		10	//nbr samples to get the mean
#define NB_SOUND_OFF	10	//nbr sample to reset the mean
#define ROTATION_COEFF	40

static bool sleep_mode = true;

/*
 *	Simple function used to detect the highest value in a buffer
 *	and to execute a motor command depending on it
 */
void sound_remote(float* data) {
	static uint8_t sound_on = 0;
	static uint8_t sound_off = 0;
	int16_t error = 0;
	static uint8_t mode = SOUND_OFF;				//To change
	float max_norm = MIN_VALUE_THRESHOLD;
	int16_t max_norm_index = -1;
	static int16_t mean_freq = 0;

	//search for the highest peak
	for (uint16_t i = MIN_FREQ; i <= MAX_FREQ; i++) {
		if (data[i] > max_norm) {
			max_norm = data[i];
			max_norm_index = i;
		}
	}
	//determine if there was a value superior of the threshold
	if (max_norm_index == -1) {
		sound_off++;
		if (sound_off == NB_SOUND_OFF) {
			mean_freq = -1;
			sound_on = 0;
		}
		mode = SOUND_OFF;
	} else {
		sound_off = 0;
		if (sound_on == 0) {
			mean_freq = max_norm_index;
			mode = ANALYSING;
			sound_on++;
		} else if (sound_on < NB_SOUND_ON) {
			mean_freq += max_norm_index;
			mode = ANALYSING;
			sound_on++;
		} else if (sound_on == NB_SOUND_ON) {
			mean_freq += max_norm_index;
			mean_freq /= (NB_SOUND_ON + 1);
			mode = ANALYSING;
			sound_on++;
		} else {
			mode = MOVING;
		}
	}

	if (mode == MOVING) {
		error = max_norm_index - mean_freq;

		//go forward
		if (max_norm_index >= mean_freq - FREQ_THRESHOLD && max_norm_index <= mean_freq + FREQ_THRESHOLD) {
			move(0);
		} else {
			move(ROTATION_COEFF * error);
		}

	} else {
		move_stop();
	}
}

/*
 *	Callback called when the demodulation of the four microphones is done.
 *	We get 160 samples per mic every 10ms (16kHz)
 *
 *	params :
 *	int16_t *data			Buffer containing 4 times 160 samples. the samples are sorted by micro
 *							so we have [micRight1, micLeft1, micBack1, micFront1, micRight2, etc...]
 *	uint16_t num_samples	Tells how many data we get in total (should always be 640)
 */
void processAudioData(int16_t *data, uint16_t num_samples) {

	/*
	 *
	 *	We get 160 samples per mic every 10ms
	 *	So we fill the samples buffers to reach
	 *	1024 samples, then we compute the FFTs.
	 *
	 */

	static uint16_t nb_samples = 0;
	static uint8_t mustSend = 0;

	if (!sleep_mode) {

		//loop to fill the buffers
		for (uint16_t i = 0; i < num_samples; i += 4) {
			//construct an array of complex numbers. Put 0 to the imaginary part
			micBack_cmplx_input[nb_samples] = (float) data[i + MIC_BACK];
			nb_samples++;
			micBack_cmplx_input[nb_samples++] = 0;					// to be tested

			//stop when buffer is full
			if (nb_samples >= (2 * FFT_SIZE)) {
				break;
			}
		}

		if (nb_samples >= (2 * FFT_SIZE)) {
			/*	FFT processing
			 *
			 *	This FFT function stores the results in the input buffer given.
			 *	This is an "In Place" function.
			 */

			doFFT_optimized(FFT_SIZE, micBack_cmplx_input);

			/*	Magnitude processing
			 *
			 *	Computes the magnitude of the complex numbers and
			 *	stores them in a buffer of FFT_SIZE because it only contains
			 *	real numbers.
			 *
			 */
			arm_cmplx_mag_f32(micBack_cmplx_input, micBack_output, FFT_SIZE);

			//sends only one FFT result over 10 for 1 mic to not flood the computer
			//sends to UART3
			if (mustSend > 8) {
				//signals to send the result to the computer
				chBSemSignal(&sendToComputer_sem);
				mustSend = 0;
			}
			nb_samples = 0;
			mustSend++;

			sound_remote(micBack_output);
		}
	}
}

void wait_send_to_computer(void) {
	chBSemWait(&sendToComputer_sem);
}

float* get_audio_buffer_ptr(void) {
	return micBack_output;
}

void microphone_start(void) {
	sleep_mode = false;
}

void microphone_stop(void) {
	sleep_mode = true;
}
