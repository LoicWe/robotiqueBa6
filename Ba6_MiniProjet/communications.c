#include "ch.h"
#include "hal.h"
#include <spi_comm.h>

#include <communications.h>

/*
 * 	@Describe:
 * 		Sends int8 numbers to the computer
 *
 *	@Params:
 *		uint8_t *data	the datas to send
 *		uint16_t size	the size of the data
 *
 * 	@Author:
 * 		TP5_Noisy_correction
 */
void SendUint8ToComputer(uint8_t* data, uint16_t size)
{
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)"START", 5);
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)&size, sizeof(uint16_t));
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)data, size);
}

