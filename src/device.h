#ifndef __FPRINTD_DEVICE_H__
#define __FPRINTD_DEVICE_H__

#include <stdlib.h>
#include <libfprint/fprint.h>

char **list_devices();
void free_devices(char **devs);

// open the special index device, if dev_idx and name not matched, return NULL.
// if dev_idx == -1, will open the first matched device
struct fp_dev *open_device(const char *name, int dev_idx);
void close_device(struct fp_dev *dev);
int enroll_finger(char *name, int dev_num,
                  enum fp_finger finger, char *username);
int identify_finger(char *name, int dev_num,
                    enum fp_finger finger, size_t *match, 
                    char *username);

#endif
