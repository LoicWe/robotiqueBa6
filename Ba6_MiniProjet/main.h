#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "camera/dcmi_camera.h"
#include "msgbus/messagebus.h"
#include "parameter/parameter.h"

#define PUCKY_PLAY				0		//TODO: ENUM
#define PUCKY_SLEEP				1
#define PUCKY_WAKE_UP			3

#define min_dist_barcode		130
#define max_dist_barcode		155

/** Robot wide IPC bus. */
extern messagebus_t bus;

extern parameter_namespace_t parameter_root;

#ifdef __cplusplus
}
#endif

#endif
