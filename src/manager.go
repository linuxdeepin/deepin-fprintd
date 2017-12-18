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
	"fmt"
	"pkg.deepin.io/lib/dbus"
	"sync"
)

const (
	dbusDest = "com.deepin.daemon.Fprintd"
	dbusPath = "/com/deepin/daemon/Fprintd"
	dbusIFC  = dbusDest
)

type Manager struct {
	devIdx     int32
	idxLocker  sync.Mutex
	working    bool
	workLocker sync.Mutex

	infos DeviceInfos

	EnrollStatus func(int32, string)
	VerifyStatus func(int32, string)
}

var _m *Manager

func newManager() *Manager {
	var m = Manager{
		infos:   doGetDeviceList(),
		devIdx:  0,
		working: false,
	}
	return &m
}

func (m *Manager) setDevIdx(idx int32) error {
	m.idxLocker.Lock()
	defer m.idxLocker.Unlock()
	if idx >= int32(len(m.infos)) || idx < 0 {
		return fmt.Errorf("Invalid device index: %d", idx)
	}
	m.devIdx = idx
	return nil
}

func (m *Manager) checkDevIdx() error {
	m.idxLocker.Lock()
	defer m.idxLocker.Unlock()
	if m.devIdx >= int32(len(m.infos)) {
		return fmt.Errorf("Invalid device index: %d", m.devIdx)
	}
	return nil
}

func (m *Manager) setWorking(v bool) {
	m.workLocker.Lock()
	m.working = v
	m.workLocker.Unlock()
}

func (m *Manager) getWorking() bool {
	m.workLocker.Lock()
	defer m.workLocker.Unlock()
	return m.working
}

func (*Manager) GetDBusInfo() dbus.DBusInfo {
	return dbus.DBusInfo{
		Dest:       dbusDest,
		ObjectPath: dbusPath,
		Interface:  dbusIFC,
	}
}

func emitSignal(name string, status int, msg string) {
	if _m == nil {
		return
	}
	err := dbus.Emit(_m, name, int32(status), msg)
	if err != nil {
		fmt.Println("Failed to emit signal:", name, status, msg)
	}
}
