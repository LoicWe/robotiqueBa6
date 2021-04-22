#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <main.h>
#include <motors.h>
#include <move.h>

static float speed = 600;
static bool move_on = true;

void move(float rotation) {
	if (move_on) {
		left_motor_set_speed(speed - rotation);
		right_motor_set_speed(speed + rotation);
	} else {
		move_stop();
	}
}

void move_stop(void) {
	left_motor_set_speed(0);
	right_motor_set_speed(0);
}

void set_speed(float new_speed) {
	speed = new_speed;
}

void activate_motors(void){
	move_on = true;
}

void deactivate_motors(void){
	move_on = false;
}
