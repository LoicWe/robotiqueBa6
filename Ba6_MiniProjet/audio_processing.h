#ifndef AUDIO_PROCESSING_H
#define AUDIO_PROCESSING_H

#define FFT_SIZE 	1024
#define MIN_VALUE_THRESHOLD		15000;	// value to take frequence into account
#define MIN_FREQ				10		// lowest frequency human voice can reach easily
#define MAX_FREQ				45		// highest frequency humain voice can reach easily
#define MIN_FREQ_INIT			17		// lowest frequency to start piloting
#define MAX_FREQ_INIT			30		// highest frequency to start piloting
#define FREQ_THRESHOLD_FORWARD	1		// threshold before rotating
#define NB_SOUND_ON				20		// nbr samples to get for the mean before running
#define NB_SOUND_OFF			15		// nbr sample to reset the mean
#define ROTATION_COEFF			12		// how strong the rotation is

enum EARING_MODE{
	SOUND_OFF = 0,
	ANALYSING,
	MOVING
};

void processAudioData(int16_t *data, uint16_t num_samples);
void microphone_start(void);
void microphone_stop(void);

#endif /* AUDIO_PROCESSING_H */
