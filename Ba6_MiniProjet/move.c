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
