#ifndef AUDIO_PROCESSING_H
#define AUDIO_PROCESSING_H

#define FFT_SIZE 	1024

typedef enum {
	//2 times FFT_SIZE because these arrays contain complex numbers (real + imaginary)
	BACK_CMPLX_INPUT = 0,
	//Arrays containing the computed magnitude of the complex numbers
	BACK_OUTPUT
}BUFFER_NAME_t;

typedef enum {
	SOUND_OFF = 0,
	ANALYSING,
	MOVING
}EARING_MODE;

void processAudioData(int16_t *data, uint16_t num_samples);
void microphone_start(void);
void microphone_stop(void);
bool get_microphone_state(void);

#endif /* AUDIO_PROCESSING_H */
