#ifndef __FPRINTD_WRAPPER_H__
#define __FPRINTD_WRAPPER_H__

#include <stdint.h>
#include "device.h"

int enroll_finger_wrapper(char *dev, int dev_idx, int drv_id,
		uint32_t finger, char *username);
int identify_user_wrapper(char *dev, int dev_idx, int drv_id,
		char *username);

#endif
