#include <stdio.h>
#include <string.h>

#include "device.h"

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <operation>\n", argv[0]);
        printf("Operation: enroll, identify\n");
        return 0;
    }

    int ret = fp_init();
    if (ret != 0) {
        fprintf(stderr, "Failed to init fprint\n");
        return -1;
    }

    int length = 0;
    Device *devs = list_devices(&length);
    if (!devs) {
        fp_exit();
        return -1;
    }

    int i = 0;
    for (; i < length; i++) {
        printf("%d: %s, %d\n", i, devs[i].name, devs[i].drv_id);
    }
    free_devices(devs, length);

    if (strcmp(argv[1], "identify") == 0) {
        printf("Start to identify\n");
        ret = identify_finger("Digital Persona U.are.U 4000/4000B/4500", 2, -1,
                              LEFT_THUMB, "deepin");
        if (ret != 0) {
            fp_exit();
            return -1;
        }
        printf("Identify successful\n");
        goto out;
    }

    if (strcmp(argv[1], "enroll") == 0) {
        printf("Will enroll 2 times\n");
        for (i = 0; i < 2; i++) {
            ret = enroll_finger("Digital Persona U.are.U 4000/4000B/4500", 2, -1,
                                LEFT_THUMB, "deepin");
            if (ret != 0) {
                fprintf(stderr, "Failed in %d times enrolled\n", i+1);
                continue;
            }
        }
    }

out:
    fp_exit();
    return 0;
}
