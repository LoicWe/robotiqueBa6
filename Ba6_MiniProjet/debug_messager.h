#ifndef DEBUG_MESSAGER_H
#define DEBUG_MESSAGER_H

#define MAX_LENGTH_STRING 	20

enum msg_priority{
	LOW_PRIO = 0,
	HIGH_PRIO = 1
};

enum msg_duration{	// in milliseconds
	LIGHTNING = 1,
	WINK = 50,
	READABLE = 300,
	EMPHASIS = 1000,
	GRAIL = 2000,
};

void debug_messager_thd_start(void);
void debug_message(char *str_p, uint32_t time_p, bool high_prio_p);
void debug_message_1(char *str_p, int16_t value1_p, uint32_t time_p, bool high_prio_p);
void debug_message_4(char *str_p, int16_t value1_p, int16_t value2_p, int16_t value3_p, int16_t value4_p, uint32_t time_p, bool high_prio_p);

#endif /* DEBUG_MESSAGER_H  */
