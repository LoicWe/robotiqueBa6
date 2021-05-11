#include "ch.h"
#include "hal.h"
#include <main.h>
#include <math.h>
#include <usbcfg.h>
#include <arm_math.h>
#include <arm_const_structs.h>

#include <move.h>
#include <audio/microphone.h>
#include <audio_processing.h>
#include <arm_math.h>
#include <communications.h>			// POSSIBLEMENT A ENLEVER
#include <led_animation.h>
#include <potentiometer.h>
#include <debug_messager.h>
#include <chprintf.h>

//semaphore
static BSEMAPHORE_DECL(sendToComputer_sem, TRUE); // @suppress("Field cannot be resolved")

//2 times FFT_SIZE because these arrays contain complex numbers (real + imaginary)
static float micBack_cmplx_input[2 * FFT_SIZE];
//Arrays containing the computed magnitude of the complex numbers
static float micBack_output[FFT_SIZE];

#define MIN_VALUE_THRESHOLD		15000;
#define MIN_FREQ				10	// plus basse fréquence humainement atteignable "facilement"
#define MAX_FREQ				45	// plus haute fréquence humainement atteignable "facilement"
#define MIN_FREQ_INIT			17	// fréquence minimal pour calculer la moyenne
#define MAX_FREQ_INIT			30	// fréquence maximal pour calculer la moyenne
#define FREQ_THRESHOLD_FORWARD	1
#define FREQ_THRESHOLD_SLOW 	5
#define NB_SOUND_ON				20	//nbr samples to get the mean before running
#define NB_SOUND_OFF			10	//nbr sample to reset the mean
#define ROTATION_COEFF_SLOW		12

static bool sleep_mode = true;

static uint8_t mode = SOUND_OFF;
static uint8_t sound_on = 0;
static uint8_t sound_off = 0;

/*
 *	Simple function used to detect the highest value in a buffer
 *	and to execute a motor command depending on it
 */
void sound_remote(float* data) {
	int16_t error = 0;
	float max_norm = MIN_VALUE_THRESHOLD;
	int16_t max_norm_index = -1;
	int16_t max_norms_index[4] = { -1, -1, -1, -1 };
	uint8_t norms_index = 0;


	static int16_t mean_freq = 0;

	//search the 4 biggest frequencies with a circular array
	for (uint16_t i = MIN_FREQ; i <= MAX_FREQ; i++) {
		if (data[i] > max_norm) {
			max_norm = data[i];
			max_norms_index[norms_index] = i;
			norms_index++;
			norms_index %= 4;
		}
	}

	/* choose the smallest of the 4
	 * it's the one we want --> tested exprimentaly
	 * with low and high frequencies. It's because low frequencies
	 * have a lot of higher harmonic
	 */
	if (max_norms_index[norms_index] == -1) {
		max_norm_index = max_norms_index[0];
	} else {
		max_norm_index = max_norms_index[norms_index];
	}

	//determine if there was a value superior of the threshold
	if (max_norm_index == -1) {
		if (mode != SOUND_OFF) {
			sound_off++;
			anim_stop_freq_manual(sound_off);
			if (sound_off == NB_SOUND_OFF) {
				mean_freq = -1;
				sound_on = 0;
				mode = SOUND_OFF;
			}
		}
	}
	else {
		sound_off = 0;
		if (sound_on == 0) {
			if (max_norm_index > MIN_FREQ_INIT && max_norm_index < MAX_FREQ_INIT) {
				mean_freq = max_norm_index;
				mode = ANALYSING;
				anim_start_freq_manual(sound_on);
				sound_on++;
				if (get_punky_state() == PUNKY_DEBUG)
					debug_message("Acquiring mean", READABLE, LOW_PRIO);
			}
		} else if (sound_on < NB_SOUND_ON) {
			mean_freq += max_norm_index;
			mode = ANALYSING;
			anim_start_freq_manual(sound_on);
			sound_on++;
		} else if (sound_on == NB_SOUND_ON) {
			// mean is found
			mean_freq += max_norm_index;
			mean_freq /= (NB_SOUND_ON + 1);
			if (get_punky_state() == PUNKY_DEBUG)
				debug_message_1("Moyenne = ", mean_freq, EMPHASIS, HIGH_PRIO);
			anim_start_freq_manual(sound_on);
			mode = ANALYSING;
			sound_on++;
		} else {
			mode = MOVING;
			motor_control_run();
		}
	}

	if (mode == MOVING && sound_off == 0) {
		error = max_norm_index - mean_freq;

		if (get_punky_state() == PUNKY_DEBUG)
				debug_message_1("Freq variation = ", error, READABLE, LOW_PRIO);

		// if near the mean_freq, go straight
		if (max_norm_index >= mean_freq - FREQ_THRESHOLD_FORWARD && max_norm_index <= mean_freq + FREQ_THRESHOLD_FORWARD) {
			set_rotation(0);
		} else {
			set_rotation(ROTATION_COEFF_SLOW * error * error * (error < 0 ? -1 : 1));
		}
		move();

	} else if (mode == SOUND_OFF || (mode == MOVING && sound_off > 0)){
		move_stop();
	}
}

/*
 * 	@Describe:
 *		Callback called when the demodulation of the four microphones is done.
 *		We get 160 samples per mic every 10ms (16kHz)
 *
 *	@Params:
 *		int16_t *data			Buffer containing 4 times 160 samples. the samples are sorted by micro
 *								so we have [micRight1, micLeft1, micBack1, micFront1, micRight2, etc...]
 *		uint16_t num_samples	Tells how many data we get in total (should always be 640)
 *
 *	@Author:
 *		TP5, adapted for our needs
 */
void processAudioData(int16_t *data, uint16_t num_samples) {

	/*
	 *	We get 160 samples per mic every 10ms
	 *	So we fill the samples buffers to reach
	 *	1024 samples, then we compute the FFTs.
	 */

	static uint16_t nb_samples = 0;
	static uint8_t mustSend = 0;

	if (!sleep_mode) {

		//loop to fill the buffers
		for (uint16_t i = 0; i < num_samples; i += 4) {
			//construct an array of complex numbers. Put 0 to the imaginary part
			micBack_cmplx_input[nb_samples] = (float) data[i + MIC_BACK];
			nb_samples++;
			micBack_cmplx_input[nb_samples++] = 0;

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
			arm_cfft_f32(&arm_cfft_sR_f32_len1024, micBack_cmplx_input, 0, 1);

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
			if (get_punky_state() == PUNKY_DEBUG && mustSend > 8) {
				//signals to send the result to the computer
				chBSemSignal(&sendToComputer_sem);
				mustSend = 0;
			}
			nb_samples = 0;
			mustSend++;

			sound_remote(micBack_output);

		}
	}else{
		nb_samples = 0;
	}
}

void wait_send_to_computer(void) {
	chBSemWait(&sendToComputer_sem);
}

float* get_audio_buffer_ptr(void) {
	return micBack_output;
}

void microphone_run(void) {
	sleep_mode = false;
}

bool get_microphone_state(void){
	return sleep_mode;
}

void microphone_stop(void) {
	if(mode != SOUND_OFF && get_punky_state() != PUNKY_SLEEP){
		anim_stop_freq();
	}
	sleep_mode = true;
	mode = SOUND_OFF;
	sound_on = 0;
	sound_off = 0;
}
