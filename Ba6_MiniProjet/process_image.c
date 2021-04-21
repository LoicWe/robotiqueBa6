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
#include "spi_comm.h"


static uint8_t suspended = 1;

//semaphore
static BSEMAPHORE_DECL(image_ready_sem, TRUE); // @suppress("Field cannot be resolved")
static BSEMAPHORE_DECL(start_imaging_sem, FALSE); // @suppress("Field cannot be resolved")

uint8_t extract_barcode(uint8_t *image) {

	int8_t digit[NB_LINE_BARCODE] = { -1, -1, -1, -1, -1 };
	uint8_t code = 0;
	uint8_t mean[3]; /* width divided into 3 segments because of auto brightness causing a n shape*/

	// déclaration d'une ligne
	struct Line line;
	line.end_pos = line.width = line.begin_pos = 0;
	line.found = false;
	uint8_t width = 0;
	uint16_t end_last_line = 0;
	uint8_t width_unit = 0;

	/***********************************
	 *  recherche du motif de démarrage
	 ***********************************/

	calculate_mean(image, mean);

	//***** FIRST LINE ligne*****
	do {
		line = line_find_next(image, end_last_line, mean[0]);
		end_last_line = line.end_pos;
		width = line.width;

		// si trouvée mais pas les bonnes dimensions, on recherche plus loin
		if (line.found && !(width > START_LINE_WIDTH - LINE_THRESHOLD && width < START_LINE_WIDTH + LINE_THRESHOLD)) {
			line.found = false;
		}
	} while (line.found == false && end_last_line != IMAGE_BUFFER_SIZE);

	//***** SECONDE LINE *****
	if (line.found) {
		line = line_find_next(image, end_last_line, mean[0]);

		// on vérifie la dimension et l'écart avec la première ligne
		if (line.found && (!(line.width > width - LINE_THRESHOLD && line.width < width + LINE_THRESHOLD) || !(line.begin_pos - end_last_line < width))) {
			line.found = false;
		} else {
			// base de comparaison des tailles des lignes codantes
			width_unit = (width + line.width) / 2;
		}
	}

	/***********************************
	 *  recherche des 5 lignes suivantes
	 ***********************************/

	if (line.found) {
		for (int i = 0; i < NB_LINE_BARCODE && line.found; i++) {
			line = line_find_next(image, line.end_pos, (i < 3 ? mean[1] : mean[2]));
			digit[i] = line_classify(line, width_unit);
			// si la ligne n'est pas reconnu, arrêt
			if (digit[i] == 0) {
				line.found = false;
				break;
			}
		}
	}

	/***********************************
	 *  vérification du schéma de clôture
	 ***********************************/

	if (line.found) {
		// avant dernière ligne = petite, dernière = moyenne
		if (digit[NB_LINE_BARCODE - 2] == SMALL && digit[NB_LINE_BARCODE - 1] == MEDIUM) {
			// assemblage des digits en base 3 -> 26 possibilités
			code = 9 * digit[0] + 3 * digit[1] + digit[2];
			barcode_validate();
		}
	}

	return code;
}

uint8_t line_classify(struct Line line, uint8_t width_unit) {

	uint8_t ratio = 0;
	uint8_t half_line = width_unit / 2;

	if (line.width + LINE_THRESHOLD > width_unit && line.width - LINE_THRESHOLD < width_unit) {
		ratio = MEDIUM;
	} else if (line.width + LINE_THRESHOLD > width_unit + half_line && line.width - LINE_THRESHOLD < width_unit + half_line) {
		ratio = LARGE;
	} else if (line.width + LINE_THRESHOLD > width_unit - half_line && line.width - LINE_THRESHOLD < width_unit - half_line) {
		ratio = SMALL;
	}

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
	}
	return line;
}

void calculate_mean(uint8_t *buffer, uint8_t *mean) {

	/*performs an average for each 3 segments third, because of auto brightness
	 * causing the values to shape a n. It gets dimmer on the sides
	 */

	uint16_t sum = 0;
	uint16_t i;

	for (uint8_t j = 0; j < 3; j++) {
		sum = 0;
		for (i = IMAGE_BUFFER_SIZE_DIV_3 * j; i < IMAGE_BUFFER_SIZE_DIV_3 * (j + 1); i++) {
			sum += buffer[i];
		}
		mean[j] = sum / IMAGE_BUFFER_SIZE_DIV_3;

	}
}

void demo_led(uint8_t code) {

	switch (code) {
	case 14:
		set_rgb_led(LED2, 10,0,0);
//		toggle_rgb_led(LED2, RED_LED, 100);
		break;
	case 16:
		set_led(LED3, 2);
		break;
	case 21:
		set_rgb_led(LED4, 10,0,0);
//		toggle_rgb_led(LED4, RED_LED, 100);
		break;
	case 24:
		set_rgb_led(LED6, 10,0,0);
//		toggle_rgb_led(LED6, RED_LED, 100);
		break;
	case 32:
		set_led(LED7, 2);
		break;
	case 33:
		set_rgb_led(LED8, 10,0,0);
//		toggle_rgb_led(LED8, RED_LED, 100);
		break;
	default:
		break;
	}
	chThdSleepMilliseconds(500);

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
		if (code != 0) {
			chprintf((BaseSequentialStream *) &SD3, "code = %d\n", code);
			demo_led(code);
		}

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

void process_image_start(void) {
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);

}
