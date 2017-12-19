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

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "utils.h"

static char **read_dir_type(const char *dir, unsigned char d_type);

int
mkdir_recursion(const char *dir)
{
    // mode: 0755
    char buf[256] = {0};
    char *p = NULL;
    mode_t mode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

    int ret = snprintf(buf, 256, "%s", dir);
    if (ret < 0) {
        fprintf(stderr, "Failed to duplicate path: %s\n", strerror(errno));
        return -1;
    }

    /* int len = strlen(buf); */
    /* if (buf[len-1] == '/') { */
    /*     buf[len-1] = '\0'; */
    /* } */

    for (p = buf + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            ret = mkdir(buf, mode);
            if (ret == -1 && errno != EEXIST) {
                fprintf(stderr, "Failed to mkdir: %s - %s\n", buf, strerror(errno));
                return -1;
            }
            *p = '/';
        }
    }
    ret = mkdir(buf, mode);
    if (ret == -1 && errno != EEXIST) {
        fprintf(stderr, "Failed to mkdir: %s - %s\n", buf, strerror(errno));
        return -1;
    }
    return 0;
}

char**
read_dir_files(const char *dir)
{
    return read_dir_type(dir, DT_REG);
}

char**
read_dir_subdirs(const char *dir)
{
    return read_dir_type(dir, DT_DIR);
}

static char**
read_dir_type(const char *dir, unsigned char d_type)
{
    DIR *dp = opendir(dir);
    if (!dp) {
        fprintf(stderr, "Failed to open dir(%s): %s\n", dir, strerror(errno));
        return NULL;
    }

    char **names = NULL;
    int count = 1;
    struct dirent *item = NULL;
    for (; (item = readdir(dp)); ) {
        if (item->d_type != d_type) {
            continue;
        }

        if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0) {
            continue;
        }

        int len = strlen(item->d_name);
        char *value = (char*)calloc(len + 1, sizeof(char));
        if (!value) {
            fprintf(stderr, "Failed to alloc for file: %s\n", strerror(errno));
            continue;
        }
        memcpy(value, item->d_name, len);

        char **tmp = (char**)realloc(names, sizeof(dir)*(count+1));
        if (!tmp) {
            free(value);
            fprintf(stderr, "Failed to realloc for readdir: %s\n", strerror(errno));
            continue;
        }
        names = tmp;
        names[count-1] = value;
        count++;
    }

    if (count != 1) {
        names[count-1] = NULL;
    }

    closedir(dp);
    return names;
}

char*
read_file_contents(const char *file, long *length)
{
    FILE *fp = fopen(file, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open file(%s): %s\n", file, strerror(errno));
        return NULL;
    }

    int ret = fseek(fp, 0, SEEK_END);
    if (ret == -1) {
        fprintf(stderr, "Failed to seek file end: %s\n", strerror(errno));
        fclose(fp);
        return NULL;
    }

    long len = ftell(fp);
    if (len == -1) {
        fprintf(stderr, "Failed to get file position: %s\n", strerror(errno));
        fclose(fp);
        return NULL;
    }

    ret = fseek(fp, 0, SEEK_SET);
    if (ret == -1) {
        fprintf(stderr, "Failed to seek file set: %s\n", strerror(errno));
        fclose(fp);
        return NULL;
    }

    char *contents = (char*)calloc(len+1, sizeof(char));
    if (!contents) {
        fprintf(stderr, "Failed to calloc for file contents: %s\n", strerror(errno));
        fclose(fp);
        return NULL;
    }

    size_t r_len = fread(contents, sizeof(char), len, fp);
    fclose(fp);
    if (r_len != (size_t)len) {
        free(contents);
        fprintf(stderr, "Failed to read file: %s\n", strerror(errno));
        return NULL;
    }

    contents[len] = '\0';
    *length = len;
    return contents;
}

void
free_strv(char **strv)
{
    if (!strv) {
        return;
    }

    int i = 0;
    for (; strv[i]; i++) {
        free(strv[i]);
    }
    free(strv);
}
