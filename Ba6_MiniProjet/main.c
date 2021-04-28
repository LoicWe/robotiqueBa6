#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "memory_protection.h"
#include "leds.h"
#include <usbcfg.h>
#include <main.h>
#include <chprintf.h>
#include <motors.h>
#include <audio/microphone.h>

#include <audio_processing.h>
#include <fft.h>
#include <communications.h>
#include <arm_math.h>
#include <process_image.h>
#include <sensors/VL53L0X/VL53L0X.h>
#include <body_led_thd.h>
#include <potentiometer.h>
#include "move.h"
#include "spi_comm.h"

static void serial_start(void) {
	static SerialConfig ser_cfg = { 115200, 0, 0, 0, };

	sdStart(&SD3, &ser_cfg); // UART3.
}

static void timer12_start(void) {
	//General Purpose Timer configuration
	//timer 12 is a 16 bit timer so we can measure time
	//to about 65ms with a 1Mhz counter
	static const GPTConfig gpt12cfg = { 1000000, /* 1MHz timer clock in order to measure uS.*/
	NULL, /* Timer callback.*/
	0, 0 };

	gptStart(&GPTD12, &gpt12cfg);
	//let the timer count to max value
	gptStartContinuous(&GPTD12, 0xFFFF);
}

int main(void) {
	halInit();
	chSysInit();
	mpu_init();

	chThdSleepMilliseconds(1000);


	//starts the serial communication
	serial_start();
	//starts the USB communication
	usb_start();
	//starts the camera
//	dcmi_start();
//	po8030_start();
//	process_image_start();
	//start the bodyled thread
//	body_led_thd_start();

	//starts timer 12
	timer12_start();
	//inits the motors
	motors_init();
	//start the ToF distance sensor
	VL53L0X_start();
	//init the PI regulator
	pi_regulator_init();

	uint16_t distance = 0;
	uint8_t code = 0;
	//start the spi for the rgb leds
	//starts the microphones processing thread.
	//it calls the callback given in parameter when samples are ready
//	mic_start(&processAudioData);

	chprintf((BaseSequentialStream *) &SD3, "initalisation -->  ok \r");


	/* Infinite loop. */
	while (1) {

//			chprintf((BaseSequentialStream *) &SD3, "start main \r");

			//laser
			distance = VL53L0X_get_dist_mm();

//			chprintf((BaseSequentialStream *) &SD3, "get distance \r");

			if (distance > MIN_DISTANCE_DETECTED && distance < MAX_DISTANCE_DETECTED) {
				pi_regulator_start();

				get_images();

//				chprintf((BaseSequentialStream *) &SD3, "valide 2 \r");

				code = get_code();

//				chprintf((BaseSequentialStream *) &SD3, "valide 3 \r");

//				if (code != 0) {
//					demo_led(code);
//					set_speed(convert_speed(code));
//				}
				set_led(LED5, 1);

//				chprintf((BaseSequentialStream *) &SD3, "valide 4 \r");

				deactivate_motors();

//				chprintf((BaseSequentialStream *) &SD3, "valide 5 \r");

			} else {
//				chprintf((BaseSequentialStream *) &SD3, "refus 1 \r");

				stop_images();

//				chprintf((BaseSequentialStream *) &SD3, "refus 2 \r");

				set_led(LED5, 0);

//				chprintf((BaseSequentialStream *) &SD3, "refus 3 \r");

				pi_regulator_stop();

//				chprintf((BaseSequentialStream *) &SD3, "refus 4 \r");

				activate_motors();

//				chprintf((BaseSequentialStream *) &SD3, "refus 5 \r");

			}

//			chprintf((BaseSequentialStream *) &SD3, "distance = %d \r", distance);

		//waits 0.5 second
		chThdSleepMilliseconds(500);

//		chprintf((BaseSequentialStream *) &SD3, "reboucle \r");

	}
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void) {
	chSysHalt("Stack smashing detected");
}
