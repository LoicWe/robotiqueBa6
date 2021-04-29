#ifndef MOVE_H
#define MOVE_H

#define ERROR_THRESHOLD			10
#define GOAL_DISTANCE 			150
#define MAX_DISTANCE_DETECTED	350
#define MIN_DISTANCE_DETECTED	50
#define KP						40 //50
#define KI 						20 //16	//must not be zero
#define MAX_SUM_ERROR 			(MOTOR_SPEED_LIMIT/KI)

// Fonction to control the robot with the sound
void move(void);
void move_stop(void);

#define MAX_SPEED 	900	//100%
#define MIN_SPEED	200	//20%


// ************************************************************************//
// ************* fonction en mode décection de codebarre ******************//
// ************************************************************************//
void set_speed(int16_t new_speed);
int16_t get_speed(void);
void set_rotation(int16_t new_rotation);
int16_t convert_speed(uint8_t code);
void motor_control_run(void);
void motor_control_stop(void);


// ************************************************************************//
// ************* fonction en mode décection de codebarre ******************//
// ************************************************************************//

//start the PI regulator thread
void pi_regulator_init(void);
void pi_regulator_run(void);
void pi_regulator_stop(void);
int16_t pi_regulator(uint16_t distance, uint8_t goal);


#endif /* MOVE_H */
