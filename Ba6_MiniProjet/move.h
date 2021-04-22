#ifndef MOVE_H
#define MOVE_H

// Fonction to control the robot with the sound
void move(float rotation);
void move_stop(void);

#define MAX_SPEED 	1100	//100%
#define MIN_SPEED	220		//20%

// Set a new speed
void set_speed(float new_speed);
int16_t convert_speed(uint8_t code);

void activate_motors(void);
void deactivate_motors(void);

#endif /* MOVE_H */
