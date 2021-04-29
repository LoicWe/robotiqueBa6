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
#include <potentiometer.h>
#include "move.h"
#include "spi_comm.h"
#include <led_animation.h>


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

	anim_clear();

	//starts the serial communication
	serial_start();
	//starts the USB communication
	usb_start();
	//starts the camera
	dcmi_start();
	po8030_start();
	process_image_start();
	//start the bodyled thread
	body_led_thd_start();

	//starts timer 12
	timer12_start();
	//inits the motors
	motors_init();
	//start the ToF distance sensor
	VL53L0X_start();

	//init the PI regulator
	pi_regulator_init();

	//starts the microphones processing thread.
	//it calls the callback given in parameter when samples are ready
	mic_start(&processAudioData);

	//
	potentiometer_init();
	uint8_t punky_state = PUNKY_DEMO;

	spi_comm_start();

	uint16_t distance = 0;
	uint8_t code = 0;


	/* Infinite loop. */
	while (1) {

		// �tat du robot, actif, inactif, en r�veil
		punky_state = get_punky_state();

		if (punky_state == PUNKY_DEMO) {

			distance = VL53L0X_get_dist_mm();

			if (distance > MIN_DISTANCE_DETECTED && distance < MAX_DISTANCE_DETECTED) {

				microphone_stop();
				motor_control_stop();
				pi_regulator_run();
				get_image_run();

				code = get_code();

				if (code != 0) {
					set_speed(convert_speed(code));
				}

			} else {
				get_image_stop();
				pi_regulator_stop();
				motor_control_run();
				microphone_run();
			}
		}

		// d�sactivation de toutes les fonctions
		else if (punky_state == PUNKY_SLEEP) {

			get_image_stop();
			pi_regulator_stop();
			microphone_stop();
			motor_control_stop();
		}

		// r�veil de punky
		else if (punky_state == PUNKY_WAKE_UP){
			set_punky_state(PUNKY_DEMO);
		}

		//waits 0.5 second
		chThdSleepMilliseconds(500);
	}
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void) {
	chSysHalt("Stack smashing detected");
}
