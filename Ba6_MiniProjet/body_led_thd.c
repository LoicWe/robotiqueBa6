#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>

#include "leds.h"
#include <body_led_thd.h>

//semaphore
static BSEMAPHORE_DECL(barcode_ready, TRUE); // @suppress("Field cannot be resolved")

static THD_WORKING_AREA(waBodyLedThd, 128);  //#### � v�rifier la taille #####//
static THD_FUNCTION(BodyLedThd, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void) arg;

	while (1) {

		//attends qu'un code barre soit lu
		chBSemWait(&barcode_ready);
		// �claire le robot en signe de validation
		set_body_led(1);

		// �teinds le robot apr�s 1 seconde
		chThdSleepMilliseconds(1000);
		set_body_led(0);

	}
}

void body_led_thd_start(void) {
	chThdCreateStatic(waBodyLedThd, sizeof(waBodyLedThd), NORMALPRIO, BodyLedThd, NULL);
}

void barcode_validate(void) {
	chBSemSignal(&barcode_ready);
}
