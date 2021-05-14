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

uint8_t get_punky_state(void);
void mode_selection_thd_start(void);

#endif /* MODE_SELECTION_H */
