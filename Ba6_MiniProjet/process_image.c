#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <camera/po8030.h>
#include "camera/dcmi_camera.h"
#include "msgbus/messagebus.h"
#include "parameter/parameter.h"
#include <process_image.h>
#include <communications.h>
#include <body_led_thd.h>
#include <leds.h>

static float distance_cm = 0;
static uint8_t suspended = 1;
static uint16_t line_position = IMAGE_BUFFER_SIZE / 2;	//middle
//static uint8_t code = 0;

//semaphore
static BSEMAPHORE_DECL(image_ready_sem, TRUE); // @suppress("Field cannot be resolved")
static BSEMAPHORE_DECL(start_imaging_sem, FALSE); // @suppress("Field cannot be resolved")

uint8_t extract_barcode(uint8_t *image) {

	uint8_t digit[3] = { 0, 0, 0 };
	uint8_t code = 0;
	uint8_t mean[3]; /* width divided into 3 segments because of auto brightness causing a n shape*/

	// déclaration d'une ligne
	struct Line line;
	struct Line line2;
	line.end_pos = line.width = 0;
	line.found = false;
	uint8_t interline = 0;
	uint8_t validate_step = 0;
	uint8_t width_unit = 0;

	calculate_mean(image, mean);

	uint8_t width = 0;
	uint16_t end_last_line = 0;

	/*** recherche du motif de démarrage ***/
	// première ligne, on cherche la bonne taille
	do {
		line = line_find_next(image, end_last_line, mean[0]);
		end_last_line = line.end_pos;
		width = line.width;

		// si trouvée mais pas les bonnes dimensions, on recommence jusqu'à la fin du buffer
		if (line.found && !(width > START_LINE_WIDTH - LINE_THRESHOLD && width < START_LINE_WIDTH + LINE_THRESHOLD)) {
			line.found = false;
		}

	} while (line.found == false && end_last_line != IMAGE_BUFFER_SIZE);
//	chprintf((BaseSequentialStream *) &SD3, "width = %d  ", width);

	// seconde ligne, on cherche également la bonne taille
	if (line.found) {
		line = line_find_next(image, end_last_line, mean[0]);
	}

	if (line.found) {
		interline = line.begin_pos - end_last_line;
		width = (width + line.width + interline) / 3;
		chprintf((BaseSequentialStream *) &SD3, "%d %d %d    %d %d   %d | ", width, interline, line.width, end_last_line, line.begin_pos, mean[0]);
	}

//	chprintf((BaseSequentialStream *) &SD3, "!!!!!");

	/*if (line.found) {
	 line2.begin_pos = line2.end_pos = line.end_pos;
	 line2.width = 0;
	 line2.found = false;
	 line2 = line_find_next(image, line2, mean);
	 }
	 if (line2.found) {
	 interline = line2.begin_pos - line.end_pos + LINE_THRESHOLD;
	 if (line.width + LINE_THRESHOLD > START_LINE_WIDTH && line.width - LINE_THRESHOLD < START_LINE_WIDTH && line2.width + LINE_THRESHOLD > START_LINE_WIDTH
	 && line2.width - LINE_THRESHOLD < START_LINE_WIDTH && interline + LINE_THRESHOLD > START_LINE_WIDTH && interline - LINE_THRESHOLD < START_LINE_WIDTH) {

	 validate_step = 1;
	 }
	 }

	 width_unit = (line.width + line2.width) / 2;
	 chprintf((BaseSequentialStream *) &SD3, "%d %d %d\n", line.width, line2.width, width_unit);
	 */
	/*

	 // cherche le code situé dans les 3 lignes suivantes
	 int j = 0;
	 while (validate_step && j < 3) {
	 validate_step = 0;
	 line2 = line_find_next(image, line2, mean);
	 validate_step = line2.found;
	 if (validate_step) {
	 digit[j] = line_classify(line2, width_unit);
	 }
	 j++;
	 }

	 // cherche le motif de fin pour valider
	 validate_step = 0;
	 line2 = line_find_next(image, line2, mean);
	 validate_step = line2.found;
	 if (validate_step && line_classify(line2, width_unit) == SMALL) {
	 validate_step = 0;
	 line2 = line_find_next(image, line2, mean);
	 validate_step = line2.found;
	 }
	 if (validate_step && line_classify(line2, width_unit) == MEDIUM) {
	 validate_step = 1;
	 } else {
	 validate_step = 0;
	 }

	 if (validate_step) {
	 barcode_validate();
	 code = digit[0] + digit[1] * 4 + digit[2] * 16;
	 } else {
	 code = 0;
	 }

	 //	chprintf((BaseSequentialStream *) &SD3, "moyenne = %d\n", mean);
	 //	chprintf((BaseSequentialStream *) &SD3, "largeur ligne 1 = %d  et  départ1 = %d\n", line.width, line.begin_pos);
	 //	chprintf((BaseSequentialStream *) &SD3, "largeur ligne 2 = %d  et  départ2 = %d\n", line2.width, line2.begin_pos);
	 //	chprintf((BaseSequentialStream *) &SD3, "largeur interli = %d\n", interline);
	 chprintf((BaseSequentialStream *) &SD3, "code = %d %d %d\n", digit[0], digit[1], digit[2]);
	 */

	return code;
}

uint8_t line_classify(struct Line line, uint8_t width_unit) {

	uint8_t ratio = 0;
	if (line.width + LINE_THRESHOLD > width_unit && line.width - LINE_THRESHOLD < width_unit) {
		ratio = MEDIUM;
	} else if (line.width + LINE_THRESHOLD > width_unit * 2 && line.width - LINE_THRESHOLD < width_unit * 2) {
		ratio = LARGE;
	} else if (line.width + LINE_THRESHOLD > width_unit / 2 && line.width - LINE_THRESHOLD < width_unit / 2) {
		ratio = SMALL;
	}
//	uint8_t ratio = ((n + d/2)/d);

	return ratio;
}

/*
 *  Returns the line's width extracted from the image buffer given
 *  Returns 0 if line not found
 */
struct Line line_find_next(uint8_t *buffer, uint16_t start_position, uint32_t mean) {

	uint16_t i = start_position, begin = start_position, end = start_position;
	uint8_t stop = 0, line_not_found = 0, wrong_line = 0;
	struct Line line;

	do {

		//search for a begin
		while (stop == 0 && i < (IMAGE_BUFFER_SIZE - WIDTH_SLOPE)) {
			//the slope must at least be WIDTH_SLOPE wide and is compared
			//to the mean of the image
			if (buffer[i] > mean && buffer[i + WIDTH_SLOPE] < mean) {
				begin = i;
				stop = 1;
			}
			i++;
		}
		//if a begin was found, search for an end
		if (i < (IMAGE_BUFFER_SIZE - WIDTH_SLOPE) && begin) {
			stop = 0;

			while (stop == 0 && i < IMAGE_BUFFER_SIZE) {
				if (buffer[i] > mean && buffer[i - WIDTH_SLOPE] < mean) {
					end = i;
					stop = 1;
				}
				i++;
			}
			//if an end was not found
			if (i > IMAGE_BUFFER_SIZE || !end) {
				line_not_found = 1;
			}
		} else		    //if no begin was found
		{
			line_not_found = 1;
		}

		//if a line too small has been detected, continues the search
		if (!line_not_found && (end - begin) < MIN_LINE_WIDTH) {
			i = end;
			begin = 0;
			end = 0;
			stop = 0;
			wrong_line = 1;
		} else {
			wrong_line = 0;
		}
	} while (wrong_line);

	if (line_not_found) {
		line.found = false;
		line.end_pos = IMAGE_BUFFER_SIZE;
	} else {
		line.width = (end - begin);
		line.end_pos = end;
		line.begin_pos = begin;
		line.found = true;
		line_position = (begin + end) / 2; //gives the line position.
	}
	return line;
}

void calculate_mean(uint8_t *buffer, uint8_t *mean) {

	//performs an average per 3 segments, because of auto brightness

	uint16_t sum = 0;
	uint16_t i;

	for (uint8_t j = 0; j < 3; j++) {
		sum = 0;
		for (i = IMAGE_BUFFER_SIZE_DIV_3 * j; i < IMAGE_BUFFER_SIZE_DIV_3 * (j + 1); i++) {
			sum += buffer[i];

		}
		mean[j] = sum / IMAGE_BUFFER_SIZE_DIV_3;
//		chprintf((BaseSequentialStream *) &SD3, "mean = %d  ", mean[j]);

	}
}

static THD_WORKING_AREA(waCaptureImage, 256);
static THD_FUNCTION(CaptureImage, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void) arg;

	//Takes pixels 0 to IMAGE_BUFFER_SIZE of the line 250 + 251 (minimum 2 lines because reasons)
	po8030_advanced_config(FORMAT_RGB565, 0, 250, IMAGE_BUFFER_SIZE, 2, SUBSAMPLING_X1, SUBSAMPLING_X1);
	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);
	dcmi_prepare();

	while (1) {
		if (!suspended) {
			//starts a capture
			dcmi_capture_start();
			//waits for the capture to be done
			wait_image_ready();
			//signals an image has been captured
			chBSemSignal(&image_ready_sem);
		} else {
			chBSemWait(&start_imaging_sem);
		}
	}
}

static THD_WORKING_AREA(waProcessImage, 1024);
static THD_FUNCTION(ProcessImage, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void) arg;

	uint8_t *img_buff_ptr;
	uint8_t image[IMAGE_BUFFER_SIZE] = { 0 };
	uint16_t lineWidth = 0;
	uint8_t code = 0;

	uint8_t send_to_computer = 0;

	while (1) {
		//waits until an image has been captured
		chBSemWait(&image_ready_sem);
		//gets the pointer to the array filled with the last image in RGB565
		img_buff_ptr = dcmi_get_last_image_ptr();

		//Extracts only the red pixels
		for (uint16_t i = 0; i < (2 * IMAGE_BUFFER_SIZE); i += 2) {
			//extracts first 5bits of the first byte
			//takes nothing from the second byte
			image[i / 2] = (uint8_t) img_buff_ptr[i] & 0xF8;
		}

		//search for a line in the image and gets its width in pixels
		code = extract_barcode(image);

		if (send_to_computer == 20) {
			send_to_computer = 0;
			//sends to the computer the image
			SendUint8ToComputer(image, IMAGE_BUFFER_SIZE);
		}
//invert the bool
		send_to_computer++;
	}
}

void get_images(void) {
	suspended = 0;
	chBSemSignal(&start_imaging_sem);
}

void stop_images(void) {
	suspended = 1;
}

uint16_t get_line_position(void) {
	return line_position;
}

void process_image_start(void) {
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);

}
