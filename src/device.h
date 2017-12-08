#ifndef __FPRINTD_DEVICE_H__
#define __FPRINTD_DEVICE_H__

#include <stdlib.h>
#include <libfprint/fprint.h>

typedef struct _device {
    char *name;
    int drv_id;
} Device;

Device *list_devices(int *length);
void free_devices(Device *devs, int dev_num);

// open the special index device, if dev_idx and name not matched, return NULL.
// if dev_idx == -1, will open the first matched device
struct fp_dev *open_device(const char *name, int dev_idx);
void close_device(struct fp_dev *dev);
int enroll_finger(char *name, int drv_id, int dev_idx,
                  uint32_t finger, char *username);
int identify_finger(char *name, int drv_id, int dev_idx,
                    uint32_t finger, char *username);
int identify_user(char *name, int drv_id, int dev_idx, char *username);

int check_print_data_file(const char *file);

typedef void(*HandlerType)(int status);

extern HandlerType enroll_handler;
extern HandlerType verify_handler;

#endif
