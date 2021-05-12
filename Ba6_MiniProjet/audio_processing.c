#include "ch.h"
#include "hal.h"
#include <main.h>
#include <usbcfg.h>
#include <arm_math.h>
#include <arm_const_structs.h>
#include <audio/microphone.h>
#include <audio_processing.h>

#include <move.h>
#include <leds_animations.h>
#include <debug_messager.h>
#include <mode_selection.h>

// 2 times FFT_SIZE because these arrays contain complex numbers (real + imaginary)
static float micBack_cmplx_input[2 * FFT_SIZE];
// Arrays containing the computed magnitude of the complex numbers
static float micBack_output[FFT_SIZE];

#define MIN_VALUE_THRESHOLD		15000;	// value to take frequence into account
#define MIN_FREQ				10		// lowest frequency human voice can reach easily
#define MAX_FREQ				45		// highest frequency humain voice can reach easily
#define MIN_FREQ_INIT			17		// lowest frequency to start piloting
#define MAX_FREQ_INIT			30		// highest frequency to start piloting
#define FREQ_THRESHOLD_FORWARD	1		// threshold before rotating
#define NB_SOUND_ON				20		// nbr samples to get for the mean before running
#define NB_SOUND_OFF			15		// nbr sample to reset the mean
#define ROTATION_COEFF			12		// how strong the rotation is

static uint8_t mode = SOUND_OFF;		// SOUND_OFF, ANALYSING, MOVING
static uint8_t sound_on = 0;			// counter to get the mean
static uint8_t sound_off = 0;			// counter to reset the mean (human has to breath)

static bool sleep_mode = true;

/*
 * 	@Describe:
 * 		detecte the highest value in a buffer.
 * 		take the mean value then start moving.
 * 		If a frequency is higher, turn right, if lower, turn left.
 *
 * 	@Param:
 * 		float* data		an array containing the frequencies
 *
 */
void frequency_piloting(float* data) {
	int16_t error = 0;									// between detected and mean
	float max_norm = MIN_VALUE_THRESHOLD;				// the biggest norm value
	int16_t max_norm_index = -1;						// the biggest norm index
	int16_t max_norms_index[4] = { -1, -1, -1, -1 };    // the four biggest
	uint8_t norms_index = 0;							// to scan the data array

	static int16_t mean_freq = 0;

	// search the 4 biggest frequencies with a circular array
	for (uint16_t i = MIN_FREQ; i <= MAX_FREQ; i++) {
		if (data[i] > max_norm) {
			max_norm = data[i];
			max_norms_index[norms_index] = i;
			norms_index++;
			norms_index %= 4;
		}
	}

	// choose the smallest of the 4 to block higher "harmonics"
	if (max_norms_index[norms_index] == -1) {
		max_norm_index = max_norms_index[0];
	} else {
		max_norm_index = max_norms_index[norms_index];
	}

	// if no value superior to threshold
	if (max_norm_index == -1) {
		// wait a bit before resetting mean (humain breath)
		if (mode != SOUND_OFF) {
			sound_off++;
			anim_stop_freq_manual(sound_off, NB_SOUND_OFF);
			if (sound_off == NB_SOUND_OFF) {
				mean_freq = -1;
				sound_on = 0;
				mode = SOUND_OFF;
			}
		}
	}
	else {
		sound_off = 0;

		// if sound was off, start the mean acquiring process
		if (sound_on == 0) {
			if (max_norm_index > MIN_FREQ_INIT && max_norm_index < MAX_FREQ_INIT) {
				mean_freq = max_norm_index;
				mode = ANALYSING;
				anim_start_freq_manual(sound_on, NB_SOUND_ON);
				sound_on++;

				if (get_punky_state() == PUNKY_DEBUG)
					debug_message("Acquiring mean", READABLE, LOW_PRIO);

			}
		}
		// add samples for the mean
		else if (sound_on < NB_SOUND_ON) {
			mean_freq += max_norm_index;
			mode = ANALYSING;
			anim_start_freq_manual(sound_on, NB_SOUND_ON);
			sound_on++;
		}
		// enough samples, set the mean
		else if (sound_on == NB_SOUND_ON) {
			mean_freq += max_norm_index;
			mean_freq /= (NB_SOUND_ON + 1);

			anim_start_freq_manual(sound_on, NB_SOUND_ON);
			mode = ANALYSING;
			sound_on++;

			if (get_punky_state() == PUNKY_DEBUG)
				debug_message_1("Moyenne = ", mean_freq, EMPHASIS, HIGH_PRIO);
		}
		// a mean is set, start piloting phase
		else {
			mode = MOVING;
//			motor_control_start();
		}
	}

	// robot moves while there is sound
	if (mode == MOVING && sound_off == 0) {
		error = max_norm_index - mean_freq;

		if (get_punky_state() == PUNKY_DEBUG)
				debug_message_1("Freq variation = ", error, READABLE, LOW_PRIO);

		// if near the mean_freq, go straight, otherwise turn with quadratic function
		if (max_norm_index >= mean_freq - FREQ_THRESHOLD_FORWARD && max_norm_index <= mean_freq + FREQ_THRESHOLD_FORWARD) {
			set_rotation(0);
		} else {
			set_rotation(ROTATION_COEFF * error * error * (error < 0 ? -1 : 1));
		}
		move();

	}
	// robot stop. If mode = moving, mean is kept (to allow humain to breath)
	else if (mode == SOUND_OFF || (mode == MOVING && sound_off > 0)){
		move_stop();
	}
}

/*
 * 	@Describe:
 *		Callback called when the demodulation of the four microphones is done.
 *		We get 160 samples per mic every 10ms (16kHz), so we fill the samples
 *		buffers to reach 1024 samples, then we compute the FFTs.
 *
 *	@Params:
 *		int16_t *data			Buffer containing 4 times 160 samples. the samples are sorted by micro
 *								so we have [micRight1, micLeft1, micBack1, micFront1, micRight2, etc...]
 *		uint16_t num_samples	Tells how many data we get in total (should always be 640)
 *
 *	@Author:
 *		TP5_Noisy_correction, adapted for our needs
 */
void processAudioData(int16_t *data, uint16_t num_samples) {

	static uint16_t nb_samples = 0;

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

			// FFT processing
			arm_cfft_f32(&arm_cfft_sR_f32_len1024, micBack_cmplx_input, 0, 1);
			// Magnitude processing
			arm_cmplx_mag_f32(micBack_cmplx_input, micBack_output, FFT_SIZE);

			nb_samples = 0;

			frequency_piloting(micBack_output);
		}
	}else{
		// reset sample otherwise problem when restarting the function
		nb_samples = 0;
	}
}

/*
 * 	@Describe:
 * 		allow the sound thread to execute it's code
 */
void microphone_start(void) {
	sleep_mode = false;
}

/*
 * 	@Describe:
 * 		prevent the sound thread to execute it's code
 * 		--> ressources and time gain for the other threads
 */
void microphone_stop(void) {
	if(mode != SOUND_OFF && get_punky_state() != PUNKY_SLEEP){
		anim_stop_freq();
	}
	sleep_mode = true;
	mode = SOUND_OFF;
	sound_on = 0;
	sound_off = 0;
}

