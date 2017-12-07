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
