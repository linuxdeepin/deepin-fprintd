#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "device.h"
#include "utils.h"
#include "storage.h"

static struct fp_print_data *do_enroll(struct fp_dev *dev);
static int do_identify(struct fp_dev *dev, struct fp_print_data **datas,
                       size_t *match);

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

char**
list_devices()
{
    struct fp_dscv_dev **ddevs = discover_devs_wrapper();
    if (!ddevs) {
        return NULL;
    }

    int i = 0;
    int count = 1;
    char **devs = NULL;
    struct fp_dscv_dev *ddev = NULL;
    for (; (ddev = ddevs[i]); i++) {
        struct fp_driver *drv = fp_dscv_dev_get_driver(ddev);
        char *name = strdup(fp_driver_get_full_name(drv));
        if (!name) {
            fprintf(stderr, "Failed to duplicate name(%s): %s\n",
                    fp_driver_get_name(drv), strerror(errno));
            continue;
        }

        char **tmp = (char**)realloc(devs, (count+1) * sizeof(char*));
        if (!tmp) {
            fprintf(stderr, "Failled to realloc memory: %s\n", strerror(errno));
            free(name);
            name = NULL;
            continue;
        }

        devs = tmp;
        tmp = NULL;
        devs[count-1] = name;
        count++;
    }

    fp_dscv_devs_free(ddevs);
    ddevs = NULL;
    devs[count-1] = NULL;

    return devs;
}

void
free_devices(char **devs)
{
    free_strv(devs);
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
enroll_finger(char *name, int dev_idx,
              enum fp_finger finger, char *username)
{
    if (!name||!username) {
        fprintf(stderr, "Invalid args for enroll\n");
        return -1;
    }

    int ret = is_valid_finger(finger);
    if (!ret) {
        return -1;
    }

    struct fp_dev *dev = open_device_wrapper(name, dev_idx);
    if (!dev) {
        return -1;
    }

    struct fp_print_data *data = do_enroll(dev);
    if (!data) {
        fprintf(stderr, "Failed to enroll finger\n");
        fp_dev_close(dev);
        dev = NULL;
        return -1;
    }

    fp_dev_close(dev);
    dev = NULL;

    ret = print_data_save(data, finger, username);
    fp_print_data_free(data);

    return ret;
}

int
identify_finger(char *name, int dev_idx,
                enum fp_finger finger, size_t *match, 
                char *username)
{
    if (!name || !match || !username) {
        fprintf(stderr, "Invalid args for identify\n");
        return -1;
    }

    int ret = is_valid_finger(finger);
    if (!ret) {
        return -1;
    }

    struct fp_dev *dev = open_device_wrapper(name, dev_idx);
    if (!dev) {
        return -1;
    }

    struct fp_print_data **datas = print_data_load(finger, username);
    if (!datas) {
        return -1;
    }
    ret = do_identify(dev, datas, match);
    fp_dev_close(dev);
    print_datas_free(datas);

    return ret;
}

static struct fp_print_data*
do_enroll(struct fp_dev *dev)
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

        switch (r) {
        case FP_ENROLL_COMPLETE:
            printf("Enroll complete!\n");
            break;
        case FP_ENROLL_FAIL:
            printf("Enroll failed :(!\n");
            break;
        case FP_ENROLL_PASS:
            printf("Enroll stage passed!\n");
            break;
        case FP_ENROLL_RETRY:
            printf("Didn't quite catch that. Please try again!\n");
            break;
        case FP_ENROLL_RETRY_TOO_SHORT:
            printf("Your swipe was too short, try again!\n");
            break;
        case FP_ENROLL_RETRY_CENTER_FINGER:
            printf("Please center your finger on the sensor, try again!\n");
            break;
        case FP_ENROLL_RETRY_REMOVE_FINGER:
            printf("Scan failed, please remove your finger and try again!\n");
            break;
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
            size_t *match)
{
    printf("\nScan your finger for identify.\n");
    int r = fp_identify_finger(dev, datas, match);
    if (r < 0) {
        fprintf(stderr, "Failed to identify, error code: %d\n", r);
        return -1;
    }

    switch (r) {
    case FP_VERIFY_NO_MATCH:
        printf("No match!\n");
        r = -1;
        break;
    case FP_VERIFY_MATCH:
        printf("Matched!\n");
        r = 0;
        break;
    case FP_VERIFY_RETRY:
        printf("Didn't quite catch that. Please try again!\n");
        r = -1;
        break;
    case FP_VERIFY_RETRY_TOO_SHORT:
        printf("Your swipe was too short, try again!\n");
        r = -1;
        break;
    case FP_VERIFY_RETRY_CENTER_FINGER:
        printf("Please center your finger on the sensor, try again!\n");
        r = -1;
        break;
    case FP_ENROLL_RETRY_REMOVE_FINGER:
        printf("Scan failed, please remove your finger and try again!\n");
        r = -1;
        break;
    }

    return r;
}
