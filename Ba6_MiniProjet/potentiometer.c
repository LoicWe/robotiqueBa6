#include "ch.h"
#include "hal.h"
#include "selector.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <main.h>
#include <potentiometer.h>

static uint8_t punky_state = PUNKY_DEMO;

static THD_WORKING_AREA(waThdPotentiometer, 128);
static THD_FUNCTION(ThdPotentiometer, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void) arg;

	static uint8_t selector;
	static uint8_t old_selector;
	static bool loopback;
	selector = get_selector();
	old_selector = selector;

	while (1) {
		uint8_t change_state = STAY;
		selector = get_selector();
		loopback = (old_selector == 0 || old_selector == 15) ? 1 : 0;
		if (!loopback) {
			if (old_selector < selector && selector < old_selector + 3) {
				change_state = RIGHT;
			} else if (old_selector > selector && selector > old_selector - 3) {
				change_state = LEFT;
			}
		} else {
			if (old_selector == 0 && selector != 0 && selector < 3) {
				change_state = RIGHT;
			} else if (old_selector == 0 && selector != 0 && selector > 13) {
				change_state = LEFT;
			} else if (old_selector == 15 && selector != 15 && selector < 2) {
				change_state = RIGHT;
			} else if (old_selector == 15 && selector != 15 && selector > 12) {
				change_state = LEFT;
			}
		}
		switch (change_state) {
		case STAY:
			break;

		case RIGHT:
			switch (punky_state) {
			case PUNKY_DEMO:
				punky_state = PUNKY_SLEEP;
				break;
			case PUNKY_DEBUG:
				punky_state = PUNKY_WAKE_UP;
				break;
			default:
				break;
			}
			break;

		case LEFT:
			switch (punky_state) {
			case PUNKY_DEMO:
				punky_state = PUNKY_DEBUG;
				break;
			case PUNKY_SLEEP:
				punky_state = PUNKY_WAKE_UP;
				break;
			default:
				break;
			}
		}

		old_selector = selector;
		chThdSleepMilliseconds(500);
	}
}

void init_potentiometer(void) {
	chThdCreateStatic(waThdPotentiometer, sizeof(waThdPotentiometer), NORMALPRIO, ThdPotentiometer, NULL);
}

void set_punky_state(uint8_t new_punky_state) {
	punky_state = new_punky_state;
}

uint8_t get_punky_state(void) {
	return punky_state;
}

