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
	init_potentiometer();

	//starts timer 12
	timer12_start();
	//inits the motors
	motors_init();
	//start the ToF distance sensor
	VL53L0X_start();
	uint16_t distance = 0;
	uint8_t code = 0;
	//start the spi for the rgb leds
	spi_comm_start();

	chThdCreateStatic(waThdPotentiometer, sizeof(waThdPotentiometer), NORMALPRIO, ThdPotentiometer, NULL);

	//starts the microphones processing thread.
	//it calls the callback given in parameter when samples are ready
	mic_start(&processAudioData);

	/* Infinite loop. */
	while (1) {


		switch (get_punky_state()) {
		case PUNKY_DEMO:

			distance = VL53L0X_get_dist_mm();
			if (distance > min_dist_barcode && distance < max_dist_barcode) {
				get_images();
				code = get_code();
				if (code != 0) {
					demo_led(code);
					set_speed(convert_speed(code));
				}
				set_led(LED5, 1);
			}
			else {
				stop_images();
				set_led(LED5, 0);
			}
			break;

			// éteind les LED sauf la 1 (témoins de pause)
			// met en pause les threads pour économie d'énergie
		case PUNKY_DEBUG:
			clear_leds();
			set_led(LED3, 1);
			set_body_led(0);
			set_front_led(0);
			stop_images();
			break;

			// sort du mode pause, redémarre les threads
		case PUNKY_SLEEP:
			clear_leds();
			set_led(LED1, 1);
			set_body_led(0);
			set_front_led(0);
			deactivate_motors();
			break;

		case PUNKY_WAKE_UP:
			activate_motors();
			clear_leds();
			set_body_led(0);
			set_front_led(0);
			set_punky_state(PUNKY_DEMO);
			break;

		default:
			break;
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
