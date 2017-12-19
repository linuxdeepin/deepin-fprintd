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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include "storage.h"
#include "utils.h"

#define PRINT_DATA_DIR "/var/lib/deepin/fprintd/"

static char *finger_to_str(enum fp_finger finger);
static char *gen_print_data_id();
static char *get_print_data_dir(enum fp_finger finger, int drv_id, const char *username);
static char *get_print_data_path(char *dir);

int
print_data_save(struct fp_print_data *data, enum fp_finger finger, int drv_id,
                const char *username)
{
    char *dir = get_print_data_dir(finger, drv_id, username);
    if (!dir) {
        return -1;
    }

    int ret = mkdir_recursion(dir);
    if (ret != 0) {
        free(dir);
        return -1;
    }

    char *path = get_print_data_path(dir);
    if (!path) {
        free(dir);
        dir = NULL;
        return -1;
    }

    FILE *fp = fopen(path, "w+");
    free(path);
    if (!fp) {
        fprintf(stderr, "Failed to open data file: %s\n", strerror(errno));
        return -1;
    }

    char *buf;
    size_t len = fp_print_data_get_data(data, (unsigned char**)&buf);
    if (!len) {
        fclose(fp);
        fprintf(stderr, "Failed to get print data: %s\n", strerror(errno));
        return -1;
    }

    size_t wlen = fwrite(buf, sizeof(char), len, fp);
    free(buf);
    if (wlen < 1) {
        fclose(fp);
        fprintf(stderr, "Failed to write print data: %s\n", strerror(errno));
        return -1;
    }

    fflush(fp);
    fclose(fp);

    return 0;
}

int
print_data_delete(enum fp_finger finger, int drv_id, const char *username)
{
    char *dir = get_print_data_dir(finger, drv_id, username);
    if (!dir) {
        return -1;
    }

    char **names = read_dir_files(dir);
    if (!names) {
        free(dir);
        return -1;
    }

    int i = 0;
    for (; names[i]; i++) {
        char path[1024] = {0};
        int ret = snprintf(path, 1024, "%s/%s", dir, names[i]);
        if (ret < 0) {
            fprintf(stderr, "Failed to combing for load: %s\n", strerror(errno));
            continue;
        }

        unlink(path);
    }
    free_strv(names);
    rmdir(dir);
    free(dir);
    return 0;
}

struct fp_print_data**
print_data_finger_load(enum fp_finger finger, int drv_id, const char *username)
{
    char *dir = get_print_data_dir(finger, drv_id, username);
    if (!dir) {
        return NULL;
    }

    int len = 0;
    struct fp_print_data **datas = load_print_datas_from_dir(dir, &len);
    free(dir);
    printf("Load %d print data for %s, %u, %d\n", len, username, finger, drv_id);

    return datas;
}

struct fp_print_data**
print_data_user_load(int drv_id, const char *username)
{
    int ret;
    char id[10] = {0};
    ret = snprintf(id, 10, "%d", drv_id);
    if (ret < 0) {
        fprintf(stderr, "Failed to combing drv id: %s\n", strerror(errno));
        return NULL;
    }

    char *dir = (char*)calloc(strlen(PRINT_DATA_DIR) + strlen(username) + 2, 1);
    if (!dir) {
        fprintf(stderr, "Failed to combing user dir: %s\n", strerror(errno));
        return NULL;
    }

    ret = sprintf(dir, "%s/%s", PRINT_DATA_DIR, username);
    if (ret < 0) {
        free(dir);
        fprintf(stderr, "Failed to combing user dir: %s\n", strerror(errno));
        return NULL;
    }

    char **subdirs = read_dir_subdirs(dir);
    if (!subdirs) {
        free(dir);
        fprintf(stderr, "Failed to get subdirs: %s\n", strerror(errno));
        return NULL;
    }

    struct fp_print_data **datas = NULL;
    int count = 0;
    int i = 0;
    for (; subdirs[i]; i++) {
        char *fingerDir = (char*)calloc(strlen(dir) +
                                        strlen(subdirs[i]) +
                                        strlen(id) + 3, 1);
        if (!fingerDir) {
            fprintf(stderr, "Failed to combing finger(%s) dir: %s\n", subdirs[i], strerror(errno));
            continue;
        }

        sprintf(fingerDir, "%s/%s/%s", dir, subdirs[i], id);

        int tmpLen = 0;
        struct fp_print_data **tmp = load_print_datas_from_dir(fingerDir, &tmpLen);
        free(fingerDir);
        if (!tmp || tmpLen == 0) {
            continue;
        }

        // copy tmp to datas
        struct fp_print_data **list = (struct fp_print_data**)realloc(datas,
                                                                      (count + tmpLen + 1) * sizeof(*tmp));
        if (!list) {
            print_datas_free(tmp);
            continue;
        }

        datas = list;
        list = NULL;

        int j = 0;
        for (; j < tmpLen; j++) {
            datas[count+j] = tmp[j];
        }
        free(tmp);
        datas[count+j] = NULL;
        count = tmpLen;
    }

    free(dir);
    free_strv(subdirs);
    return datas;
}

void
print_datas_free(struct fp_print_data **datas)
{
    if (!datas) {
        return;
    }

    int i = 0;
    for (; datas[i]; i++) {
        fp_print_data_free(datas[i]);
    }
    free(datas);
}

static char*
get_print_data_path(char *dir)
{
    char *id = gen_print_data_id();
    if (!id) {
        return NULL;
    }

    int dir_len = strlen(dir);
    int id_len = strlen(id);
    char *tmp = (char*)realloc(dir, sizeof(char) * (dir_len + 1+ id_len + 1));
    if (!tmp) {
        free(id);
        fprintf(stderr, "Failed to realloc for path: %s\n", strerror(errno));
        return NULL;
    }

    dir = tmp;
    dir[dir_len] = '/';
    memcpy(dir+dir_len+1, id, id_len);
    dir[dir_len+id_len+1] = '\0';
    free(id);

    return dir;
}

static char*
finger_to_str(enum fp_finger finger)
{
    switch (finger) {
    case LEFT_THUMB:
        return strdup("left-thumb");
    case LEFT_INDEX:
        return strdup("left-index");
    case LEFT_MIDDLE:
        return strdup("left-middle");
    case LEFT_RING:
        return strdup("left-ring");
    case LEFT_LITTLE:
        return strdup("left-little");
    case RIGHT_THUMB:
        return strdup("right-thumb");
    case RIGHT_INDEX:
        return strdup("right-index");
    case RIGHT_MIDDLE:
        return strdup("right-middle");
    case RIGHT_RING:
        return strdup("right-ring");
    case RIGHT_LITTLE:
        return strdup("right-little");
    }

    return NULL;
}

/**
 * 唯一 id 生成器，参考了 twitter snowflake 算法。
 * id 由 timestamp + sequence 组成，限定 sequence < 1000, 即 1s 只能生成 1000 个 id
 *
 * 存在的问题：如果用户生成过 id 后又把时间调到过去，可能出现 id 重复的问题，暂不处理
 **/

#define _MAX_SEQUENCE 1000

static char*
gen_print_data_id()
{
    static int _sequence = 0;
    static uintmax_t _last_timestamp = 0;
    static char id[1024];

    time_t result = time(NULL);
    uintmax_t timestamp = (uintmax_t)result;

    if (timestamp == _last_timestamp) {
        if (_sequence >= _MAX_SEQUENCE) {
            // 此秒内的 id 已分配完
            fprintf(stderr, "The id has reached the maximum, wait the next second\n");
            return NULL;
        }

        _sequence += 1;
    } else {
        _last_timestamp = timestamp;
        _sequence = 0;
    }

    memset(id, 0, 1024);
    int ret = snprintf(id, 1024, "%lu%d", timestamp, _sequence);
    if (ret < 0) {
        fprintf(stderr, "Failed to combing id: %s\n", strerror(errno));
        return NULL;
    }

    return strdup(id);
}

static char*
get_print_data_dir(enum fp_finger finger, int drv_id, const char *username)
{
    int ret;
    char id[10] = {0};
    ret = snprintf(id, 10, "%d", drv_id);
    if (ret < 0) {
        fprintf(stderr, "Failed to combing drv id: %s\n", strerror(errno));
        return NULL;
    }

    char *finger_name = finger_to_str(finger);
    char *dir = (char*)calloc(strlen(PRINT_DATA_DIR) +
                              strlen(username) +
                              strlen(finger_name) +
                              strlen(id) + 3, sizeof(char));
    if (!dir) {
        fprintf(stderr, "Failed to allocate memory for path: %s\n", strerror(errno));
        goto failed;
    }

    ret = sprintf(dir, "%s%s/%s/%s", PRINT_DATA_DIR, username, finger_name, id);
    if (ret < 0) {
        free(dir);
        dir = NULL;
        fprintf(stderr, "Failed to combing path: %s\n", strerror(errno));
        goto failed;
    }

failed:
    free(finger_name);
    finger_name = NULL;

    return dir;
}

struct fp_print_data*
load_print_data_file(const char *path)
{
    long length;
    char *contents = NULL;

    contents = read_file_contents(path, &length);
    if (!contents) {
        return NULL;
    }

    struct fp_print_data *fdata = NULL;
    fdata = fp_print_data_from_data((unsigned char *)contents, length);
    free(contents);
    return fdata;
}

struct fp_print_data**
load_print_datas_from_dir(const char *dir, int *length)
{
    char **names = read_dir_files(dir);
    if (!names) {
        return NULL;
    }

    int i = 0;
    int count = 1;
    struct fp_print_data **datas = NULL;
    for (; names[i]; i++) {
        char path[1024] = {0};
        int ret = snprintf(path, 1024, "%s/%s", dir, names[i]);
        if (ret < 0) {
            fprintf(stderr, "Failed to combing for load: %s\n", strerror(errno));
            continue;
        }

        struct fp_print_data *data = load_print_data_file(path);
        if (!data) {
            continue;
        }
        struct fp_print_data **tmp = (struct fp_print_data**)realloc(datas, sizeof(data) * (count+1));
        if (!tmp) {
            fp_print_data_free(data);
            fprintf(stderr, "Failed to realloc for load data: %s\n", strerror(errno));
            continue;
        }

        datas = tmp;
        datas[count-1] = data;
        count++;
    }
    free_strv(names);
    if (count != 1) {
        datas[count-1] = NULL;
    }

    *length = count -1;
    return datas;
}
