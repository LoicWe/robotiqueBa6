#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

typedef enum {
	PUNKY_PLAY = 0,
	PUNKY_SLEEP,
	PUNKY_WAKE_UP
}PUNKY_STATE;

void init_potentiometer(void);

void set_punky_state(uint8_t new_punky_state);
uint8_t get_punky_state(void);

#endif /* POTENTIOMETER_H */
