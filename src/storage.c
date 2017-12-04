#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdint.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <glib.h>

#include "storage.h"
#include "utils.h"

#define PRINT_DATA_DIR "/var/lib/deepin/fprintd/"

static char *finger_to_str(enum fp_finger finger);
static char *gen_print_data_id();
static char *get_print_data_dir(enum fp_finger finger, const char *username);
static char *get_print_data_path(enum fp_finger finger, const char *username);
static int mkdir_recursion(const char *path);
static struct fp_print_data *load_from_file(const char *path);
static char **read_dir_files(const char *dir);

int
print_data_save(struct fp_print_data *data, enum fp_finger finger,
                    const char *username)
{
    char *path = get_print_data_path(finger, username);
    if (!path) {
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

    size_t ret = fwrite(buf, sizeof(char), len, fp);
    free(buf);
    if (ret < 1) {
        fclose(fp);
        fprintf(stderr, "Failed to write print data: %s\n", strerror(errno));
        return -1;
    }

    fflush(fp);
    fclose(fp);

    return 0;
}

int
print_data_delete(enum fp_finger finger, const char *username)
{
    char *dir = get_print_data_dir(finger, username);
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
print_data_load(enum fp_finger finger, const char *username)
{
	char *dir = get_print_data_path(finger, username);
	if (!dir) {
		return NULL;
	}

	char **names = read_dir_files(dir);
	if (!names) {
		free(dir);
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

		struct fp_print_data *data = load_from_file(path);
		if (!data) {
			continue;
		}
		struct fp_print_data **tmp = (struct fp_print_data**)realloc(datas, sizeof(data) * (count+1));
		if (!tmp) {
			fprintf(stderr, "Failed to realloc for load data: %s\n", strerror(errno));
			continue;
		}

		datas = tmp;
		datas[count-1] = data;
		count++;
	}
	free(dir);
	free_strv(names);
	datas[count-1] = NULL;

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
get_print_data_path(enum fp_finger finger, const char *username)
{
    char *dir = get_print_data_dir(finger, username);
    if (!dir) {
        return NULL;
    }

    char *id = gen_print_data_id();
    if (!id) {
        free(dir);
        return NULL;
    }

    int dir_len = strlen(dir);
    int id_len = strlen(id);
    char *tmp = (char*)realloc(dir, sizeof(char) * (dir_len + 1+ id_len + 1));
    if (!tmp) {
        free(dir);
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
get_print_data_dir(enum fp_finger finger, const char *username)
{
	int ret;
    char *finger_name = finger_to_str(finger);
    char *dir = (char*)calloc(sizeof(PRINT_DATA_DIR) +
                               sizeof(username) +
                               sizeof(finger_name) + 2, sizeof(char));
    if (!dir) {
        fprintf(stderr, "Failed to allocate memory for path: %s\n", strerror(errno));
        goto failed;
    }

    ret = sprintf(dir, "%s%s/%s", PRINT_DATA_DIR, username, finger_name);
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

static struct fp_print_data*
load_from_file(const char *path)
{
	gsize length;
	char *contents;
	GError *error = NULL;

	g_file_get_contents(path, &contents, &length, &error);
	if (error) {
		fprintf(stderr, "Failed to get file(%s) contents: %s\n", 
				path, error->message);
		g_error_free(error);
		return NULL;
	}

	struct fp_print_data *fdata = NULL;
	fdata = fp_print_data_from_data((unsigned char *)contents, length);
	return fdata;
}

static int
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
            if (ret == -1) {
                fprintf(stderr, "Failed to mkdir: %s - %s\n", buf, strerror(errno));
                return -1;
            }
            *p = '/';
        }
    }
    ret = mkdir(buf, mode);
    if (ret == -1) {
        fprintf(stderr, "Failed to mkdir: %s - %s\n", buf, strerror(errno));
    }
    return ret;
}

static char**
read_dir_files(const char *dir)
{
	DIR *dp = opendir(dir);
	if (!dp) {
		fprintf(stderr, "Failed to open dir: %s\n", strerror(errno));
		return NULL;
	}

	char **names;
	int count = 1;
	struct dirent *item = NULL;
	for (; (item = readdir(dp)); ) {
		if (item->d_type != DT_REG) {
			continue;
		}
		char **tmp = (char**)realloc(names, sizeof(char*)*(count+1));
		if (!tmp) {
			fprintf(stderr, "Failed to realloc for readdir: %s\n", strerror(errno));
			continue;
		}

		names = tmp;
		names[count-1] = strdup(item->d_name);
		count++;
	}

	if (count == 1) {
		return NULL;
	}
	names[count-1] = NULL;

	return names;
}
