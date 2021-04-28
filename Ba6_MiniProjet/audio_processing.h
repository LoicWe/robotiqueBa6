#ifndef AUDIO_PROCESSING_H
#define AUDIO_PROCESSING_H

#define FFT_SIZE 	1024

typedef enum {
	//2 times FFT_SIZE because these arrays contain complex numbers (real + imaginary)
	BACK_CMPLX_INPUT = 0,
	//Arrays containing the computed magnitude of the complex numbers
	BACK_OUTPUT
} BUFFER_NAME_t;

typedef enum {
	SOUND_OFF = 0,
	ANALYSING,
	MOVING
}EARING_MODE;

void processAudioData(int16_t *data, uint16_t num_samples);

/*
*	put the invoking thread into sleep until it can process the audio datas
*/
void wait_send_to_computer(void);

/*
*	Returns the pointer to the BUFFER_NAME_t buffer asked
*/
float* get_audio_buffer_ptr(void);

void microphone_run(void);

void microphone_stop(void);

#endif /* AUDIO_PROCESSING_H */
