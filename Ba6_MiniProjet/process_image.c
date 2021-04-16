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
static uint8_t suspended = 0;
static uint16_t line_position = IMAGE_BUFFER_SIZE/2;	//middle

//semaphore
static BSEMAPHORE_DECL(image_ready_sem, TRUE); // @suppress("Field cannot be resolved")

uint8_t extract_barcode(uint8_t *image){

	uint8_t code;

	// déclaration d'une ligne
	struct Line line;
	struct Line line2;
	line.end_pos = line.width = 0;
	line.found = false;
	uint8_t interline = 0;

	uint32_t mean = calculate_mean(image);

	// recherche du motif de démarrage
	line = extract_line_width(image, line, mean-5); /*-5 pour compenser la luminosité auto*/
	if(line.found){
		line2.begin_pos = line2.end_pos = line.end_pos;
		line2.width = 0;
		line2.found = false;
		line2 = extract_line_width(image, line2, mean);
	}
	if(line2.found){
		interline = line2.begin_pos - line.end_pos + LINE_THRESHOLD;
		if(line.width + LINE_THRESHOLD > START_LINE_WIDTH
				&& line.width - LINE_THRESHOLD < START_LINE_WIDTH
				&& line2.width + LINE_THRESHOLD > START_LINE_WIDTH
				&& line2.width - LINE_THRESHOLD < START_LINE_WIDTH
				&& interline + LINE_THRESHOLD > START_LINE_WIDTH
				&& interline - LINE_THRESHOLD < START_LINE_WIDTH){
			// éclaire le robot en vert
			barcode_validate();
		}
	}
	else{
	}
	chprintf((BaseSequentialStream *)&SD3, "moyenne = %d\n", mean);
	chprintf((BaseSequentialStream *)&SD3, "largeur ligne 1 = %d  et  départ1 = %d\n", line.width, line.begin_pos);
	chprintf((BaseSequentialStream *)&SD3, "largeur ligne 2 = %d  et  départ2 = %d\n", line2.width, line2.begin_pos);
	chprintf((BaseSequentialStream *)&SD3, "largeur interli = %d\n", interline);


	return code;
}


/*
 *  Returns the line's width extracted from the image buffer given
 *  Returns 0 if line not found
 */
struct Line extract_line_width(uint8_t *buffer, struct Line line, uint32_t mean){

	uint16_t i = line.end_pos, begin = line.end_pos, end = line.end_pos;
	uint8_t stop = 0, line_not_found = 0, wrong_line = 0;

	do{
		//search for a begin
		while(stop == 0 && i < (IMAGE_BUFFER_SIZE - WIDTH_SLOPE))
		{ 
			//the slope must at least be WIDTH_SLOPE wide and is compared
		    //to the mean of the image
		    if(buffer[i] > mean && buffer[i+WIDTH_SLOPE] < mean)
		    {
		        begin = i;
		        stop = 1;
		    }
		    i++;
		}
		//if a begin was found, search for an end
		if (i < (IMAGE_BUFFER_SIZE - WIDTH_SLOPE) && begin)
		{
		    stop = 0;
		    
		    while(stop == 0 && i < IMAGE_BUFFER_SIZE)
		    {
		        if(buffer[i] > mean && buffer[i-WIDTH_SLOPE] < mean)
		        {
		            end = i;
		            stop = 1;
		        }
		        i++;
		    }
		    //if an end was not found
		    if (i > IMAGE_BUFFER_SIZE || !end)
		    {
		        line_not_found = 1;
		    }
		}
		else//if no begin was found
		{
		    line_not_found = 1;
		}

		//if a line too small has been detected, continues the search
		if(!line_not_found && (end-begin) < MIN_LINE_WIDTH){
			i = end;
			begin = 0;
			end = 0;
			stop = 0;
			wrong_line = 1;
		}else{
			wrong_line = 0;
		}
	}while(wrong_line);

	if(line_not_found){
		line.found = false;
	}else{
		line.width = (end - begin);
		line.end_pos = end;
		line.begin_pos = begin;
		line.found = true;
		line_position = (begin + end)/2; //gives the line position.
	}

	return line;
}

uint32_t calculate_mean(uint8_t *buffer){
	//performs an average
	uint32_t mean = 0;
	for(uint16_t i = 0 ; i < IMAGE_BUFFER_SIZE ; i++){
		mean += buffer[i];
	}
	mean /= IMAGE_BUFFER_SIZE;
	return mean;
}

static THD_WORKING_AREA(waCaptureImage, 256);
static THD_FUNCTION(CaptureImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	//Takes pixels 0 to IMAGE_BUFFER_SIZE of the line 10 + 11 (minimum 2 lines because reasons)
	po8030_advanced_config(FORMAT_RGB565, 0, 10, IMAGE_BUFFER_SIZE, 2, SUBSAMPLING_X1, SUBSAMPLING_X1);
	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);
	dcmi_prepare();

    while(1){

    	if(!suspended){
        //starts a capture
		dcmi_capture_start();
		//waits for the capture to be done
		wait_image_ready();
		//signals an image has been captured
		chBSemSignal(&image_ready_sem);
    	}else{
			chThdSleepMilliseconds(1000);
    	}
    }
}


static THD_WORKING_AREA(waProcessImage, 1024);
static THD_FUNCTION(ProcessImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	uint8_t *img_buff_ptr;
	uint8_t image[IMAGE_BUFFER_SIZE] = {0};
	uint16_t lineWidth = 0;

	bool send_to_computer = true;

    while(1){
    	//waits until an image has been captured
        chBSemWait(&image_ready_sem);
		//gets the pointer to the array filled with the last image in RGB565    
		img_buff_ptr = dcmi_get_last_image_ptr();

		//Extracts only the red pixels
		for(uint16_t i = 0 ; i < (2 * IMAGE_BUFFER_SIZE) ; i+=2){
			//extracts first 5bits of the first byte
			//takes nothing from the second byte
			image[i/2] = (uint8_t)img_buff_ptr[i]&0xF8;
		}

		//search for a line in the image and gets its width in pixels
		lineWidth = extract_barcode(image);

		if(send_to_computer){
			//sends to the computer the image
			SendUint8ToComputer(image, IMAGE_BUFFER_SIZE);
		}
		//invert the bool
		send_to_computer = !send_to_computer;
    }
}

void capture_image(logical capture){
	if(capture == YES)
		suspended = 0;
	if(capture == NO)
		suspended = 1;
}

uint16_t get_line_position(void){
	return line_position;
}

void process_image_start(void){
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);

}
