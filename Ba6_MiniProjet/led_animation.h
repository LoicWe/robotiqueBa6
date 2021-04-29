#ifndef LED_ANIMATION_H
#define LED_ANIMATION_H


enum animation{
	ANIM_CLEAR,
	ANIM_BARCODE,
	ANIM_SLEEP,
	ANIM_WAKE_UP,
	ANIM_FREQ,
	ANIM_FORWARD,
	ANIM_BACKWARD
};

void body_led_thd_start(void);
void anim_barcode(void);
void anim_start_freq(uint8_t intensity);
void anim_stop_freq(uint8_t intensity);
void anim_sleep(void);
void anim_wake_up(void);
void anim_clear(void);

#endif /* LED_ANIMATION_H  */
