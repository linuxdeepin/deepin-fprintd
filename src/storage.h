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

#ifndef __FPRINTD_STORAGE_H__
#define __FPRINTD_STORAGE_H__

#include <libfprint/fprint.h>

int print_data_save(struct fp_print_data *data, uint32_t finger, int drv_id,
                    const char *username);
int print_data_delete(uint32_t finger, int drv_id, const char *username);
struct fp_print_data **print_data_finger_load(uint32_t finger, int drv_id, const char *username);
struct fp_print_data **print_data_user_load(int drv_id, const char *username);
void print_datas_free(struct fp_print_data **datas);

struct fp_print_data *load_print_data_file(const char *path);
struct fp_print_data **load_print_datas_from_dir(const char *dir, int *length);

#endif
