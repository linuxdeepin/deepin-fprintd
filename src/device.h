/*
 * Copyright (C) 2017 ~ 2017 Deepin Technology Co., Ltd.
 *
 * Author:     jouyouyun <jouyouwen717@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
int identify_datas(char *name, int dev_idx, struct fp_print_data **datas);

int check_print_data_file(const char *file);

typedef void(*HandlerType)(int status);

extern HandlerType enroll_handler;
extern HandlerType verify_handler;

#endif
