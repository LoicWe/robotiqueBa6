#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

#define IMAGE_BUFFER_SIZE		640
#define MIN_LINE_WIDTH			10
#define WIDTH_SLOPE				5
#define LINE_THRESHOLD 			5		// différence acceptable de largeur par rapport à la ligne de référence
#define START_LINE_WIDTH		44		// largeur ligne de référence
#define IMAGE_BUFFER_SIZE_DIV_3	213		// un tiers du buffer
#define NB_LINE_BARCODE			7		// nombre de lignes composant le code

enum ratio{
	SMALL = 1,
	MEDIUM = 2,
	LARGE = 3
};

struct Line{
	uint16_t end_pos, begin_pos;
	uint8_t width;
	bool found;
};

void get_image_run(void);
void get_image_stop(void);
uint8_t get_code(void);
void set_code(uint8_t code_p);
void process_image_start(void);
struct Line line_find_next(uint8_t *buffer, uint16_t start_position, uint32_t mean);
void extract_barcode(uint8_t *image);
void calculate_mean(uint8_t *buffer, uint8_t *mean);
uint8_t line_classify(struct Line line, uint8_t width_unit);
//void demo_led(uint8_t code);




#endif /* PROCESS_IMAGE_H */
