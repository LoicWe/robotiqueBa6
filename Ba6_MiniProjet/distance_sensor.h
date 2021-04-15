/**
 * @file    VL53L0X.h
 * @brief   High level functions to use the VL53L0X TOF sensor.
 * 
 * @author  Eliot Ferragni, Timothée Hirt
 */


#ifndef VL53L0X_H
#define VL53L0X_H

#include "sensors/VL53L0X/Api/core/inc/vl53l0x_api.h"

#define USE_I2C_2V8

#define VL53L0X_ADDR 0x52

//////////////////// PROTOTYPES PUBLIC FUNCTIONS /////////////////////

/**
 * @brief 			Init the VL53L0X_Dev_t structure and the sensor.
 * 
 * @param device 	Pointer to the structure of the sensor
 * @return 			VL53L0X_Error See ::VL53L0X_Error
 */	
VL53L0X_Error distance_sensor_init(VL53L0X_Dev_t* device);

/**
 * @brief 			Configure the accuracy of the sensor (range).
 * 
 * @param device 	Pointer to the structure of the sensor
 * @param accuracy 	Accuracy chosen
 * 
 * @return 			VL53L0X_Error See ::VL53L0X_Error
 */
VL53L0X_Error distance_sensor_configAccuracy(VL53L0X_Dev_t* device, VL53L0X_AccuracyMode accuracy);

/**
 * @brief 			Begin the meausurement process with the specified mode.
 * 
 * @param device 	Pointer to the structure of the sensor
 * @param mode 		Mode chosen to take the measures
 * 
 * @return 			VL53L0X_Error See ::VL53L0X_Error
 */
VL53L0X_Error distance_sensor_startMeasure(VL53L0X_Dev_t* device, VL53L0X_DeviceModes mode);

/**
 * @brief 			Get the last valid measure and lpace it in the sensor structure given.
 * 
 * @param device 	Pointer to the structure of the sensor
 * @return 			VL53L0X_Error See ::VL53L0X_Error
 */
VL53L0X_Error distance_sensor_getLastMeasure(VL53L0X_Dev_t* device);

/**
 * @brief 			Stop the measuement process.
 * 
 * @param device 	Pointer to the structure of the sensor
 * @return 			VL53L0X_Error See ::VL53L0X_Error
 */	
VL53L0X_Error distance_sensor_stopMeasure(VL53L0X_Dev_t* device);

/**
 * @brief Init a thread which uses the distance sensor to
 * continuoulsy measure the distance.
 */
void distance_sensor_start(void);

/**
* @brief   Stop the distance measurement.
*
*/
void distance_sensor_stop(void);

/**
* @brief   Pause the thread until resumed.
*
*/
void distance_sensor_pause(void);

/**
* @brief   Resume the thread.
*
*/
void distance_sensor_resume(void);

/**
 * @brief 			Return the last distance measured in mm
 * 
 * @return 			Last distance measured in mm
 */	
uint16_t distance_sensor_get_dist_mm(void);

#endif /* VL53L0X_H*/
