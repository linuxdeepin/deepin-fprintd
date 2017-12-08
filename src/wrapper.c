#include "wrapper.h"
#include "_cgo_export.h"

static void
handle_enroll_status(int status)
{
	handleEnrollStatus(status);
}

static void
handle_verify_status(int status)
{
	handleVerifyStatus(status);
}

int
enroll_finger_wrapper(char *dev, int dev_idx, int drv_id, 
		uint32_t finger, char *username)
{
	enroll_handler = &handle_enroll_status;
	return enroll_finger(dev, dev_idx, drv_id, finger, username);
}

int
identify_user_wrapper(char *dev, int dev_idx, int drv_id, 
		char *username)
{
	verify_handler = &handle_verify_status;
	return identify_user(dev, dev_idx, drv_id, username);
}
