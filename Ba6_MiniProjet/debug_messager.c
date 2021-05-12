#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>
#include <string.h>

#include <debug_messager.h>

//semaphore
static BSEMAPHORE_DECL(send_debug, FALSE); // @suppress("Field cannot be resolved")

static char str[MAX_LENGTH_STRING];
static int16_t value1 = 0;		// parameter 1 to print
static int16_t value2 = 0;		// parameter 2 to print
static int16_t value3 = 0;		// parameter 3 to print
static int16_t value4 = 0;		// parameter 4 to print
static systime_t time = 100; 	// CANNOT be 0, otherwise panic error at init
static bool sending = false;	// tricks for priority message
static uint8_t nbr_values = 1;	// number of parameters

/*
 * 	@Describe:
 * 		Generic function to print informations in the console.
 * 		The goal of this thread is to print usefule and relevant data
 * 		of the robot in a readable way for demonstration.
 * 		User can set with setting functions the message, the values and
 * 		the duration of the message.
 * 		A priority is given such that high priority messages can force printing (not 100%)
 *
 */
static THD_WORKING_AREA(waDebugMsgThd, 512);
static THD_FUNCTION(DebugMsgThd, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void) arg;

	while (1) {

		//wait for a message to be received
		chBSemWait(&send_debug);


		switch (nbr_values) {
		case 0:
			chprintf((BaseSequentialStream *) &SD3, "%s \r", str);
			break;
		case 1:
			chprintf((BaseSequentialStream *) &SD3, "%s %d \r", str, value1);
			break;
		case 4:
			chprintf((BaseSequentialStream *) &SD3, "%s %d %d %d   |   %d \r", str, value1, value2, value3, value4);
			break;
		}

		chThdSleepMilliseconds(time);
		sending = false;
	}
}

/*
 * 	@Describe:
 * 		This function allow to display messages in the console for debug,
 * 		with display time. The semaphore block any other msg
 * 		to be sent while the current one is running.
 *
 * 	@Params:
 *		char 		*str_p			a string to be displayed. MAX 20 characters
 *		systime_t 	time_p			the time in milliseconds to be displayed
 *		bool 		high_prio_p		The priority of the message
 */
void debug_message(char *str_p, systime_t time_p, bool high_prio_p) {
	if (sending == false || high_prio_p) {

		sending = true;
		if (strlen(str_p) > MAX_LENGTH_STRING) {
			strcpy(str, "msg trop long");
		} else {
			strcpy(str, str_p);
		}

		time = time_p;
		nbr_values = 0;

//		if(high_prio_p == HIGH_PRIO)
//			chBSemReset(&send_debug, FALSE);

		chBSemSignal(&send_debug);
	}
}

/*
 * 	@Describe:
 * 		This function allow to display messages in the console for debug,
 * 		with display time. The semaphore block any other msg
 * 		to be sent while the current one is running.
 *
 * 	@Params:
 *		char 		*str_p			a string to be displayed. MAX 20 characters
 *		int16_t 	value1_p		an integer value to display
 *		systime_t 	time_p			the time in milliseconds to be displayed
 *		bool 		high_prio_p		The priority of the message
 */
void debug_message_1(char *str_p, int16_t value1_p, systime_t time_p, bool high_prio_p) {

	if (sending == false || high_prio_p) {

		sending = true;
		if (strlen(str_p) > MAX_LENGTH_STRING) {
			strcpy(str, "msg trop long");
		} else {
			strcpy(str, str_p);
		}

		value1 = value1_p;
		time = time_p;
		nbr_values = 1;
//		if(high_prio_p == HIGH_PRIO)
//			chBSemReset(&send_debug, FALSE);

		chBSemSignal(&send_debug);
	}
}

/*
 * 	@Describe:
 * 		This function allow to display messages in the console for debug,
 * 		with display time. The semaphore block any other msg
 * 		to be sent while the current one is running.
 *
 * 	@Params:
 *		char 		*str_p			a string to be displayed. MAX 20 characters
 *		systime_t 	time_p			the time in milliseconds to be displayed
 *		int16_t 	value1_p		an integer value to display
 *		int16_t 	value2_p		an integer value to display
 *		int16_t 	value3_p		an integer value to display
 *		int16_t 	value4_p		an integer value to display
 *		bool 		high_prio_p		The priority of the message
 */
void debug_message_4(char *str_p, int16_t value1_p, int16_t value2_p, int16_t value3_p, int16_t value4_p, systime_t time_p, bool high_prio_p) {

	if (sending == false || high_prio_p) {

		sending = true;
		if (strlen(str_p) > MAX_LENGTH_STRING) {
			strcpy(str, "msg trop long");
		} else {
			strcpy(str, str_p);
		}
		value1 = value1_p;
		value2 = value2_p;
		value3 = value3_p;
		value4 = value4_p;
		time = time_p;

		nbr_values = 4;

//		if(high_prio_p == HIGH_PRIO)
//			chBSemReset(&send_debug, FALSE);

		chBSemSignal(&send_debug);
	}
}

void debug_messager_thd_start(void) {
	chThdCreateStatic(waDebugMsgThd, sizeof(waDebugMsgThd), NORMALPRIO, DebugMsgThd, NULL);
}

