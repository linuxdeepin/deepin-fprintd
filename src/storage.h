#ifndef __FPRINTD_STORAGE_H__
#define __FPRINTD_STORAGE_H__

#include <libfprint/fprint.h>

int print_data_save(struct fp_print_data *data, enum fp_finger finger, int drv_id,
                    const char *username);
int print_data_delete(enum fp_finger finger, int drv_id, const char *username);
struct fp_print_data **print_data_load(enum fp_finger finger, int drv_id, const char *username);
void print_datas_free(struct fp_print_data **datas);

struct fp_print_data *load_print_data_file(const char *path);

#endif
