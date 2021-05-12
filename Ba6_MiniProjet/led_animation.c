#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>

#include "leds.h"
#include <led_animation.h>

//semaphore
static BSEMAPHORE_DECL(anim_ready, TRUE); // @suppress("Field cannot be resolved")

static uint8_t animation = ANIM_CLEAR;
static uint8_t freq_led_intensity = 0;	// link for intensity depending of extern variable
static uint8_t direction = 0;

/*
 * 	@Describe:
 * 		The animations SHOULD always be called with one time call.
 * 		If an animation is called on a repeated basis, it may cause some trouble
 * 		and animation could be ignored as there is no priority rules
 */
static THD_WORKING_AREA(waLedAnimationThd, 128);
static THD_FUNCTION(LedAnimationThd, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void) arg;

	while (1) {

		//wait for an animation to be launched
		chBSemWait(&anim_ready);

		switch (animation) {
//		case ANIM_CLEAR_DEBUG:
//			set_led(LED1, 0);
//			set_led(LED3, 0);
//			set_led(LED5, 0);
//			set_led(LED7, 0);
//			break;

		case ANIM_BARCODE:
			// turn on the body led for 0.8 second and the direction leds

			set_body_led(1);

			if (direction == ANIM_FORWARD) {
				for (uint8_t i = 0; i < 100; i++) {
					set_rgb_led(LED2, 0, i, 0);
					set_rgb_led(LED8, 0, i, 0);
					chThdSleepMilliseconds(5);
				}
				for (uint8_t i = 100; i > 0; i--) {
					set_rgb_led(LED2, 0, i, 0);
					set_rgb_led(LED8, 0, i, 0);
					chThdSleepMilliseconds(10);
				}
				set_rgb_led(LED2, 0, 0, 0);
				set_rgb_led(LED8, 0, 0, 0);
			} else {
				for (uint8_t i = 0; i < 100; i++) {
					set_rgb_led(LED4, 0, i, 0);
					set_rgb_led(LED6, 0, i, 0);
					chThdSleepMilliseconds(5);
				}
				for (uint8_t i = 100; i > 0; i--) {
					set_rgb_led(LED4, 0, i, 0);
					set_rgb_led(LED6, 0, i, 0);
					chThdSleepMilliseconds(10);
				}
				anim_clear_rgbs();
			}
			set_body_led(0);

			break;

		case ANIM_DEBUG:
			for (uint8_t i = 0; i < 2; i++) {
				set_led(LED1, 1);
				set_led(LED5, 1);
				chThdSleepMilliseconds(TIME_DEBUG);
				set_led(LED1, 0);
				set_led(LED5, 0);
				set_rgb_led(LED8, 60, 0, 0);
				set_rgb_led(LED4, 60, 0, 0);
				chThdSleepMilliseconds(TIME_DEBUG);
				set_rgb_led(LED8, 0, 0, 0);
				set_rgb_led(LED4, 0, 0, 0);
				set_led(LED7, 1);
				set_led(LED3, 1);
				chThdSleepMilliseconds(TIME_DEBUG);
				set_led(LED7, 0);
				set_led(LED3, 0);
				set_rgb_led(LED6, 60, 0, 0);
				set_rgb_led(LED2, 60, 0, 0);
				chThdSleepMilliseconds(TIME_DEBUG);
				set_rgb_led(LED6, 0, 0, 0);
				set_rgb_led(LED2, 0, 0, 0);
			}
			set_led(LED1, 1);
			set_led(LED5, 1);
			break;

		case ANIM_CLEAR_DEBUG:
			set_led(LED1, 0);
			set_rgb_led(LED2, 60, 0, 0);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_rgb_led(LED2, 0, 0, 0);
			set_led(LED3, 1);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_led(LED3, 0);
			set_rgb_led(LED4, 60, 0, 0);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_rgb_led(LED4, 0, 0, 0);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_led(LED5, 0);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_rgb_led(LED6, 60, 0, 0);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_rgb_led(LED6, 0, 0, 0);
			set_led(LED7, 1);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_led(LED7, 0);
			set_rgb_led(LED8, 60, 0, 0);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_rgb_led(LED8, 0, 0, 0);
			set_led(LED1, 1);
			chThdSleepMilliseconds(TIME_DEBUG);
			set_led(LED1, 0);
			break;

		case ANIM_SLEEP:
			// turn on red with a sleep through yellow
			for (uint8_t i = 0; i < 50; i++) {
				set_rgb_led(LED2, i, i, 0);
				set_rgb_led(LED4, i, i, 0);
				set_rgb_led(LED6, i, i, 0);
				set_rgb_led(LED8, i, i, 0);
				chThdSleepMilliseconds(10);
			}
			for (uint8_t i = 0; i < 50; i++) {
				set_rgb_led(LED2, 50 + i, 50 - i, 0);
				set_rgb_led(LED4, 50 + i, 50 - i, 0);
				set_rgb_led(LED6, 50 + i, 50 - i, 0);
				set_rgb_led(LED8, 50 + i, 50 - i, 0);
				chThdSleepMilliseconds(10);
			}
			break;

		case ANIM_WAKE_UP:
			// turn off the red rgb leds
			for (uint8_t i = 0; i < 101; i += 2) {
				set_rgb_led(LED2, 100 - i, 0, 0);
				set_rgb_led(LED4, 100 - i, 0, 0);
				set_rgb_led(LED6, 100 - i, 0, 0);
				set_rgb_led(LED8, 100 - i, 0, 0);
				chThdSleepMilliseconds(10);
			}
			anim_clear_rgbs();
			break;

		case ANIM_FREQ:
			// turn off the blue leds
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
			// turn on or off the led to blue, depending on an extern variable
			set_rgb_led(LED2, 0, freq_led_intensity, freq_led_intensity);
			set_rgb_led(LED4, 0, freq_led_intensity, freq_led_intensity);
			set_rgb_led(LED6, 0, freq_led_intensity, freq_led_intensity);
			set_rgb_led(LED8, 0, freq_led_intensity, freq_led_intensity);
			break;

		default:
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

void anim_barcode(uint8_t direction_p) {
	direction = direction_p;
	animation = ANIM_BARCODE;
	chBSemSignal(&anim_ready);
}

void anim_start_freq_manual(uint8_t intensity) {
	animation = ANIM_FREQ_MANUAL;
	if (intensity <= 10) {
		freq_led_intensity = intensity;
	} else {
		freq_led_intensity = (intensity - 9) * 5;
	}
	chBSemSignal(&anim_ready);
}

void anim_stop_freq_manual(uint8_t intensity) {
	animation = ANIM_FREQ_MANUAL;
	if (intensity == 10) {
		freq_led_intensity = 0;
	} else {
		freq_led_intensity = 50 - intensity * 5;
	}
	chBSemSignal(&anim_ready);
}

void anim_stop_freq(void) {
	if (freq_led_intensity != 0) {
		animation = ANIM_FREQ;
		chBSemSignal(&anim_ready);
	}
}

void anim_sleep(void) {
	animation = ANIM_SLEEP;
	chBSemSignal(&anim_ready);
}

void anim_wake_up(void) {
	animation = ANIM_WAKE_UP;
	chBSemSignal(&anim_ready);
}

/*
 * 	@Describe:
 * 		debug mode is signaled with the front and rear red leds on
 */
void anim_debug(void) {
	animation = ANIM_DEBUG;
	chBSemSignal(&anim_ready);
}

void anim_clear_debug(void) {
	animation = ANIM_CLEAR_DEBUG;
	chBSemSignal(&anim_ready);
}

void anim_clear_rgbs(void){
	set_rgb_led(LED2, 0, 0, 0);
	set_rgb_led(LED4, 0, 0, 0);
	set_rgb_led(LED6, 0, 0, 0);
	set_rgb_led(LED8, 0, 0, 0);
}

void leds_animations_thd_start(void) {
	chThdCreateStatic(waLedAnimationThd, sizeof(waLedAnimationThd), NORMALPRIO - 1, LedAnimationThd, NULL);
}

