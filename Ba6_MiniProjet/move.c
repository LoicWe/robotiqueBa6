#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>
#include <sensors/VL53L0X/VL53L0X.h>

#include <main.h>
#include <motors.h>
#include <move.h>
#include <leds.h>

static int16_t speed = 600;
static bool move_on = true;
static bool sleep_mode = false;


// *************************************************************************//
// ************* fonction en mode détection de fréquences ******************//
// *************************************************************************//

void move(int16_t rotation) {
	if (move_on) {
		left_motor_set_speed(speed + rotation);
		right_motor_set_speed(speed - rotation);
	}
}

void move_stop(void) {
	left_motor_set_speed(0);
	right_motor_set_speed(0);
}

void set_speed(int16_t new_speed) {
	speed = new_speed;
}

int16_t convert_speed(uint8_t code){

	int16_t speed = 0;

	if(code > 25){
		speed = (MAX_SPEED - MIN_SPEED)/13*code+3*MIN_SPEED-2*MAX_SPEED; //vitesse entre 20 et 100%
	}else{
		speed = -( (-MAX_SPEED + MIN_SPEED)/12*code+2*MAX_SPEED-MIN_SPEED); //vitesse entre -20 et -100%
	}
	return speed;
}

void motor_control_start(void){
	move_on = true;
}

void motor_control_stop(void) {
	move_on = false;
}



// ************************************************************************//
// ************* fonction en mode décection de codebarre ******************//
// ************************************************************************//

//Régulateur PI afin d'approcher un code barre
int16_t pi_regulator(uint16_t distance, uint8_t goal) {
	int16_t error = 0;
	int16_t speed = 0;
//	systime_t time;


	static int16_t sum_error = 0;

	error = distance - goal;

	//disables the PI regulator if the error is to small
	//this avoids to always move as we cannot exactly be where we want and
	//the camera is a bit noisy

//	time = chVTGetSystemTime();
	if (fabs(error) < ERROR_THRESHOLD) {
		return 0;
	}
//	chprintf((BaseSequentialStream *) &SD3, "temps = %d \r", chVTGetSystemTime - time);


	sum_error += error;

	//we set a maximum and a minimum for the sum to avoid an uncontrolled growth
	if (sum_error > MAX_SUM_ERROR) {
		sum_error = MAX_SUM_ERROR;
	} else if (sum_error < -MAX_SUM_ERROR) {
		sum_error = -MAX_SUM_ERROR;
	}

	speed = (KP * error + KI * sum_error)/4;

	return (int16_t) speed;
}


static THD_WORKING_AREA(waPiRegulator, 256);
static THD_FUNCTION(PiRegulator, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void) arg;

	systime_t time = 0;
	uint16_t distance;
	int16_t speed = 0;

	while (1) {

		if (!sleep_mode) {

			time = chVTGetSystemTime();
//			chprintf((BaseSequentialStream *) &SD3, "temps = %d \r", time);
			distance = VL53L0X_get_dist_mm();

			//if distance is too big we remarked some problem with the sensors so we take of too big and too small values
			distance = distance > MAX_DISTANCE_DETECTED ? GOAL_DISTANCE : distance;
			distance = distance < MIN_DISTANCE_DETECTED ? GOAL_DISTANCE : distance;

			//computes the speed to give to the motors
			//distance_cm is modified by the image processing thread
			speed = pi_regulator(distance, GOAL_DISTANCE);

			//applies the speed from the PI regulator
			right_motor_set_speed(speed);
			left_motor_set_speed(speed);

			//100Hz plus mainteant !!!!!
			time = chVTGetSystemTime();
//			chprintf((BaseSequentialStream *) &SD3, "temps 2 = %d \r", time);
//			chprintf((BaseSequentialStream *) &SD3, "temps = %d \r", (uint32_t) (chVTGetSystemTime() - time));

			chThdSleepUntilWindowed(time, time + MS2ST(50));		//TODO: test 20, 30, 50 ms;
		}else{
			chThdSleepMilliseconds(500);
		}

	}
}

void pi_regulator_init(void) {
	chThdCreateStatic(waPiRegulator, sizeof(waPiRegulator), NORMALPRIO+2, PiRegulator, NULL);
}

void pi_regulator_start(void) {
	sleep_mode = false;
}

void pi_regulator_stop(void){
	sleep_mode = true;
}
