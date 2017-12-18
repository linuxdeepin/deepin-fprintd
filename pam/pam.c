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
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#define PAM_SM_AUTH
#include <security/pam_modules.h>

#include "../src/device.h"
#include "../src/storage.h"

#define DEFAULT_MAX_TRIES 3
#define DEFAULT_TIMEOUT 30

#define MAX_TRIES_MATCH "max-tries="
#define TIMEOUT_MATCH "timeout="

#define D(pamh, ...) {\
    if (debug) {\
        char s[1024] = {0}; \
        snprintf(s, 1024, __VA_ARGS__); \
        send_debug_msg(pamh, s); \
    }\
}

static int debug = 0;
static uint32_t max_tries = DEFAULT_MAX_TRIES;
static uint32_t timeout = DEFAULT_TIMEOUT;

static int
send_msg(pam_handle_t *pamh, const char *msg, int style)
{
    const struct pam_message mymsg = {
        .msg_style = style,
        .msg = msg,
    };
    const struct pam_message *msgp = &mymsg;
    const struct pam_conv *pc = NULL;
    struct pam_response *resp = NULL;
    int r = 0;

    r = pam_get_item(pamh, PAM_CONV, (const void**)&pc);
    if (r != PAM_SUCCESS) {
        return -1;
    }

    if (!pc || !pc->conv) {
        return -1;
    }

    r = pc->conv(1, &msgp, &resp, pc->appdata_ptr);
    if (r != PAM_SUCCESS) {
        return -1;
    }
    return 0;
}

static int
send_info_msg(pam_handle_t *pamh, const char *msg)
{
    return send_msg(pamh, msg, PAM_TEXT_INFO);
}

static int
send_err_msg(pam_handle_t *pamh, const char *msg)
{
    return send_msg(pamh, msg, PAM_ERROR_MSG);
}

static void
send_debug_msg(pam_handle_t *pamh, const char *msg)
{
    const char *service;
    const void *item;

    if (pam_get_item(pamh, PAM_SERVICE, &item) != PAM_SUCCESS || !item) {
        service = "<unknown>";
    } else {
        service = item;
    }

    openlog(service, LOG_CONS | LOG_PID, LOG_AUTHPRIV);
    syslog(LOG_AUTHPRIV|LOG_WARNING, "%s(%s): %s", "pam_deepin_fprintd", service, msg);
    closelog();
}

static void
parse_params(pam_handle_t *pamh, int argc, const char **argv)
{
    int i = 0;
    for (; i < argc; i++) {
        if (!argv[i]) {
            continue;
        }

        if (strcmp(argv[i], "debug") == 0) {
            printf("debug on\n");
            debug = 1;
        } else if (strncmp(argv[i], MAX_TRIES_MATCH, strlen(MAX_TRIES_MATCH)) == 0 &&
                   strlen(argv[i]) == strlen(MAX_TRIES_MATCH) + 1) {
            // max tries range: 1 -9
            max_tries = atoi(argv[i] + strlen(MAX_TRIES_MATCH));
            if (max_tries < 1) {
                max_tries = DEFAULT_MAX_TRIES;
            }
            D(pamh, "max_tries specified as: %d", max_tries);
        } else if (strncmp(argv[i], TIMEOUT_MATCH, strlen(TIMEOUT_MATCH)) == 0 &&
                   strlen(argv[i]) <= strlen(TIMEOUT_MATCH) + 2) {
            // timeout range: 10 - 99
            timeout = atoi (argv[i] + strlen (TIMEOUT_MATCH));
            if (timeout < 10) {
                timeout = DEFAULT_TIMEOUT;
            }
            D(pamh, "timeout specified as: %d", timeout);
        }
    }
    return;
}

static Device
get_default_device()
{
    Device dev = {
        .name = NULL,
        .drv_id = -1,
    };

    int dev_num;
    Device* devs = list_devices(&dev_num);
    if (!devs || dev_num == 0) {
        return dev;
    }

    dev.name = strdup(devs[0].name);
    dev.drv_id = devs[0].drv_id;
    free_devices(devs, dev_num);
    return dev;
}

static int
do_identify(pam_handle_t *pamh, const char *username)
{
    // TODO: handle multiple devices. Now ignore it.
    int r = 0;

    send_info_msg(pamh, "[do_verify] init device");
    r = fp_init();
    if (r != 0) {
        send_err_msg(pamh, "[do_verify] failed to init device");
        return PAM_AUTHINFO_UNAVAIL;
    }

    send_info_msg(pamh, "[do_verify] get default device");
    Device dev = get_default_device();
    if (!dev.name) {
        fp_exit();
        send_err_msg(pamh, "[do_verify] failed to get default device");
        return PAM_AUTHINFO_UNAVAIL;
    }

    char _msg[1024] = {0};
    snprintf(_msg, 1024, "[do_verify] get default device(%d - %s) for user: %s", dev.drv_id, dev.name, username);
    send_info_msg(pamh, _msg);
    struct fp_print_data **datas = print_data_user_load(dev.drv_id, username);
    if (!datas) {
        free(dev.name);
        fp_exit();
        memset(_msg, 0, 1024);
        snprintf(_msg, 1024, "[do_verify] failed to get print datas for %s", username);
        send_err_msg(pamh, _msg);
        return PAM_AUTHINFO_UNAVAIL;
    }

    uint32_t i =0;
    for (; i < max_tries; i++) {
        memset(_msg, 0, 1024);
        snprintf(_msg, 1024, "[do_verify] scan your fingerprint: %s", dev.name);
        send_info_msg(pamh, _msg);
        r = identify_datas(dev.name, 0, datas);
        if (r == 0) {
            break;
        }

        memset(_msg, 0, 1024);
        snprintf(_msg, 1024, "[do_verify] failed to identify for %s, try again", username);
        send_info_msg(pamh, _msg);
    }
    free(dev.name);
    print_datas_free(datas);
    fp_exit();

    return r;
}

static int
do_auth(pam_handle_t *pamh, const char* username)
{
    int r = do_identify(pamh, username);
    if (r != 0) {
        return PAM_AUTHINFO_UNAVAIL;
    }

    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags,
                                   int argc, const char **argv)
{
    // TODO:
    // 1. i18n

    const char *rhost = NULL;
    const char *username = NULL;
    int r = 0;

    pam_get_item(pamh, PAM_RHOST, (const void**)&rhost);
    if (rhost && strlen(rhost) > 0) {
        // remote login, such as: SSH
        return PAM_AUTHINFO_UNAVAIL;
    }

    send_info_msg(pamh, "[pam_sm_authenticate] start...");
    r = pam_get_user(pamh, &username, NULL);
    if (r != PAM_SUCCESS) {
        return PAM_AUTHINFO_UNAVAIL;
    }

    char _msg[1024] = {0};
    snprintf(_msg, 1024, "[pam_sm_authenticate] get username: %s", username);
    send_info_msg(pamh, _msg);

    parse_params(pamh, argc, argv);

    return do_auth(pamh, username);
}


PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags,
                              int argc, const char **argv)
{
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_chauthtok(pam_handle_t *pamh, int flags,
                              int argc, const char **argv)
{
    return PAM_SUCCESS;
}
