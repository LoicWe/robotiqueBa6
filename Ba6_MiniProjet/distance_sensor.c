/**
 * @file    VL53L0X.c
 * @brief   High level functions to use the VL53L0X TOF sensor.
 *
 * @author  Eliot Ferragni, Timothée Hirt
 */

#include "distance_sensor.h"

#include "ch.h"
#include "hal.h"
#include "sensors/VL53L0X/Api/core/inc/vl53l0x_api.h"
#include "shell.h"
#include "chprintf.h"
#include "i2c_bus.h"
#include "usbcfg.h"
#include "leds.h"
#include "communication.h"

static uint16_t dist_mm = 0;
static thread_reference_t* distThd = NULL;
//static msg_t msg;
static uint8_t suspended = 0;
static bool VL53L0X_configured = false;

//////////////////// PUBLIC FUNCTIONS /////////////////////////
static THD_WORKING_AREA(waVL53L0XThd, 512);
static THD_FUNCTION(VL53L0XThd, arg) {

	chRegSetThreadName("VL53L0x Thd");
	VL53L0X_Error status = VL53L0X_ERROR_NONE;

	(void) arg;
	static VL53L0X_Dev_t device;

	device.I2cDevAddr = VL53L0X_ADDR;

	status = distance_sensor_init(&device);

	if (status == VL53L0X_ERROR_NONE) {
		distance_sensor_configAccuracy(&device, VL53L0X_DEFAULT_MODE);
	}
	if (status == VL53L0X_ERROR_NONE) {
		distance_sensor_startMeasure(&device, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
	}
	if (status == VL53L0X_ERROR_NONE) {
		VL53L0X_configured = true;
	}

	/* Reader thread loop.*/
	while (chThdShouldTerminateX() == false) {

		// panic error à chaque lancement
		/*		chSysLock();
		 if(suspended){
		 msg = chThdSuspendS(distThd);
		 }
		 chSysUnlock();*/

		// vu qu'on ne peut pas stopper, ralenti le rythme si "suspendu"
		if (!suspended) {
			if (VL53L0X_configured) {
				distance_sensor_getLastMeasure(&device);
				dist_mm = device.Data.LastRangeMeasure.RangeMilliMeter;
			}
			chThdSleepMilliseconds(100);
		} else {
			chThdSleepMilliseconds(1000);
		}
	}
}

VL53L0X_Error distance_sensor_init(VL53L0X_Dev_t* device) {

	VL53L0X_Error status = VL53L0X_ERROR_NONE;

	uint8_t VhvSettings;
	uint8_t PhaseCal;
	uint32_t refSpadCount;
	uint8_t isApertureSpads;

//init
	if (status == VL53L0X_ERROR_NONE) {
		// Structure and device initialisation
		status = VL53L0X_DataInit(device);
	}

	if (status == VL53L0X_ERROR_NONE) {
		// Get device info
		status = VL53L0X_GetDeviceInfo(device, &(device->DeviceInfo));
	}

	if (status == VL53L0X_ERROR_NONE) {
		// Device Initialization
		status = VL53L0X_StaticInit(device);
	}

//calibration
	if (status == VL53L0X_ERROR_NONE) {
		// SPAD calibration
		status = VL53L0X_PerformRefSpadManagement(device, &refSpadCount,
				&isApertureSpads);
	}

	if (status == VL53L0X_ERROR_NONE) {
		// Calibration
		status = VL53L0X_PerformRefCalibration(device, &VhvSettings, &PhaseCal);
	}

	return status;
}

VL53L0X_Error distance_sensor_configAccuracy(VL53L0X_Dev_t* device,
		VL53L0X_AccuracyMode accuracy) {

	VL53L0X_Error status = VL53L0X_ERROR_NONE;

//Activation Limits
	if (status == VL53L0X_ERROR_NONE) {
		status = VL53L0X_SetLimitCheckEnable(device,
		VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1);
	}

	if (status == VL53L0X_ERROR_NONE) {
		status = VL53L0X_SetLimitCheckEnable(device,
		VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1);
	}

//specific accuracy config
//copied from ST example and API Guide
	switch (accuracy) {

	case VL53L0X_DEFAULT_MODE:
		if (status == VL53L0X_ERROR_NONE) {
			status = VL53L0X_SetLimitCheckEnable(device,
			VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, 1);
		}

		if (status == VL53L0X_ERROR_NONE) {
			status = VL53L0X_SetLimitCheckValue(device,
			VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD,
					(FixPoint1616_t) (1.5 * 0.023 * 65536));
		}
		break;

	case VL53L0X_HIGH_ACCURACY:
		if (status == VL53L0X_ERROR_NONE) {
			status = VL53L0X_SetLimitCheckValue(device,
			VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
					(FixPoint1616_t) (0.25 * 65536));
		}
		if (status == VL53L0X_ERROR_NONE) {
			status = VL53L0X_SetLimitCheckValue(device,
			VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
					(FixPoint1616_t) (18 * 65536));
		}
		if (status == VL53L0X_ERROR_NONE) {
			status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(device,
					200000);
		}
		break;

	case VL53L0X_LONG_RANGE:
		if (status == VL53L0X_ERROR_NONE) {
			status = VL53L0X_SetLimitCheckValue(device,
			VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
					(FixPoint1616_t) (0.1 * 65536));
		}
		if (status == VL53L0X_ERROR_NONE) {
			status = VL53L0X_SetLimitCheckValue(device,
			VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
					(FixPoint1616_t) (60 * 65536));
		}
		if (status == VL53L0X_ERROR_NONE) {
			status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(device,
					33000);
		}
		if (status == VL53L0X_ERROR_NONE) {
			status = VL53L0X_SetVcselPulsePeriod(device,
			VL53L0X_VCSEL_PERIOD_PRE_RANGE, 18);
		}
		if (status == VL53L0X_ERROR_NONE) {
			status = VL53L0X_SetVcselPulsePeriod(device,
			VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 14);
		}
		break;

	case VL53L0X_HIGH_SPEED:
		if (status == VL53L0X_ERROR_NONE) {
			status = VL53L0X_SetLimitCheckValue(device,
			VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
					(FixPoint1616_t) (0.25 * 65536));
		}
		if (status == VL53L0X_ERROR_NONE) {
			status = VL53L0X_SetLimitCheckValue(device,
			VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
					(FixPoint1616_t) (32 * 65536));
		}
		if (status == VL53L0X_ERROR_NONE) {
			status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(device,
					30000);
		}
		break;
	default:
		status = VL53L0X_ERROR_INVALID_PARAMS;
		break;
	}

	return status;
}

VL53L0X_Error distance_sensor_startMeasure(VL53L0X_Dev_t* device,
		VL53L0X_DeviceModes mode) {

	VL53L0X_Error status = VL53L0X_ERROR_NONE;

	status = VL53L0X_SetDeviceMode(device, mode);

	if (status == VL53L0X_ERROR_NONE) {
		status = VL53L0X_StartMeasurement(device);
	}

	return status;
}

VL53L0X_Error distance_sensor_getLastMeasure(VL53L0X_Dev_t* device) {
	return VL53L0X_GetRangingMeasurementData(device,
			&(device->Data.LastRangeMeasure));
}

VL53L0X_Error distance_sensor_stopMeasure(VL53L0X_Dev_t* device) {
	return VL53L0X_StopMeasurement(device);
}

void distance_sensor_start(void) {

	if (VL53L0X_configured) {
		return;
	}

	i2c_start();

	distThd = chThdCreateStatic(waVL53L0XThd, sizeof(waVL53L0XThd),NORMALPRIO, VL53L0XThd,
	NULL);
}

void distance_sensor_stop(void) {
	chThdTerminate(distThd);
	chThdWait(distThd);
	distThd = NULL;
	VL53L0X_configured = false;
}

void distance_sensor_pause(void) {
	suspended = 1;
}

void distance_sensor_resume(void) {
	suspended = 0;
}

uint16_t distance_sensor_get_dist_mm(void) {
	return dist_mm;
}

