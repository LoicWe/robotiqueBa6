#ifndef MOVE_H
#define MOVE_H

#define ERROR_THRESHOLD			0.1f
#define GOAL_DISTANCE 			10.0f
#define KP						800.0f
#define KI 						3.5f	//must not be zero
#define MAX_SUM_ERROR 			(MOTOR_SPEED_LIMIT/KI)

// Fonction to control the robot with the sound
void move(uint16_t rotation);
void move_stop(void);

// Set a new speed
void set_speed(int16_t new_speed);

void activate_motors(void);
void deactivate_motors(void);

//start the PI regulator thread
void init_pi_regulator(void);

void start_pi_regulator(void);		//TODO: public func
void stop_pi_regulator(void);

#endif /* MOVE_H */
