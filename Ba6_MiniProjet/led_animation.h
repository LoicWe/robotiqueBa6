#ifndef BODY_LED_THD_H
#define BODY_LED_THD_H



enum animation{
	ANIM_CLEAR,
	ANIM_BARCODE,
	ANIM_SLEEP,
	ANIM_WAKE_UP,
	ANIM_START_FREQ,
	ANIM_STOP_FREQ
};

void body_led_thd_start(void);
void anim_barcode(void);
void anim_start_freq(void);
void anim_stop_freq(void);
void anim_sleep(void);
void anim_wake_up(void);
void anim_clear(void);

#endif /* BODY_LED_THD_H  */
