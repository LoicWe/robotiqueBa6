#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arm_math.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "usbcfg.h"
#include "spi_comm.h"
#include "memory_protection.h"
#include <leds.h>
#include <motors.h>
#include <audio/microphone.h>
#include <sensors/VL53L0X/VL53L0X.h>

#include <main.h>
#include <move.h>
#include <led_animation.h>
#include <process_image.h>
#include <potentiometer.h>
#include <communications.h>
#include <audio_processing.h>


static void serial_start(void) {
	static SerialConfig ser_cfg = { 115200, 0, 0, 0, };

	sdStart(&SD3, &ser_cfg); // UART3.
}

/*	used to check execution time of different parts
static void timer12_start(void) {
	//General Purpose Timer configuration
	//timer 12 is a 16 bit timer so we can measure time
	//to about 65ms with a 1Mhz counter
	static const GPTConfig gpt12cfg = { 1000000, // 1MHz timer clock in order to measure uS.
	NULL,
	0, 0 };

	gptStart(&GPTD12, &gpt12cfg);
	//let the timer count to max value
	gptStartContinuous(&GPTD12, 0xFFFF);
}
*/

//private function for the main
void punky_run(void);

int main(void) {
	halInit();
	chSysInit();
	mpu_init();

	//starts the serial communication
	serial_start();
	//starts the USB communication
	usb_start();
	//start communication with the ESP32
	spi_comm_start();
	//starts timer 12
	//timer12_start();

	//starts the camera
	dcmi_start();
	po8030_start();
	process_image_start();
	//starts the microphones processing thread.
	//it calls the callback given in parameter when samples are ready
	mic_start(&processAudioData);
	//inits the motors
	motors_init();
	//start the ToF distance sensor
	VL53L0X_start();

	//starts the potentiometer thread
	potentiometer_init();
	//inits the PI regulator
	pi_regulator_init();
	//starts the led thread for visual feedback
	leds_animations_thd_start();

	//declare variables
	static uint8_t punky_state = PUNKY_DEMO;

	/* Infinite loop. */
	while (1) {
		//get the current state
		punky_state = get_punky_state();

		// basic mode of operation
		if (punky_state == PUNKY_DEMO) {
			punky_run();
		}

		// add animation and send data to the computer
		else if (punky_state == PUNKY_DEBUG) {
			// add an animation on top of the other
			punky_run();
			anim_debug();
		}

		// deactivate everything except a visual indicator
		else if (punky_state == PUNKY_SLEEP) {
			microphone_stop();
			get_image_stop();
			pi_regulator_stop();
			motor_control_stop();

		}

		// transition state to restart every thread after sleep mode
		else if (punky_state == PUNKY_WAKE_UP) {
			set_punky_state(PUNKY_DEMO);
		}

		//waits 0.5 second
		chThdSleepMilliseconds(500);
	}
}

void punky_run(void) {
	uint16_t distance = 0;
	int8_t code = 0;

	//switch between frequence mode and Pi mode using the TOF
	distance = VL53L0X_get_dist_mm();
	chprintf((BaseSequentialStream *) &SD3, "Distance : %d \r", distance);

	// search for a barre code if distance is in the good range
	if (distance > MIN_DISTANCE_DETECTED && distance < MAX_DISTANCE_DETECTED) {
		if (get_punky_state() == PUNKY_DEBUG)	//debug mode
			chprintf((BaseSequentialStream *) &SD3, "\r===== \rMode PI \r");
		//stop useless process
		microphone_stop();
		motor_control_stop();

		//start pi_regulator and image caption
		pi_regulator_run();
		get_image_run();
		code = get_code();

		if (code == 2) {
			chprintf((BaseSequentialStream *) &SD3, "Trop gauche\r");
			if (get_punky_state() == PUNKY_DEBUG)	//debug mode
				chprintf((BaseSequentialStream *) &SD3, "YES end\r");
			set_rotation(60);
		}
		else if(code == 1){
			chprintf((BaseSequentialStream *) &SD3, "Trop droite\r");
			if (get_punky_state() == PUNKY_DEBUG)	//debug mode
				chprintf((BaseSequentialStream *) &SD3, "YES start\r");
			set_rotation(-60);
		}
		else if (code == 0){
			chprintf((BaseSequentialStream *) &SD3, "parfait\r");
			if (get_punky_state() == PUNKY_DEBUG)	//debug mode
				chprintf((BaseSequentialStream *) &SD3, "NO\r");
			set_rotation(0);
		}
		else{
			// if a good code is detected, set new speed
			set_speed(code);
			anim_barcode();
		}

	} else {
		if (get_punky_state() == PUNKY_DEBUG)	//debug mode
			chprintf((BaseSequentialStream *) &SD3, "\r===== \rMode Frequences \r");
		//stop useless process
		get_image_stop();
		pi_regulator_stop();

		//start frequency control
		motor_control_run();
		microphone_run();
	}
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void) {
	chSysHalt("Stack smashing detected");
}
