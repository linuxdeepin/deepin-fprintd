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

#include "wrapper.h"
#include "_cgo_export.h"

static void
handle_enroll_status(int status)
{
	handleEnrollStatus(status);
}

static void
handle_verify_status(int status)
{
	handleVerifyStatus(status);
}

int
enroll_finger_wrapper(char *dev, int drv_id, int dev_idx,
		uint32_t finger, char *username)
{
	enroll_handler = &handle_enroll_status;
	return enroll_finger(dev, drv_id, dev_idx, finger, username);
}

int
identify_user_wrapper(char *dev, int drv_id, int dev_idx,
		char *username)
{
	verify_handler = &handle_verify_status;
	return identify_user(dev, drv_id, dev_idx, username);
}
