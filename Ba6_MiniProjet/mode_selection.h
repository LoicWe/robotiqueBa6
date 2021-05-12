#ifndef MODE_SELECTION_H
#define MODE_SELECTION_H

enum PUNKY_STATE{
	PUNKY_DEMO = 0,
	PUNKY_DEBUG,
	PUNKY_SLEEP,
};

enum CHANGE_STATE{
	STAY = 0,
	RIGHT,
	LEFT
};

void mode_selection_thd_start(void);
void set_punky_state(uint8_t new_punky_state);
uint8_t get_punky_state(void);

#endif /* MODE_SELECTION_H */
