#ifndef MOVE_H
#define MOVE_H

// Fonction to control the robot with the sound
void move(float rotation);
void move_stop(void);

// Set a new speed
void set_speed(float new_speed);

void activate_motors(void);
void deactivate_motors(void);

#endif /* MOVE_H */
