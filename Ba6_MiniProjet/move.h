#ifndef MOVE_H
#define MOVE_H

#define ERROR_THRESHOLD			14
#define GOAL_DISTANCE 			150
#define MAX_DISTANCE_DETECTED	200
#define MIN_DISTANCE_DETECTED	50
#define KP						50
#define KI 						16	//must not be zero
#define MAX_SUM_ERROR 			(MOTOR_SPEED_LIMIT/KI)

// Fonction to control the robot with the sound
void move(uint16_t rotation);
void move_stop(void);

#define MAX_SPEED 	900	//100%
#define MIN_SPEED	200		//20%

// Set a new speed
void set_speed(int16_t new_speed);

int16_t convert_speed(uint8_t code);


void activate_motors(void);
void deactivate_motors(void);

int16_t pi_regulator(uint16_t distance, uint8_t goal);

//start the PI regulator thread
void pi_regulator_init(void);

void pi_regulator_start(void);		//TODO: public func
void pi_regulator_stop(void);

#endif /* MOVE_H */
