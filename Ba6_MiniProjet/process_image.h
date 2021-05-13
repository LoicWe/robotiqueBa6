#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

#define IMAGE_BUFFER_SIZE		640		// width in pixel of an image
#define MIN_LINE_WIDTH			10		// min line width that is valid to find
#define WIDTH_SLOPE				5		// max slope width (dist. between up and down)
#define LINE_THRESHOLD 			5		// margin for line width reference
#define START_LINE_WIDTH		44		// line width reference (medium size)
#define IMAGE_BUFFER_SIZE_DIV_3	213		// a third of the buffer
#define NB_LINE_BARCODE			7		// number of line in a barcode

enum RATIO{
	SMALL = 1,
	MEDIUM = 2,
	LARGE = 3
};

enum CODE_TYPE{
	START_PATTERN = 1,
	END_PATTERN = 2,
	NOT_DETECTED = 0
};

struct Line{
	// can take value from 0 to IMAGE_BUFFER_SIZE
	uint16_t end_pos, begin_pos;
	uint16_t width;
	bool found;
};

void get_image_start(void);
void get_image_stop(void);
uint8_t get_code(void);
void set_code(uint8_t code_p);
void process_image_start(void);
struct Line line_find_next(uint8_t *buffer, uint16_t start_position, uint8_t *mean_p);
struct Line line_find_next_inverted_direction(uint8_t *buffer, int16_t start_position, uint8_t *mean_p);
void extract_barcode(uint8_t *image);
void calculate_mean(uint8_t *buffer, uint8_t *mean);
uint8_t line_classify(struct Line line, uint8_t width_unit);

#endif /* PROCESS_IMAGE_H */
