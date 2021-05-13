#ifndef AUDIO_PROCESSING_H
#define AUDIO_PROCESSING_H

#define FFT_SIZE 	1024

enum EARING_MODE{
	SOUND_OFF = 0,
	ANALYSING,
	MOVING
};

void processAudioData(int16_t *data, uint16_t num_samples);
void microphone_start(void);
void microphone_stop(void);

#endif /* AUDIO_PROCESSING_H */
