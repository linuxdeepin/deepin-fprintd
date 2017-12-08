#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "device.h"
#include "utils.h"
#include "storage.h"

static struct fp_print_data *do_enroll(struct fp_dev *dev, HandlerType handler);
static int do_identify(struct fp_dev *dev, struct fp_print_data **datas,
                       size_t *match, HandlerType handler);
static void handle_enroll_status(int status);
static void handle_verify_status(int status);

HandlerType enroll_handler = &handle_enroll_status;
HandlerType verify_handler = &handle_verify_status;

static struct fp_dscv_dev**
discover_devs_wrapper()
{
    struct fp_dscv_dev **discovered_devs = fp_discover_devs();
    if (!discovered_devs) {
        fprintf(stderr, "Failed to discover devices\n");
    }
    return discovered_devs;
}

static struct fp_dev*
open_device_wrapper(const char *name, int dev_idx)
{
    struct fp_dev *dev = open_device(name, dev_idx);
    if (!dev) {
        fprintf(stderr, "Failed to open device, not matched\n");
    }
    return dev;
}

static int
is_valid_finger(enum fp_finger finger)
{
    switch (finger) {
    case LEFT_THUMB:
    case LEFT_INDEX:
    case LEFT_MIDDLE:
    case LEFT_RING:
    case LEFT_LITTLE:
    case RIGHT_THUMB:
    case RIGHT_INDEX:
    case RIGHT_MIDDLE:
    case RIGHT_RING:
    case RIGHT_LITTLE:
        return 1;
    }

    fprintf(stderr, "Invalid finger\n");
    return 0;
}

Device*
list_devices(int *length)
{
    if (!length) {
        fprintf(stderr, "Invalid argument\n");
        return NULL;
    }

    struct fp_dscv_dev **ddevs = discover_devs_wrapper();
    if (!ddevs) {
        return NULL;
    }

    int i = 0;
    int count = 0;
    Device *devs = NULL;
    struct fp_dscv_dev *ddev = NULL;
    for (; (ddev = ddevs[i]); i++) {
        struct fp_driver *drv = fp_dscv_dev_get_driver(ddev);
        char *name = strdup(fp_driver_get_full_name(drv));
        if (!name) {
            fprintf(stderr, "Failed to duplicate name(%s): %s\n",
                    fp_driver_get_name(drv), strerror(errno));
            continue;
        }

        Device *tmp = (Device*)realloc(devs, (count+1) * sizeof(Device));
        if (!tmp) {
            fprintf(stderr, "Failled to realloc memory: %s\n", strerror(errno));
            free(name);
            name = NULL;
            continue;
        }

        devs = tmp;
        tmp = NULL;
        devs[count].name = name;
        devs[count].drv_id = fp_driver_get_driver_id(drv);
        count++;
    }

    fp_dscv_devs_free(ddevs);
    ddevs = NULL;

    *length = count;
    return devs;
}

void
free_devices(Device *devs, int dev_num)
{
    if (!devs) {
        return;
    }

    int i = 0;
    for (; i < dev_num; i++) {
        free(devs[i].name);
    }
    free(devs);
}

struct fp_dev*
open_device(const char *name, int dev_idx)
{
    if (!name) {
        return NULL;
    }

    struct fp_dscv_dev **ddevs = discover_devs_wrapper();
    if (!ddevs) {
        return NULL;
    }

    int i =0;
    struct fp_dev *dev = NULL;
    struct fp_dscv_dev *ddev = NULL;
    for (; (ddev = ddevs[i]); i++) {
        if (dev_idx != -1 && dev_idx != i) {
            continue;
        }

        struct fp_driver *drv = fp_dscv_dev_get_driver(ddev);
        const char *value = fp_driver_get_full_name(drv);
        int ret = strcmp(name, value);
        if (dev_idx == -1) {
            if (ret != 0) {
                continue;
            }
        } else {
            if (ret != 0) {
                // not matched
                break;
            }
        }

        // matched
        dev = fp_dev_open(ddev);
        break;
    }

    fp_dscv_devs_free(ddevs);
    ddevs = NULL;

    return dev;
}

void
close_device(struct fp_dev *dev)
{
    if (!dev) {
        return ;
    }

    fp_dev_close(dev);
    dev = NULL;
}

int
enroll_finger(char *name, int drv_id, int dev_idx,
              uint32_t finger, char *username)
{
    if (!name||!username) {
        fprintf(stderr, "Invalid args for enroll\n");
        return -1;
    }

    int ret = is_valid_finger((enum fp_finger)finger);
    if (!ret) {
        return -1;
    }

    struct fp_dev *dev = open_device_wrapper(name, dev_idx);
    if (!dev) {
        return -1;
    }

    struct fp_print_data *data = do_enroll(dev, enroll_handler);
    if (!data) {
        fprintf(stderr, "Failed to enroll finger\n");
        fp_dev_close(dev);
        dev = NULL;
        return -1;
    }

    fp_dev_close(dev);
    dev = NULL;

    ret = print_data_save(data, finger, drv_id, username);
    fp_print_data_free(data);

    return ret;
}

int
identify_finger(char *name, int drv_id, int dev_idx,
                uint32_t finger, char *username)
{
    if (!name || !username) {
        fprintf(stderr, "Invalid args for identify\n");
        return -1;
    }

    int ret = is_valid_finger((enum fp_finger)finger);
    if (!ret) {
        return -1;
    }

    struct fp_dev *dev = open_device_wrapper(name, dev_idx);
    if (!dev) {
        return -1;
    }

    struct fp_print_data **datas = print_data_finger_load(finger, drv_id, username);
    if (!datas) {
        fp_dev_close(dev);
        return -1;
    }

    size_t match = 0;
    ret = do_identify(dev, datas, &match, verify_handler);
    printf("Matched(%lu) for %s - %s\n", match, username, name);
    fp_dev_close(dev);
    print_datas_free(datas);

    return ret;
}

int
identify_user(char *name, int drv_id, int dev_idx, char *username)
{
    if (!name || !username) {
        fprintf(stderr, "Invalid args for identify\n");
        return -1;
    }

    struct fp_dev *dev = open_device_wrapper(name, dev_idx);
    if (!dev) {
        return -1;
    }

    struct fp_print_data **datas = print_data_user_load(drv_id, username);
    if (!datas) {
        fp_dev_close(dev);
        return -1;
    }

    size_t match = 0;
    int ret = do_identify(dev, datas, &match, verify_handler);
    printf("Matched(%lu) for %s - %s\n", match, username, name);
    fp_dev_close(dev);
    print_datas_free(datas);

    return ret;
}

int
check_print_data_file(const char *file)
{
    struct fp_print_data *data = load_print_data_file(file);
    if (!data) {
        return 0;
    }
    fp_print_data_free(data);
    return 1;
}

static struct fp_print_data*
do_enroll(struct fp_dev *dev, HandlerType handler)
{
    printf("You will need to successfully scan you finger %d times\n",
           fp_dev_get_nr_enroll_stages(dev));
    struct fp_print_data *print_data = NULL;
    int r = 0;
    do{
        printf("\nScan your finger now.\n");
        r = fp_enroll_finger(dev, &print_data);
        if (r < 0) {
            fprintf(stderr, "Failed to enroll finger\n");
            return NULL;
        }

		if (handler) {
			(*handler)(r);
		}
    } while (r != FP_ENROLL_COMPLETE);

    if (!print_data) {
        fprintf(stderr, "Enroll complete but no print!\n");
        return NULL;
    }

    printf("Enrollment completed!\n\n");
    return print_data;
}

static int
do_identify(struct fp_dev *dev, struct fp_print_data **datas,
            size_t *match, HandlerType handler)
{
    printf("\nScan your finger for identify.\n");
    int r = fp_identify_finger(dev, datas, match);
    if (r < 0) {
        fprintf(stderr, "Failed to identify, error code: %d\n", r);
        return -1;
    }

	if (handler) {
		(*handler)(r);
	}
    if (r == FP_VERIFY_MATCH) {
        return 0;
    }

    return -1;
}

static void
handle_enroll_status(int status)
{
	return;
}

static void
handle_verify_status(int status)
{
	return;
}
