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
#include <mode_selection.h>
#include <communications.h>
#include <audio_processing.h>
#include <leds_animations.h>

static void serial_start(void) {
	static SerialConfig ser_cfg = { 115200, 0, 0, 0, };

	sdStart(&SD3, &ser_cfg); // UART3.
}

//	used to check execution time of different parts
 static void timer12_start(void) {
 //General Purpose Timer configuration
 //timer 12 is a 16 bit timer so we can measure time
 //to about 65ms with a 1Mhz counter
 static const GPTConfig gpt12cfg = { 10000, // 10kHz timer clock in order to measure mS.
 NULL,
 0, 0 };

 gptStart(&GPTD12, &gpt12cfg);
 //let the timer count to max value
 gptStartContinuous(&GPTD12, 0xFFFF);
 }


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
	timer12_start();

	//starts the camera
	dcmi_start();
	po8030_start();
	process_image_start();
	//starts the microphones processing thread.
	mic_start(&processAudioData);
	//inits the motors
	motors_init();
	//start the ToF distance sensor
	VL53L0X_start();

	//starts the potentiometer thread
	mode_selection_thd_start();
	//inits the PI regulator
	pi_regulator_thd_start();
	//starts the led thread for visual feedback
	leds_animations_thd_start();
	//starts the debug messager services
	debug_messager_thd_start();

	/* Infinite loop. */
	while (1) {

		// basic mode of operation
		switch (get_punky_state()) {
			case PUNKY_DEMO:
				punky_run();
				break;

			case PUNKY_DEBUG:
				punky_run();
				break;

			case PUNKY_SLEEP:
				get_image_stop();
				pi_regulator_stop();
				microphone_stop();
				move_stop();
				break;
		}

		//waits 0.5 second
		chThdSleepMilliseconds(500);
	}
}

/*
 *	@Describe:
 *		The robot has 2 modes, "frequency" and "PI". If a short distance is detected,
 *		PI mode is running : the robot drives at the right distance of the barcode and
 *		image detection is running to find the code.
 *		Otherwise, frequeny mode is running. The robot can be piloted with the voice.
 *
 */
void punky_run(void) {
	uint16_t distance;
	int8_t code;
	static bool code_found = false;

	distance = VL53L0X_get_dist_mm();

	//mecanism to reactivate the frequency mode after a code is found
	if (distance > MAX_DISTANCE_DETECTED) {
		code_found = false;
	}

	// => PI mode
	if (distance > MIN_DISTANCE_DETECTED && distance < MAX_DISTANCE_DETECTED && !code_found) {
		if (get_punky_state() == PUNKY_DEBUG)	//debug mode
			debug_message("== PI CODE ==", LIGHTNING, LOW_PRIO);
		microphone_stop();
		pi_regulator_start();
		get_image_start();

		code = get_code();

		if (code == END_PATTERN) {
			if (get_punky_state() == PUNKY_DEBUG)	//debug mode
				debug_message("Trop gauche", LIGHTNING, HIGH_PRIO);

			set_rotation(BARCODE_ROTATION_SPEED);

		} else if (code == START_PATTERN) {
			if (get_punky_state() == PUNKY_DEBUG)	//debug mode
				debug_message("Trop droite", LIGHTNING, HIGH_PRIO);

			set_rotation(-BARCODE_ROTATION_SPEED);

		} else if (code == NOT_DETECTED) {
			if (get_punky_state() == PUNKY_DEBUG)	//debug mode
				debug_message("Parfait", LIGHTNING, LOW_PRIO);

			set_rotation(0);

		} else {
			// a valide code is captured
			set_speed(code);
			code_found = true; // stop the robot, therefore activate frequency mode
			anim_barcode(get_speed() > 0 ? ANIM_FORWARD : ANIM_BACKWARD);
		}

	}
	// => frequency mode
	else {
		if (get_punky_state() == PUNKY_DEBUG)	//debug mode
			debug_message("== Frequences ==", LIGHTNING, LOW_PRIO);

		get_image_stop();
		pi_regulator_stop();
		microphone_start();
	}
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void) {
	chSysHalt("Stack smashing detected");
}
