#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <sensors/VL53L0X/VL53L0X.h>
#include <chbsem.h>

#include <main.h>
#include <motors.h>
#include <move.h>
#include <mode_selection.h>
#include <communications.h>

static int16_t speed = 600;			// up to 1100, see motor files
static int16_t rotation = 0;		// coeff +/- to speed to rotate
static bool sleep_mode = false;		// stop the motors if SLEEP_MODE

// semaphores
static BSEMAPHORE_DECL(pi_regulator_start_sem, TRUE); // @suppress("Field cannot be resolved")

// *************************************************************************//
// *************       Function for frequency mode        ******************//
// *************************************************************************//

/*
 * 	@Describe:
 * 		Called by frequency mode to pilote the robot
 * 		Apply the speed and rotation set to the motors
 *
 */
void move(void) {
	left_motor_set_speed(speed + rotation);
	right_motor_set_speed(speed - rotation);
}

/*
 *  @Describe:
 *  	stop the motors
 */
void move_stop(void) {
	left_motor_set_speed(0);
	right_motor_set_speed(0);
}

/*
 * 	@Return:
 * 		the actual speed set
 */
int16_t get_speed(void){
	return speed;
}

/*
 * 	@Describe:
 * 		Converts a valide code into a speed for the robot.
 * 		Valide is from -14 to 13
 * 			-   -14 is max negative speed and -1 min negative speed
 * 			-   1 is min positive speed and 13 is max negative speed
 *
 * 	@Param:
 * 		int8_t code		a valide code from -14 to 13
 */
void set_speed(int8_t code) {

	code = code - 27;

	if (code > 0) {
		speed = (MAX_SPEED - MIN_SPEED) / 13 * code + MIN_SPEED;
	} else {
		speed = ((MAX_SPEED - MIN_SPEED) / 14 * code - MIN_SPEED);
	}

	if (get_punky_state() == PUNKY_DEBUG)
		debug_message_1("vitesse = ", speed, EMPHASIS, HIGH_PRIO);
}


// ************************************************************************//
// *************           Function for PI mode          ******************//
// ************************************************************************//

/*
 * 	@Param:
 * 		int16_t	 new_rotation		The rotation to set
 */
void set_rotation(int16_t new_rotation) {
	rotation = new_rotation;
}

/*
 * 	@Return:
 * 		the actual rotation set
 */
int16_t get_rotation(void){
	return rotation;
}

/*
 *  @Describe:
 *  	PI regulator to be at right distance for barcode
 *
 *	@Params:
 *		uint16_t distance	distance detected, in mm
 *		uint8_t goal		position to be reached
 *
 *	@Return:
 *		int16_t speed		the speed adapted for the robot
 *
 */
int16_t pi_regulator(uint16_t distance, uint8_t goal) {

	int16_t error = 0;
	int16_t speed = 0;

	static int16_t sum_error = 0;

	error = distance - goal;

	//disables the PI regulator if the error is to small
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

	// divided by 4 to adjust to the motor max speed
	speed = (KP * error + KI * sum_error) / 4;

	return (int16_t) speed;
}

/*
 * 	@Describe:
 * 		Proportionnal Integral regulator thread to approach the barcode at the right distance.
 *
 * 	@Author:
 * 		TP4_CamReg_correction, adapted for our needs
 */
static THD_WORKING_AREA(waPiRegulator, 256);
static THD_FUNCTION(PiRegulator, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void) arg;

	systime_t time1 = 0;
	uint16_t distance;
	int16_t speed = 0;

	while (1) {

		if (!sleep_mode) {

			time1 = chVTGetSystemTime();
			distance = VL53L0X_get_dist_mm();

			// PI only in the MIN-MAX range (MIN is to avoid short distance sensor problem)
			distance = distance > MAX_DISTANCE_DETECTED ? GOAL_DISTANCE : distance;
			distance = distance < MIN_DISTANCE_DETECTED ? GOAL_DISTANCE : distance;

			//computes the speed to give to the motors
			speed = pi_regulator(distance, GOAL_DISTANCE);

			// speed is given by ToF sensor, rotation by barcode analysis
			right_motor_set_speed(speed+rotation);
			left_motor_set_speed(speed-rotation);


			chThdSleepUntilWindowed(time1, time1 + MS2ST(50));
		}
		else{
			move_stop();
			chBSemWait(&pi_regulator_start_sem);
		}
	}
}

void pi_regulator_thd_start(void) {
	chThdCreateStatic(waPiRegulator, sizeof(waPiRegulator), NORMALPRIO + 2, PiRegulator, NULL);
}

/*
 * 	@Describe:
 * 		allow the PI regulator thread to execute it's code
 */
void pi_regulator_start(void) {

	if(sleep_mode){
		sleep_mode = false;
		chBSemSignal(&pi_regulator_start_sem);
	}
}

/*
 * 	@Describe:
 * 		prevent the PI regulator thread to execute it's code
 * 		--> ressources and time gain for the other threads
 */
void pi_regulator_stop(void) {
	sleep_mode = true;
}
