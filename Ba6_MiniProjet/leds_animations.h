#ifndef LEDS_ANIMATIONS_H
#define LEDS_ANIMATIONS_H


enum animation{
	ANIM_CLEAR,
	ANIM_CLEAR_DEBUG,
	ANIM_BARCODE,
	ANIM_DEBUG,
	ANIM_SLEEP,
	ANIM_WAKE_UP,
	ANIM_FREQ_MANUAL,
	ANIM_FREQ,
};

enum direction{
	ANIM_FORWARD,
	ANIM_BACKWARD,
};

#define TIME_DEBUG 50

void leds_animations_thd_start(void);
void anim_barcode(uint8_t direction_p);
void anim_start_freq_manual(uint8_t intensity);
void anim_stop_freq_manual(uint8_t intensity);
void anim_stop_freq(void);
void anim_debug(void);
void anim_sleep(void);
void anim_wake_up(void);
void anim_clear_debug(void);
void anim_clear_rgbs(void);

#endif /* LEDS_ANIMATIONS_H  */
