#ifndef LED_ANIMATION_H
#define LED_ANIMATION_H


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

void leds_animations_thd_start(void);
void anim_barcode(void);
void anim_start_freq_manual(uint8_t intensity);
void anim_stop_freq_manual(uint8_t intensity);
void anim_stop_freq(void);
void anim_debug(void);
void anim_sleep(void);
void anim_wake_up(void);
void anim_clear_debug(void);

#endif /* LED_ANIMATION_H  */
