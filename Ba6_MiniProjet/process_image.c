#include "ch.h"
#include "hal.h"
#include <usbcfg.h>
#include <camera/po8030.h>
#include "camera/dcmi_camera.h"
#include "msgbus/messagebus.h"
#include "parameter/parameter.h"

#include <process_image.h>
#include <communications.h>
#include <mode_selection.h>
#include <leds_animations.h>

static uint8_t code = 0;
static bool barcode_found = false;		// to block code change before reading

static bool sleep_mode = 1;

// semaphores
static BSEMAPHORE_DECL(image_ready_sem, TRUE); // @suppress("Field cannot be resolved")
static BSEMAPHORE_DECL(start_imaging_sem, FALSE); // @suppress("Field cannot be resolved")

/*
 *  @Describe:
 *  	analyse a pixel line from an image to extract a barcode with
 *  	the following layout :
 *  		- 2 medium lines as starting pattern
 *  		- 3 coding lines with 3 sizes ; small, medium, large
 *  		- 1 small and 1 medium line as ending pattern
 *  	The function search the lines and set the code depending if
 *  	barcode is fund, just the start pattern or just the end pattern
 *  	The code is a static variable accessible all time in the file
 *  	Steps :
 *  		- Search for the start pattern
 *  		- Search for the 3 coding lines
 *  		- Validate the barcode with the end pattern
 *  		- If not validate, then search again in other direction the end pattern
 *
 *  @Param:
 *  	uint8_t *image	The pixel line to analyse. IMAGE_BUFFER_SIZE long
 *
 */
void extract_barcode(uint8_t *image) {

	// save line width and allow keeping track of barcode decryption state
	int8_t digit[NB_LINE_BARCODE] = { -1, -1, -1, -1, -1, -1, -1 };
	// luminosity mean, divided into 3 segments
	uint8_t mean[3];

	// a line
	struct Line line;
	line.end_pos = line.width = line.begin_pos = 0;
	line.found = false;
	// memories of previous line properties
	uint16_t width = 0;
	uint16_t end_last_line = 0;
	uint8_t width_unit = 0;

	/*********************************************************
	 *****  search for start pattern (2 medium lines)    *****
	 *********************************************************/

	calculate_mean(image, mean);

	//***** FIRST LINE *****
	do {
		line = line_find_next(image, end_last_line, mean);
		end_last_line = line.end_pos;
		width = line.width;

		// if a line is found but not with the right dimensions, start over
		if (line.found && !(width > START_LINE_WIDTH - LINE_THRESHOLD && width < START_LINE_WIDTH + LINE_THRESHOLD)) {
			line.found = false;
		}
	} while (line.found == false && end_last_line != IMAGE_BUFFER_SIZE);

	//***** SECONDE LINE *****
	if (line.found) {
		// validate first line for tracking purpose
		digit[0] = 2;

		line = line_find_next(image, end_last_line, mean);

		// check line dimension (hardcoded at around 12 cm) and gap with first line
		if (line.found && (!(line.width > width - LINE_THRESHOLD && line.width < width + LINE_THRESHOLD) || !(line.begin_pos - end_last_line < width))) {
			line.found = false;
		} else {
			// validate line for tracking purpose
			digit[1] = 2;
			// the two line mean is the base for the other lines (width_unit = medium size)
			width_unit = (uint8_t) (width + line.width) / 2;
			end_last_line = line.end_pos;
		}
	}

	/************************************************************
	 *  Search the 5 next lines (3 coding and last 2 checking)  *
	 ************************************************************/

	if (line.found) {
		for (int i = 2; i < NB_LINE_BARCODE && line.found; i++) {
			line = line_find_next(image, line.end_pos, mean);
			digit[i] = line_classify(line, width_unit);
			end_last_line = line.end_pos;
			// if a line doesn't have a recognized size, stop
			if (digit[i] == -1 || !(line.begin_pos - end_last_line < width_unit)) {
				line.found = false;
				break;
			}
		}
	}

	/*************************************
	 ******        SET CODE         ******
	 *************************************/

	// if end pattern is SMALL then MEDIUM, barcode was successfuly read
	if (digit[NB_LINE_BARCODE - 2] == SMALL && digit[NB_LINE_BARCODE - 1] == MEDIUM) {
		// code is created in base 3 -> 27 possibilities with 3 line of 3 sizes
		set_code(9 * digit[2] + 3 * digit[3] + digit[4]);
		if (get_punky_state() == PUNKY_DEBUG)
			debug_message_4("Code = ", digit[2], digit[3], digit[4], code, EMPHASIS, HIGH_PRIO);
	}
	// IF start pattern read but NOT the end pattern, set code to 1
	else if (digit[0] == MEDIUM && digit[1] == MEDIUM) {
		set_code(1);
	}
	// if start pattern NOT read, analyse the line in the other direction to found end pattern
	else {

		/************************************************
		 ******     END PATTERN SEARCH, INVERTED    *****
		 ************************************************/

		// same operations as before, but in the reverse direction
		line.end_pos = line.begin_pos = IMAGE_BUFFER_SIZE - 1;
		line.width = 0;
		line.found = false;

		width = 0;
		end_last_line = IMAGE_BUFFER_SIZE - 1;
		digit[NB_LINE_BARCODE - 2] = digit[NB_LINE_BARCODE - 1] = -1;

		/***** FIRST LINE *****/
		do {
			// analyse pixel line in reverse direction
			line = line_find_next_inverted_direction(image, end_last_line, mean);
			end_last_line = line.end_pos;
			width = line.width;

			// if a line is found but not with the right dimensions, start over
			if (line.found && !(width > START_LINE_WIDTH - LINE_THRESHOLD && width < START_LINE_WIDTH + LINE_THRESHOLD)) {
				line.found = false;
			}
		} while (line.found == false && end_last_line != 0);

		//***** SECONDE LINE *****
		if (line.found) {
			digit[NB_LINE_BARCODE - 1] = 2;

			line = line_find_next_inverted_direction(image, end_last_line, mean);

			// check line dimension (hardcoded at around 12 cm) and gap with first line
			if (line.found && (!(line.width > width / 2 - LINE_THRESHOLD && line.width < width / 2 + LINE_THRESHOLD) || !(abs(end_last_line - line.begin_pos) < width))) {
				line.found = false;
			} else {
				digit[NB_LINE_BARCODE - 2] = 1;
			}
		}
		if (digit[NB_LINE_BARCODE - 1] == 2 && digit[NB_LINE_BARCODE - 2] == 1) {
			// if end pattern is found
			set_code(2);
		} else {
			// if nothing was found
			set_code(0);
		}
	}
}

/*
 *  @Describe:
 *  	give a size to a new line, depending on a reference line
 *
 *  @Params:
 *  	struct Line line		The line to classify
 *  	uint8_t width_unit		The reference width, set as medium
 *
 *  @Return:
 *  	uint8_t ratio			The line size
 */
uint8_t line_classify(struct Line line, uint8_t width_unit) {

	uint8_t ratio = -1;
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
 *	@Describe:
 *  	Search for a line and it size in pixel, from 0 to image size
 *
 *  @Params:
 *  	uint8_t *buffer				a pixel line to analyse. IMAGE_BUFFER_SIZE long
 *  	uint16_t start_position		position in the buffer to start search
 *  	uint32_t *mean_p			luminosity means to find the line
 *
 *  @Return:
 *  	Returns the line (struct) extracted from the image buffer given
 *  	If not found, line.found = false, true otherwise
 *
 *	@Author:
 *		TP4_CamReg_correction, adapted for our needs
 *
 */
struct Line line_find_next(uint8_t *buffer, uint16_t start_position, uint8_t *mean_p) {

	uint16_t i = start_position, begin = start_position, end = start_position;
	uint8_t stop = 0, line_not_found = 0, wrong_line = 0;
	struct Line line;
	uint8_t mean = 0;

	do {

		//search for a begin
		while (stop == 0 && i < (IMAGE_BUFFER_SIZE - WIDTH_SLOPE)) {
			//the slope must at least be WIDTH_SLOPE wide and is compared
			//to the mean of the image
			mean = (i < IMAGE_BUFFER_SIZE_DIV_3 ? mean_p[0] : (i < IMAGE_BUFFER_SIZE_DIV_3 * 2 ? mean_p[1] - 20 : mean_p[2]));
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
				mean = (i < IMAGE_BUFFER_SIZE_DIV_3 ? mean_p[0] : (i < IMAGE_BUFFER_SIZE_DIV_3 * 2 ? mean_p[1] - 20 : mean_p[2]));
				if (buffer[i] > mean && buffer[i - WIDTH_SLOPE] < mean) {
					end = i;
					stop = 1;
				}
				i++;
			}
			//if an end was not found
			if (i == IMAGE_BUFFER_SIZE || !end) {
				line_not_found = 1;
			}
		}
		//if no begin was found
		else {
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

/*
 *	@Describe:
 *  	Search for a line and it size in pixel, from image size to 0
 *
 *  @Params:
 *  	uint8_t *buffer				a pixel line to analyse. IMAGE_BUFFER_SIZE long
 *  	uint16_t start_position		position in the buffer to start search
 *  	uint32_t *mean_p			luminosity means to find the line
 *
 *  @Return:
 *  	Returns the line (struct) extracted from the image buffer given
 *  	If not found, line.found = false, true otherwise
 *
 *	@Author:
 *		TP4_CamReg_correction, adapted for our needs, turned upside down
 *
 */
struct Line line_find_next_inverted_direction(uint8_t *buffer, int16_t start_position, uint8_t *mean_p) {

	int16_t i = start_position, begin = start_position, end = start_position;
	uint8_t stop = 0, line_not_found = 0, wrong_line = 0;
	struct Line line;
	uint8_t mean = 0;

	do {

		//search for a begin
		while (stop == 0 && i > WIDTH_SLOPE) {
			//the slope must at least be WIDTH_SLOPE wide and is compared to the mean of the image
			mean = (i < IMAGE_BUFFER_SIZE_DIV_3 ? mean_p[0] : (i < IMAGE_BUFFER_SIZE_DIV_3 * 2 ? mean_p[1] - 20 : mean_p[2]));
			if (buffer[i] > mean && buffer[i - WIDTH_SLOPE] < mean) {
				begin = i;
				stop = 1;
			}
			i--;

		}
		//if a begin was found, search for an end
		if (i > WIDTH_SLOPE && begin != start_position) {
			stop = 0;

			while (stop == 0 && i > 0) {
				mean = (i < IMAGE_BUFFER_SIZE_DIV_3 ? mean_p[0] : (i < IMAGE_BUFFER_SIZE_DIV_3 * 2 ? mean_p[1] - 20 : mean_p[2]));
				if (buffer[i] > mean && (i + WIDTH_SLOPE >= IMAGE_BUFFER_SIZE ? buffer[IMAGE_BUFFER_SIZE - 1] : buffer[i + WIDTH_SLOPE]) < mean) {
					end = i;
					stop = 1;
				}
				i--;
			}
			//if an end was not found
			if (i == 0 || end == start_position) {
				line_not_found = 1;
			}
		}
		//if no begin was found
		else {
			line_not_found = 1;
		}

		//if a line too small has been detected, continues the search
		if (!line_not_found && abs(begin - end) < MIN_LINE_WIDTH) {
			i = end;
			begin = start_position;
			end = start_position;
			stop = 0;
			wrong_line = 1;
		} else {
			wrong_line = 0;
		}
	} while (wrong_line);

	if (line_not_found) {
		line.found = false;
		line.end_pos = 0;
		line.width = 0;
	} else {
		line.width = (uint16_t) abs(begin - end);
		line.end_pos = (uint16_t) end;
		line.begin_pos = (uint16_t) begin;
		line.found = true;
	}
	return line;
}

/*
 * 	@Describe:
 * 		Performs an average for each 3 segments third, because of auto brightness
 *	    causing the values to have a n shape : it gets dimmer on the sides.
 *
 *	@Params:
 *		uint8_t *buffer		a pixel line to analyse. IMAGE_BUFFER_SIZE long
 *		uint8_t	*mean		pointer to the mean array. Contains the mean of each third of line
 */
void calculate_mean(uint8_t *buffer, uint8_t *mean) {

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

/*
 * 	@Describe:
 * 		Rules the image capture process and the send the results for analysis.
 *
 * 	@Author:
 * 		TP4_CamReg_correction, adapted for our needs
 */
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
		if (!sleep_mode) {
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

/*
 * 	@Describe:
 * 		Analyse image when one is received.
 *
 * 	@Author:
 * 		TP4_CamReg_correction, adapted for our needs
 */
static THD_WORKING_AREA(waProcessImage, 1024);
static THD_FUNCTION(ProcessImage, arg) {

	chRegSetThreadName(__FUNCTION__);
	(void) arg;

	uint8_t *img_buff_ptr;
	uint8_t image[IMAGE_BUFFER_SIZE] = { 0 };
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

		extract_barcode(image);

		// slow send to not flood computer, show only in demo mode
		if (send_to_computer >= 15 && get_punky_state() == PUNKY_DEMO) {
			send_to_computer = 0;
			SendUint8ToComputer(image, IMAGE_BUFFER_SIZE);
		}
		send_to_computer++;
	}
}

/*
 * 	@Describe:
 * 		Allow to spare ressources by short-circuiting the images analyse threads
 * 		if sleep mode = 1, then the thread wait for a start semaphore
 * 		if sleep mode = 0, then the thread run in the background
 */
void get_image_start(void) {

	if (sleep_mode) {
		sleep_mode = 0;
		chBSemSignal(&start_imaging_sem);
	}
}

/*
 * 	@Describe:
 * 		Allow to spare ressources by short-circuiting the images analyse threads
 * 		if sleep mode = 1, then the thread wait for a start semaphore
 * 		if sleep mode = 0, then the thread run in the background
 */
void get_image_stop(void) {

	sleep_mode = 1;
	code = 0;
}

/*
 * 	@Describe:
 * 		set the code. The image capture process and image analyis process
 * 		are 13 times faster than the main file process -> code variable
 * 		is modified 13 faster than it is read. In order to not miss validated
 * 		barcode, if one is found, the variable is then lock for any incomplete
 * 		code, until the code is read by someone.
 *
 * 	@Params:
 * 		uint8_t code_p		The code found to be set
 */
void set_code(uint8_t code_p) {

	if (code_p != 0 && code_p != 1 && code_p != 2) {
		barcode_found = true;
		code = code_p;
	} else if (barcode_found == false) {
		code = code_p;
	}
}

/*
 *  @Describe:
 *  	Get the code. Release the barcode found flag. See set_code for more
 *  	explainations.
 *		Values : 13 to 39   valide code to set speed
 *				 	1 		barcode start pattern found
 *					2		barcode end pattern found
 *					0		nothing found
 *
 *  @Return:
 *  	uint8_t code	The code
 */
uint8_t get_code(void) {

	barcode_found = false;
	return code;
}

void process_image_start(void) {
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);
}
