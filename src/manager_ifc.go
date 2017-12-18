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
)

func (m *Manager) GetDeviceList() []string {
	return m.infos.Names()
}

func (m *Manager) GetDefaultDevice() (int32, error) {
	err := m.checkDevIdx()
	if err != nil {
		return -1, err
	}
	return m.devIdx, nil
}

func (m *Manager) SetDefaultDevice(idx int32) error {
	return m.setDevIdx(idx)
}

func (m *Manager) UnsetDefaultDevice() error {
	if m.getWorking() {
		return fmt.Errorf("Enroll or verify operation being performed")
	}

	return m.setDevIdx(0)
}

func (m *Manager) ListEnrolledFingers(username string) ([]string, error) {
	err := m.checkDevIdx()
	if err != nil {
		return nil, err
	}
	return getEnrooledFingers(username, m.infos[m.devIdx].DriverId)
}

func (m *Manager) DeleteEnrolledFinger(finger uint32, username string) error {
	err := m.checkDevIdx()
	if err != nil {
		return err
	}

	return doDeleteFinger(finger, m.infos[m.devIdx].DriverId, username)
}

func (m *Manager) CleanEnrolledFinger(username string) error {
	return doCleanUserFingers(username)
}

func (m *Manager) EnrollStart(finger int32, username string) error {
	if m.getWorking() {
		return fmt.Errorf("Enroll or verify operation being performed")
	}

	m.setWorking(true)
	err := m.checkDevIdx()
	if err != nil {
		m.setWorking(false)
		return err
	}

	dev := m.infos[m.devIdx]
	go func() {
		err := doEnroll(dev.Name, dev.DriverId, m.devIdx, finger, username)
		m.setWorking(false)
		if err != nil {
			logger.Warning("Failed to enroll:", finger, username, err)
		}
	}()
	return nil
}

func (m *Manager) EnrollStop() error {
	if !m.getWorking() {
		return fmt.Errorf("No enroll or verify operation being performed")
	}

	// TODO: close dev
	return nil
}

func (m *Manager) VerifyStart(username string) error {
	if m.getWorking() {
		return fmt.Errorf("Enroll or verify operation being performed")
	}

	m.setWorking(true)
	err := m.checkDevIdx()
	if err != nil {
		m.setWorking(false)
		return err
	}

	dev := m.infos[m.devIdx]
	go func() {
		err := doIdentify(dev.Name, dev.DriverId, m.devIdx, username)
		m.setWorking(false)
		if err != nil {
			logger.Warning("Failed to verify:", username, err)
		}
	}()

	return nil
}

func (m *Manager) VerifyStop() error {
	if !m.getWorking() {
		return fmt.Errorf("No enroll or verify operation being performed")
	}

	// TODO: close dev
	return nil
}
