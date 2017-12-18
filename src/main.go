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

package main

import (
	"pkg.deepin.io/lib/dbus"
	"pkg.deepin.io/lib/log"
)

var logger = log.NewLogger(dbusDest)

func Start() error {
	if _m != nil {
		return nil
	}

	fpInit()
	_m = newManager()
	err := dbus.InstallOnSystem(_m)
	if err != nil {
		logger.Error("Failed to install fprintd system bus:", err)
		return err
	}
	dbus.DealWithUnhandledMessage()
	return nil
}

func Stop() error {
	if _m == nil {
		return nil
	}
	fpExit()
	return nil
}

func main() {
	Start()
	defer Stop()

	err := dbus.Wait()
	if err != nil {
		logger.Error("Lost dbus connection:", err)
		return
	}
}
