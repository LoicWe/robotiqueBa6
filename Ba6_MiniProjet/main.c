#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arm_math.h>

#include "ch.h"
#include "hal.h"

#include "usbcfg.h"
#include "spi_comm.h"
#include "memory_protection.h"
#include <motors.h>
#include <audio/microphone.h>
#include <sensors/VL53L0X/VL53L0X.h>

#include <main.h>
#include <move.h>
#include <process_image.h>
#include <potentiometer.h>
#include <communications.h>
#include <audio_processing.h>
#include <debug_messager.h>
#include <led_animation.h>

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
	//starts the debug messager services
	debug_messager_thd_start();

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
		}

		// deactivate everything except a visual indicator
		else if (punky_state == PUNKY_SLEEP) {
			get_image_stop();
			pi_regulator_stop();
			motor_control_stop();
			microphone_stop();
			move_stop();
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
	static bool code_found = false;

	//switch between frequence mode and Pi mode using the TOF
	distance = VL53L0X_get_dist_mm();

	if(distance > MAX_DISTANCE_DETECTED){
		code_found = false;
	}

	// search for a barcode if distance is in the good range
	if (distance > MIN_DISTANCE_DETECTED && distance < MAX_DISTANCE_DETECTED && !code_found) {
		if (get_punky_state() == PUNKY_DEBUG)	//debug mode
			debug_message("== PI CODE ==", LIGHTNING, LOW_PRIO);
		//stop unrelated processes
		microphone_stop();
		motor_control_stop();

		//start pi_regulator and image capture
		pi_regulator_run();
		get_image_run();
		code = get_code();

		// valide code are between 13 and 39
		if (code == 2) {
			// barcode is too far to the left
			if (get_punky_state() == PUNKY_DEBUG)	//debug mode
				debug_message("Trop gauche", LIGHTNING, HIGH_PRIO);

			set_rotation(BARCODE_ROT_SPEED);

		} else if (code == 1) {
			// barcode is too far to the right
			if (get_punky_state() == PUNKY_DEBUG)	//debug mode
				debug_message("Trop droite", LIGHTNING, HIGH_PRIO);

			set_rotation(-BARCODE_ROT_SPEED);

		} else if (code == 0) {
			// no rotation indication could be found
			if (get_punky_state() == PUNKY_DEBUG)	//debug mode
				debug_message("Parfait", LIGHTNING, LOW_PRIO);

			set_rotation(0);

		} else {
			// a valide code is captured
			set_speed(code);
			code_found = true;
			anim_barcode(get_speed() > 0 ? ANIM_FORWARD:ANIM_BACKWARD);
		}

	} else {
		if (get_punky_state() == PUNKY_DEBUG)	//debug mode
			debug_message("== Frequences ==", LIGHTNING, LOW_PRIO);
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
