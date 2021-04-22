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
	body_led_thd_start();
	init_potentiometer();

    //starts timer 12
    timer12_start();
    //inits the motors
    motors_init();
    VL53L0X_start();
	uint16_t distance = 0;


//    //temp tab used to store values in complex_float format
//    //needed bx doFFT_c
//    static complex_float temp_tab[FFT_SIZE];
//    //send_tab is used to save the state of the buffer to send (double buffering)
//    //to avoid modifications of the buffer while sending it
//    static float send_tab[FFT_SIZE];

    //starts the microphones processing thread.
    //it calls the callback given in parameter when samples are ready
    mic_start(&processAudioData);

	/* Infinite loop. */
	while (1) {

		switch (get_punky_state()) {
		case PUNKY_DEMO:
			//audio processing

			//code barre


			//laser
			distance = VL53L0X_get_dist_mm();
			if (distance > min_dist_barcode && distance < max_dist_barcode){
				capture_image(YES);
				set_led(LED5, 1);
			}else{
				capture_image(NO);
				set_led(LED5, 0);
			}
//			        //waits until a result must be sent to the computer
//			        wait_send_to_computer();
//			        //we copy the buffer to avoid conflicts
//			        arm_copy_f32(get_audio_buffer_ptr(LEFT_OUTPUT), send_tab, FFT_SIZE);
//			        SendFloatToComputer((BaseSequentialStream *) &SD3, send_tab, FFT_SIZE);

			break;

			// �teind les LED sauf la 1 (t�moins de pause)
			// met en pause les threads pour �conomie d'�nergie
		case PUNKY_DEBUG:
			clear_leds();
			set_led(LED3, 1);
			set_body_led(0);
			set_front_led(0);
			break;

			// sort du mode pause, red�marre les threads
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
    	//waits 1 second
        chThdSleepMilliseconds(500);
	}
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void) {
	chSysHalt("Stack smashing detected");
}
