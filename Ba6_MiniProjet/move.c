#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>
#include <sensors/VL53L0X/VL53L0X.h>

#include <main.h>
#include <motors.h>
#include <move.h>
#include <led_animation.h>
#include <potentiometer.h>
#include <spi_comm.h>

static int16_t speed = 600;
static int16_t rotation = 0;
static bool move_on = true;		// enable or disable the control of motor when frequency mode
static bool sleep_mode = false;	// stop the motors if SLEEP_MODE


// *************************************************************************//
// ************* fonction en mode détection de fréquences ******************//
// *************************************************************************//


// called by frequency mode (cannot be stopped)
void move() {
	// to stop the callback when in PI mode
	if (move_on) {
		left_motor_set_speed(speed + rotation);
		right_motor_set_speed(speed - rotation);
	}
}

void move_stop(void) {
	left_motor_set_speed(0);
	right_motor_set_speed(0);
}

int16_t get_speed(void){
	return speed;
}

void set_rotation(int16_t new_rotation) {
	rotation = new_rotation;
}

void set_speed(int8_t code) {
	code = code - 26;

	// code from -14 to 13.
	// -14 is max negative speed and -1 min negative speed
	// 1 is min positive speed and 13 is max negative speed
	if (code > 0) {
		anim_forward();
		speed = (MAX_SPEED - MIN_SPEED) / 13 * code + MIN_SPEED;
	} else {
		anim_backward();
		speed = ((MAX_SPEED - MIN_SPEED) / 13 * code - MIN_SPEED);
	}

	if (get_punky_state() == PUNKY_DEBUG)
		chprintf((BaseSequentialStream *) &SD3, "vitesse effective = %d\r", speed);

}

void motor_control_run(void) {
	move_on = true;
}

void motor_control_stop(void) {
	move_stop();
	move_on = false;
	move_stop();
}


// ************************************************************************//
// ************* fonction en mode décection de codebarre ******************//
// ************************************************************************//


/* PI regulator to be at right distance for barcode
 *
 */
int16_t pi_regulator(uint16_t distance, uint8_t goal) {
	int16_t error = 0;
	int16_t speed = 0;

	static int16_t sum_error = 0;

	error = distance - goal;

	//disables the PI regulator if the error is to small
	//this avoids to always move as we cannot exactly be where we want and
	//the camera is a bit noisy


	if (fabs(error) < ERROR_THRESHOLD) {
		return 0;
	}

	sum_error += error;

	//we set a maximum and a minimum for the sum to avoid an uncontrolled growth
	if (sum_error > MAX_SUM_ERROR) {
		sum_error = MAX_SUM_ERROR;
	} else if (sum_error < -MAX_SUM_ERROR) {
		sum_error = -MAX_SUM_ERROR;
	}

	speed = (KP * error + KI * sum_error) / 4;

	return (int16_t) speed;
}

static THD_WORKING_AREA(waPiRegulator, 256);
static THD_FUNCTION(PiRegulator, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void) arg;

	systime_t time1 = 0;
	bool pi_stop_first_time = false;
	uint16_t distance;
	int16_t speed = 0;

	while (1) {

		if (!sleep_mode) {

			time1 = chVTGetSystemTime();
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
			pi_stop_first_time = true;

			//100Hz plus mainteant !!!!!
			chThdSleepUntilWindowed(time1, time1 + MS2ST(50));		//TODO: test 20, 30, 50 ms;
		}else{
			if(pi_stop_first_time == true){
				right_motor_set_speed(0);
				left_motor_set_speed(0);
				pi_stop_first_time = false;
			}
			chThdSleepMilliseconds(500);
		}

	}
}

void pi_regulator_init(void) {
	chThdCreateStatic(waPiRegulator, sizeof(waPiRegulator), NORMALPRIO + 2, PiRegulator, NULL);
}

void pi_regulator_run(void) {
	sleep_mode = false;
}

void pi_regulator_stop(void) {
	sleep_mode = true;
}
