#include <led_animation.h>
#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>

#include "leds.h"

//semaphore
static BSEMAPHORE_DECL(anim_ready, TRUE); // @suppress("Field cannot be resolved")

static uint8_t animation = ANIM_CLEAR;

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

		case ANIM_START_FREQ:
			// passe au vert avec une touche de bleu
			for (uint8_t i = 0; i < 30; i++) {
				set_rgb_led(LED2, 0, 30 + i, 14 - i / 2);
				set_rgb_led(LED4, 0, 30 + i, 14 - i / 2);
				set_rgb_led(LED6, 0, 30 + i, 14 - i / 2);
				set_rgb_led(LED8, 0, 30 + i, 14 - i / 2);
				chThdSleepMilliseconds(10);
			}

			chThdSleepMilliseconds(500);

			for (uint8_t i = 0; i < 30; i++) {
				set_rgb_led(LED2, 0, 60 - i, i / 2);
				set_rgb_led(LED4, 0, 60 - i, i / 2);
				set_rgb_led(LED6, 0, 60 - i, i / 2);
				set_rgb_led(LED8, 0, 60 - i, i / 2);
				chThdSleepMilliseconds(10);
			}
			chThdSleepMilliseconds(500);

			break;

		case ANIM_STOP_FREQ:
			for (uint8_t i = 0; i < 31; i++) {
				set_rgb_led(LED2, 0, 30 - i, 15 - i / 2);
				set_rgb_led(LED4, 0, 30 - i, 15 - i / 2);
				set_rgb_led(LED6, 0, 30 - i, 15 - i / 2);
				set_rgb_led(LED8, 0, 30 - i, 15 - i / 2);
				chThdSleepMilliseconds(10);
			}
			break;
		}
	}
}

void anim_barcode(void) {
	animation = ANIM_BARCODE;
	chBSemSignal(&anim_ready);
}

void anim_debug(void) {
	animation = ANIM_DEBUG;
	chBSemSignal(&anim_ready);
}

void anim_start_freq(void) {
	animation = ANIM_START_FREQ;
	chBSemSignal(&anim_ready);
}

void anim_stop_freq(void) {
	animation = ANIM_STOP_FREQ;
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
