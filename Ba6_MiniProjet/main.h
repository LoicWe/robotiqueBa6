#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "camera/dcmi_camera.h"
#include "msgbus/messagebus.h"
#include "parameter/parameter.h"

#define PUCKY_PLAY				0
#define PUCKY_SLEEP				1
#define PUCKY_WAKE_UP			3

#define IMAGE_BUFFER_SIZE		640
#define MIN_LINE_WIDTH				40
#define WIDTH_SLOPE				5
#define PXTOCM					1570.0f //experimental value
#define MAX_DISTANCE 			25.0f
#define GOAL_DISTANCE 			10.0f
#define ROTATION_THRESHOLD		10
#define ROTATION_COEFF			2
#define ERROR_THRESHOLD			0.1f	//[cm] because of the noise of the camera
#define KP						800.0f
#define KI 						3.5f	//must not be zero
#define MAX_SUM_ERROR 			(MOTOR_SPEED_LIMIT/KI)


/** Robot wide IPC bus. */
extern messagebus_t bus;

extern parameter_namespace_t parameter_root;

#ifdef __cplusplus
}
#endif

#endif
