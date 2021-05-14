#include "ch.h"
#include "hal.h"
#include "selector.h"
#include <math.h>
#include <usbcfg.h>

#include <main.h>
#include <mode_selection.h>
#include <leds_animations.h>

static uint8_t punky_state = PUNKY_DEMO;

static THD_WORKING_AREA(waThdPotentiometer, 128);
static THD_FUNCTION(ThdPotentiometer, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void) arg;

	static uint8_t selector;
	static uint8_t old_selector;
	static bool loopback;			//to detect selector loopback
	uint8_t change_state;
	old_selector = get_selector();

	while (1) {
		change_state = STAY;
		selector = get_selector();		//get the new position
		loopback = (old_selector == 0 || old_selector == 15) ? 1 : 0;
		if (!loopback) {
			//no loopback
			if (old_selector < selector && selector < old_selector + 3) {
				change_state = RIGHT;
			} else if (old_selector > selector && selector > old_selector - 3) {
				change_state = LEFT;
			}
		} else {
			//special case to detect loopback
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

		//change state
		switch (change_state) {
		case STAY:
			break;

		case RIGHT:
			switch (punky_state) {
			case PUNKY_DEMO:
				punky_state = PUNKY_SLEEP;
				anim_sleep();
				break;
			case PUNKY_DEBUG:
				anim_clear_debug(); 	// reset all animation in order to stop the punky debug overlay
				punky_state = PUNKY_DEMO;
				break;
			default:
				break;
			}
			break;

		case LEFT:
			switch (punky_state) {
			case PUNKY_DEMO:
				punky_state = PUNKY_DEBUG;
				anim_debug();
				break;
			case PUNKY_SLEEP:
				anim_wake_up();
				punky_state = PUNKY_DEMO;
				break;
			default:
				break;
			}
		}

		old_selector = selector;
		chThdSleepMilliseconds(500);
	}
}

/*
 * 	@Describe:
 * 		Get the state of the robot
 */
uint8_t get_punky_state(void) {
	return punky_state;
}

void mode_selection_thd_start(void) {
	chThdCreateStatic(waThdPotentiometer, sizeof(waThdPotentiometer), NORMALPRIO+2, ThdPotentiometer, NULL);
}
