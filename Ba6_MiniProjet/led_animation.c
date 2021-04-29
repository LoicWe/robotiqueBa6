#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>

#include "leds.h"
#include <led_animation.h>
#include <move.h>

//semaphore
static BSEMAPHORE_DECL(anim_ready, TRUE); // @suppress("Field cannot be resolved")

static uint8_t animation = ANIM_CLEAR;
static uint8_t freq_led_intensity = 0;

static THD_WORKING_AREA(waBodyLedThd, 128);  //#### à vérifier la taille #####//
static THD_FUNCTION(BodyLedThd, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void) arg;

	while (1) {

		//attends qu'une animation soit lancée
		chBSemWait(&anim_ready);


		switch (animation) {
		case ANIM_CLEAR:
			set_rgb_led(LED2, 0, 0, 0);
			set_rgb_led(LED4, 0, 0, 0);
			set_rgb_led(LED6, 0, 0, 0);
			set_rgb_led(LED8, 0, 0, 0);
			break;

		case ANIM_BARCODE:
			// allume puis éteins le body led
			set_body_led(1);
			chThdSleepMilliseconds(800);
			set_body_led(0);

			break;

		case ANIM_DEBUG:

			break;

		case ANIM_SLEEP:
			// passe au rouge avec une touche de jaune
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
			for (uint8_t i = 0; i < 101; i += 2) {
				set_rgb_led(LED2, 100 - i, 0, 0);
				set_rgb_led(LED4, 100 - i, 0, 0);
				set_rgb_led(LED6, 100 - i, 0, 0);
				set_rgb_led(LED8, 100 - i, 0, 0);
				chThdSleepMilliseconds(10);
			}
			break;

		case ANIM_FREQ:
			set_rgb_led(LED2, 0, freq_led_intensity, freq_led_intensity);
			set_rgb_led(LED4, 0, freq_led_intensity, freq_led_intensity);
			set_rgb_led(LED6, 0, freq_led_intensity, freq_led_intensity);
			set_rgb_led(LED8, 0, freq_led_intensity, freq_led_intensity);
			break;

		case ANIM_FORWARD:
			set_rgb_led(LED2, 0, 100, 0);
			set_rgb_led(LED4, 0, freq_led_intensity, freq_led_intensity);
			set_rgb_led(LED6, 0, freq_led_intensity, freq_led_intensity);
			set_rgb_led(LED8, 0, 100, 0);
			break;

		case ANIM_BACKWARD:
			set_rgb_led(LED2, 0, freq_led_intensity, freq_led_intensity);
			set_rgb_led(LED4, 0, 100, 0);
			set_rgb_led(LED6, 0, 100, 0);
			set_rgb_led(LED8, 0, freq_led_intensity, freq_led_intensity);

			break;
		}
	}
}

void anim_barcode(void) {
	animation = ANIM_BARCODE;
	chBSemSignal(&anim_ready);
}

void anim_start_freq(uint8_t intensity) {
	animation = ANIM_FREQ;
	if (intensity == 20) {
		if (get_speed() > 0) {
			animation = ANIM_FORWARD;
		} else {
			animation = ANIM_BACKWARD;
		}
	} else if (intensity <= 10) {
		freq_led_intensity = intensity;
	} else {
		freq_led_intensity = (intensity - 9) * 5;
	}
	chBSemSignal(&anim_ready);
}

void anim_debug(void) {
	animation = ANIM_DEBUG;
	chBSemSignal(&anim_ready);
}
void anim_stop_freq(uint8_t intensity) {
	animation = ANIM_FREQ;
	if (intensity == 10) {
		freq_led_intensity = 0;
	} else {
		freq_led_intensity = 50 - intensity * 5;
	}
	chBSemSignal(&anim_ready);
}

void anim_sleep(void) {
	animation = ANIM_SLEEP;
	chBSemSignal(&anim_ready);
}

void anim_wake_up(void) {
	animation = ANIM_WAKE_UP;
	chBSemSignal(&anim_ready);
}

void anim_clear(void) {
	animation = ANIM_CLEAR;
	chBSemSignal(&anim_ready);
}

void body_led_thd_start(void) {
	chThdCreateStatic(waBodyLedThd, sizeof(waBodyLedThd), NORMALPRIO - 1, BodyLedThd, NULL);
}

/*void demo_led(uint8_t code) {

	set_led(LED3, 0);
	set_led(LED7, 0);
	set_rgb_led(LED2, 0,0,0);
	set_rgb_led(LED4, 0,0,0);
	set_rgb_led(LED6, 0,0,0);
	set_rgb_led(LED8, 0,0,0);

	switch (code) {
	case 14:
		set_rgb_led(LED2, 100,0,0);
//		toggle_rgb_led(LED2, RED_LED, 100);
		break;
	case 16:
		set_led(LED3, 2);
		break;
	case 21:
		set_rgb_led(LED4, 100,0,0);
//		toggle_rgb_led(LED4, RED_LED, 100);
		break;
	case 24:
		set_rgb_led(LED6, 100,0,0);
//		toggle_rgb_led(LED6, RED_LED, 100);
		break;
	case 32:
		set_led(LED7, 2);
		break;
	case 33:
		set_rgb_led(LED8, 100,0,0);
//		toggle_rgb_led(LED8, RED_LED, 100);
		break;
	default:
		break;
	}
	chThdSleepMilliseconds(500);

}*/
