#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <main.h>
#include <motors.h>
#include <move.h>

static float speed = 600;


void move(float rotation) {

	left_motor_set_speed(speed - rotation);
	right_motor_set_speed(speed + rotation);
}

void move_stop(void) {
	left_motor_set_speed(0);
	right_motor_set_speed(0);
}

void set_speed(float new_speed) {
	speed = new_speed;
}

int16_t convert_speed(uint8_t code){

	int16_t speed = 0;

	if(code > 25){
		speed = 67*code-154); //vitesse entre 20 et 100%
	}else{
		speed = -73*code-2045; //vitesse entre -20 et -100%
	}

//	chprintf((BaseSequentialStream *) &SD3, "vitesse = %d\n", speed);

	return speed;
}
