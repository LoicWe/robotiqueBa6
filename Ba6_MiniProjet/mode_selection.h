#ifndef MODE_SELECTION_H
#define MODE_SELECTION_H

typedef enum {
	PUNKY_DEMO = 0,
	PUNKY_DEBUG,
	PUNKY_SLEEP,
	PUNKY_WAKE_UP
}PUNKY_STATE;

typedef enum {
	STAY = 0,
	RIGHT,
	LEFT
}CHANGE_STATE;

void mode_selection_thd_init(void);

void set_punky_state(uint8_t new_punky_state);
uint8_t get_punky_state(void);

#endif /* MODE_SELECTION_H */
