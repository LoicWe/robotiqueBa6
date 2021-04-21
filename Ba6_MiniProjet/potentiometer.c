#include "ch.h"
#include "hal.h"
#include "selector.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <main.h>
#include <potentiometer.h>

uint8_t punky_state = PUNKY_PLAY;

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
		selector = get_selector();
		loopback = (old_selector == 0 || old_selector == 15) ? 1 : 0;
		if (!loopback) {
			// si tourne dans le sens des aiguilles d'une montre - arrêt
			if (old_selector < selector && selector < old_selector + 3) {
				punky_state = PUNKY_SLEEP;
			} else if (old_selector > selector && selector > old_selector - 3) {
				punky_state = PUNKY_WAKE_UP;
			}
		} else {
			if (old_selector == 0 && selector != 0 && selector < 3) {
				punky_state = PUNKY_SLEEP;
			} else if (old_selector == 0 && selector != 0 && selector > 13) {
				punky_state = PUNKY_WAKE_UP;
			} else if (old_selector == 15 && selector != 15 && selector < 2) {
				punky_state = PUNKY_SLEEP;
			} else if (old_selector == 15 && selector != 15 && selector > 12) {
				punky_state = PUNKY_WAKE_UP;
			}
		}

		old_selector = selector;
		chThdSleepMilliseconds(1000);
	}
}

void init_potentiometer(void) {
	chThdCreateStatic(waThdPotentiometer, sizeof(waThdPotentiometer), NORMALPRIO, ThdPotentiometer, NULL);
}

void set_punky_state(uint8_t new_punky_state){
	punky_state = new_punky_state;
}

uint8_t get_punky_state(void){
	return punky_state;
}

