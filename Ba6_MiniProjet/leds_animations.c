#include "ch.h"
#include "hal.h"
#include "leds.h"
#include <math.h>
#include <usbcfg.h>

#include <leds_animations.h>

//semaphore
static BSEMAPHORE_DECL(anim_ready, TRUE); // @suppress("Field cannot be resolved")

static uint8_t animation = ANIM_CLEAR;	// animation to be run
static uint8_t freq_led_intensity = 0;	// link for intensity depending of external variable
static uint8_t direction = 0;			// forward or backward, for direction indication

/*
 * 	@Describe:
 * 		Nice animations to help the user and indicate actions.
 * 		The animations SHOULD always be called with one time call.
 * 		If an animation is called on a repeated basis, it may cause some trouble
 * 		and animation could be ignored as there is no priority rules
 * 		Animations are started by a semaphore call
 *
 * 		=> See animation call function for description
 *
 */
static THD_WORKING_AREA(waLedAnimationThd, 128);
static THD_FUNCTION(LedAnimationThd, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void) arg;

	while (1) {

		//wait for an animation to be launched
		chBSemWait(&anim_ready);

		switch (animation) {
		case ANIM_BARCODE:

			set_body_led(1);

			//2 RGB leds show the direction of the barcode
			//turning green with a fade in and out
			if (direction == ANIM_FORWARD) {
				for (uint8_t i = 0; i < FULL_I; i++) {
					set_rgb_led(LED2, 0, i, 0);
					set_rgb_led(LED8, 0, i, 0);
					chThdSleepMilliseconds(5);
				}
				for (uint8_t i = FULL_I; i > 0; i--) {
					set_rgb_led(LED2, 0, i, 0);
					set_rgb_led(LED8, 0, i, 0);
					chThdSleepMilliseconds(10);
				}
				set_rgb_led(LED2, 0, 0, 0);
				set_rgb_led(LED8, 0, 0, 0);
			} else {
				for (uint8_t i = 0; i < FULL_I; i++) {
					set_rgb_led(LED4, 0, i, 0);
					set_rgb_led(LED6, 0, i, 0);
					chThdSleepMilliseconds(5);
				}
				for (uint8_t i = FULL_I; i > 0; i--) {
					set_rgb_led(LED4, 0, i, 0);
					set_rgb_led(LED6, 0, i, 0);
					chThdSleepMilliseconds(10);
				}
				anim_clear_rgbs();
			}
			set_body_led(0);

			break;

		case ANIM_DEBUG:
			//red counterclockwise animation with 8 leds
			for (uint8_t i = 0; i < 2; i++) {
				set_led(LED1, 1);
				set_led(LED5, 1);
				chThdSleepMilliseconds(TIME_DEBUG);
				set_led(LED1, 0);
				set_led(LED5, 0);
				set_rgb_led(LED8, HALF_I, 0, 0);
				set_rgb_led(LED4, HALF_I, 0, 0);
				chThdSleepMilliseconds(TIME_DEBUG);
				set_rgb_led(LED8, 0, 0, 0);
				set_rgb_led(LED4, 0, 0, 0);
				set_led(LED7, 1);
				set_led(LED3, 1);
				chThdSleepMilliseconds(TIME_DEBUG);
				set_led(LED7, 0);
				set_led(LED3, 0);
				set_rgb_led(LED6, HALF_I, 0, 0);
				set_rgb_led(LED2, HALF_I, 0, 0);
				chThdSleepMilliseconds(TIME_DEBUG);
				set_rgb_led(LED6, 0, 0, 0);
				set_rgb_led(LED2, 0, 0, 0);
			}
			set_led(LED1, 1);
			set_led(LED5, 1);
			break;

		case ANIM_CLEAR_DEBUG:
			// red clockwise animation with 8 leds
			set_led(LED1, 0);
			set_rgb_led(LED2, HALF_I, 0, 0);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_rgb_led(LED2, 0, 0, 0);
			set_led(LED3, 1);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_led(LED3, 0);
			set_rgb_led(LED4, HALF_I, 0, 0);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_rgb_led(LED4, 0, 0, 0);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_led(LED5, 0);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_rgb_led(LED6, HALF_I, 0, 0);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_rgb_led(LED6, 0, 0, 0);
			set_led(LED7, 1);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_led(LED7, 0);
			set_rgb_led(LED8, HALF_I, 0, 0);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_rgb_led(LED8, 0, 0, 0);
			set_led(LED1, 1);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_led(LED1, 0);
			break;

		case ANIM_SLEEP:
			//yellow fade in with RGB leds
			for (uint8_t i = 0; i < HALF_I; i++) {
				set_rgb_led(LED2, i, i, 0);
				set_rgb_led(LED4, i, i, 0);
				set_rgb_led(LED6, i, i, 0);
				set_rgb_led(LED8, i, i, 0);
				chThdSleepMilliseconds(10);
			}
			//turning to red with RGB leds
			for (uint8_t i = 0; i < HALF_I; i++) {
				set_rgb_led(LED2, HALF_I + i, HALF_I - i, 0);
				set_rgb_led(LED4, HALF_I + i, HALF_I - i, 0);
				set_rgb_led(LED6, HALF_I + i, HALF_I - i, 0);
				set_rgb_led(LED8, HALF_I + i, HALF_I - i, 0);
				chThdSleepMilliseconds(10);
			}
			break;

		case ANIM_WAKE_UP:
			//fade out turning RGB leds off
			for (uint8_t i = 0; i < FULL_I; i += 2) {
				set_rgb_led(LED2, FULL_I - i, 0, 0);
				set_rgb_led(LED4, FULL_I - i, 0, 0);
				set_rgb_led(LED6, FULL_I - i, 0, 0);
				set_rgb_led(LED8, FULL_I - i, 0, 0);
				chThdSleepMilliseconds(10);
			}
			anim_clear_rgbs();
			break;

		case ANIM_FREQ:
			//turn off the blue leds
			for (uint8_t i = freq_led_intensity; i > 0; i--) {

				freq_led_intensity--;

				set_rgb_led(LED2, 0, freq_led_intensity, freq_led_intensity);
				set_rgb_led(LED4, 0, freq_led_intensity, freq_led_intensity);
				set_rgb_led(LED6, 0, freq_led_intensity, freq_led_intensity);
				set_rgb_led(LED8, 0, freq_led_intensity, freq_led_intensity);
				chThdSleepMilliseconds(6);
			}
			anim_clear_rgbs();
			break;

		case ANIM_FREQ_MANUAL:
			//blue fade in during mean acquisition
			set_rgb_led(LED2, 0, freq_led_intensity, freq_led_intensity);
			set_rgb_led(LED4, 0, freq_led_intensity, freq_led_intensity);
			set_rgb_led(LED6, 0, freq_led_intensity, freq_led_intensity);
			set_rgb_led(LED8, 0, freq_led_intensity, freq_led_intensity);
			break;

		default:
			//turn off every leds
			set_rgb_led(LED2, 0, 0, 0);
			set_rgb_led(LED4, 0, 0, 0);
			set_rgb_led(LED6, 0, 0, 0);
			set_rgb_led(LED8, 0, 0, 0);
			set_led(LED1, 0);
			set_led(LED3, 0);
			set_led(LED5, 0);
			set_led(LED7, 0);
			break;
		}

		chThdSleepMilliseconds(50);
	}
}

/*
 * 	@Describe:
 * 		Color : green
 * 		Turn on the body led for 0.8 second
 * 		Indicate speed direction with a sweep up to 100%,
 * 		then back off.
 */
void anim_barcode(uint8_t direction_p) {
	direction = direction_p;
	animation = ANIM_BARCODE;
	chBSemSignal(&anim_ready);
}

/*
 * 	@Describe:
 * 		Color : blue & green
 * 		Accompany the user with the mean sampling.
 * 		The intensity reflects the percentage of the process.
 * 		When mean is sampled, brightness is maximal at FREQ_I intensity.
 */
void anim_start_freq_manual(uint8_t step, uint8_t nb_steps) {
	animation = ANIM_FREQ_MANUAL;

	freq_led_intensity = HALF_I	 * step / nb_steps;

	chBSemSignal(&anim_ready);
}

/*
 * 	@Describe:
 * 		Color : blue & green
 * 		Indicate the user the mean reset.
 * 		The intensity reflects the percentage of the process.
 * 		When mean is lost, brightness is zero.
 */
void anim_stop_freq_manual(uint8_t step, uint8_t nb_steps) {
	animation = ANIM_FREQ_MANUAL;

	freq_led_intensity = HALF_I - HALF_I * step / nb_steps;

	chBSemSignal(&anim_ready);
}

/*
 * 	@Describe:
 * 		Color : blue & green
 * 		Shutdown the frequency leds automatically with a sweep if
 * 		frequency mode is forced to stop, i.e. when a barcode is
 * 		shown while moving.
 */
void anim_stop_freq(void) {
	if (freq_led_intensity != 0) {
		animation = ANIM_FREQ;
		chBSemSignal(&anim_ready);
	}
}

/*
 * 	@Describe:
 * 		Color : yellow & red
 * 		Sweep to yellow-red up to 50% intensity then up to 100%
 * 		full red. Stay on.
 */
void anim_sleep(void) {
	animation = ANIM_SLEEP;
	chBSemSignal(&anim_ready);
}

/*
 * 	@Describe:
 * 		Color : red
 * 		Switch off the leds with a sweep.
 */
void anim_wake_up(void) {
	animation = ANIM_WAKE_UP;
	chBSemSignal(&anim_ready);
}

/*
 * 	@Describe:
 * 		Color : red
 * 		Two points 180° opposite, rotating full led circle twice.
 * 		Stay on.
 */
void anim_debug(void) {
	animation = ANIM_DEBUG;
	chBSemSignal(&anim_ready);
}

/*
 * 	@Describe:
 * 		Color : red
 * 		A point turns full circle once then turn off.
 */
void anim_clear_debug(void) {
	animation = ANIM_CLEAR_DEBUG;
	chBSemSignal(&anim_ready);
}

/*
 * 	@Describe:
 * 		Color : none
 * 		Turn off all leds.
 */
void anim_clear_rgbs(void) {
	set_rgb_led(LED2, 0, 0, 0);
	set_rgb_led(LED4, 0, 0, 0);
	set_rgb_led(LED6, 0, 0, 0);
	set_rgb_led(LED8, 0, 0, 0);
}

void leds_animations_thd_start(void) {
	chThdCreateStatic(waLedAnimationThd, sizeof(waLedAnimationThd), NORMALPRIO - 1, LedAnimationThd, NULL);
}

