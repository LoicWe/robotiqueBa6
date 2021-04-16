#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "memory_protection.h"
#include "leds.h"
#include "selector.h"
#include <usbcfg.h>
#include <main.h>
#include <chprintf.h>
#include <motors.h>
#include <audio/microphone.h>

#include <audio_processing.h>
#include <fft.h>
#include <communications.h>
#include <arm_math.h>
#include <pi_regulator.h>
#include <process_image.h>
#include <sensors/VL53L0X/VL53L0X.h>

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
		loopback = (old_selector == 0 || old_selector == 15) ? 1:0;
		clear_leds();
		if (!loopback){
			if (old_selector<selector && selector<old_selector+3){
				set_led(LED1, 1);
			} else if (old_selector>selector && selector>old_selector-3){
				set_led(LED3, 1);
			}
		} else {
			if (old_selector == 0 && selector != 0 && selector < 3){
				set_led(LED1, 1);
			}else if (old_selector == 0 && selector != 0 && selector > 13){
				set_led(LED3, 1);
			}else if (old_selector == 15 && selector != 15 && selector < 2){
				set_led(LED1, 1);
			}else if (old_selector == 15 && selector != 15 && selector > 12){
				set_led(LED3, 1);
			}
		}
		old_selector = selector;
		chThdSleepMilliseconds(1000);
	}
}

static void serial_start(void)
{
	static SerialConfig ser_cfg = {
	    115200,
	    0,
	    0,
	    0,
	};

	sdStart(&SD3, &ser_cfg); // UART3.
}

static void timer12_start(void){
    //General Purpose Timer configuration   
    //timer 12 is a 16 bit timer so we can measure time
    //to about 65ms with a 1Mhz counter
    static const GPTConfig gpt12cfg = {
        1000000,        /* 1MHz timer clock in order to measure uS.*/
        NULL,           /* Timer callback.*/
        0,
        0
    };

    gptStart(&GPTD12, &gpt12cfg);
    //let the timer count to max value
    gptStartContinuous(&GPTD12, 0xFFFF);
}

int main(void)
{
    halInit();
    chSysInit();
    mpu_init();

    //starts the serial communication
    serial_start();
    //starts the USB communication
    usb_start();
//    //starts timer 12
//    timer12_start();
//    //inits the motors
//    motors_init();
    VL53L0X_start();
    uint16_t distance = 0;

	chThdCreateStatic(waThdPotentiometer, sizeof(waThdPotentiometer), NORMALPRIO, ThdPotentiometer, NULL);

//    //temp tab used to store values in complex_float format
//    //needed bx doFFT_c
//    static complex_float temp_tab[FFT_SIZE];
//    //send_tab is used to save the state of the buffer to send (double buffering)
//    //to avoid modifications of the buffer while sending it
//    static float send_tab[FFT_SIZE];

//    //starts the microphones processing thread.
//    //it calls the callback given in parameter when samples are ready
//    mic_start(&processAudioData);

    /* Infinite loop. */
    while (1) {
    	//ajout thread potentiometre


    	//audio processing

    	//code barre

    	//laser
    	distance = VL53L0X_get_dist_mm();
    	if(distance > 200)
    		set_body_led(1);
    	else
    		set_body_led(0);


//        //waits until a result must be sent to the computer
//        wait_send_to_computer();
//        //we copy the buffer to avoid conflicts
//        arm_copy_f32(get_audio_buffer_ptr(), send_tab, FFT_SIZE);
//        SendFloatToComputer((BaseSequentialStream *) &SD3, send_tab, FFT_SIZE);
    }
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void)
{
    chSysHalt("Stack smashing detected");
}
