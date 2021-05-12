#ifndef MOVE_H
#define MOVE_H

#define ERROR_THRESHOLD			10		// +/- 1cm width without moving
#define GOAL_DISTANCE 			150		// distance to read barcode idealy
#define MAX_DISTANCE_DETECTED	350		// after 35cm, no detection
#define MIN_DISTANCE_DETECTED	50		// under 5cm, no detection (ToF problems with close souce)
#define KP						40		// PI parameter
#define KI 						20		// PI parameter
#define MAX_SUM_ERROR 			(MOTOR_SPEED_LIMIT/KI)	// Anti windup limit

// Fonction to control the robot with the sound
void move(void);
void move_stop(void);

#define MAX_SPEED 					900		// max for motor : 1100
#define MIN_SPEED					110
#define BARCODE_ROTATION_SPEED 		60


// *************************************************************************//
// *************       Function for frequency mode        ******************//
// *************************************************************************//

void set_speed(int8_t code);
int16_t get_speed(void);
void set_rotation(int16_t new_rotation);
int16_t get_rotation(void);

// ************************************************************************//
// *************           Function for PI mode          ******************//
// ************************************************************************//

//start the PI regulator thread
void pi_regulator_thd_start(void);
void pi_regulator_start(void);
void pi_regulator_stop(void);
int16_t pi_regulator(uint16_t distance, uint8_t goal);


#endif /* MOVE_H */
