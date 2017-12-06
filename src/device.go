package main

import (
	"fmt"
	"pkg.deepin.io/lib/dbus"
	"sync"
)

type DeviceInfo struct {
	index      int32
	curUser    string
	userLocker sync.Mutex

	Name     string
	DriverId int32

	EnrollStatus         func(int32, string)
	VerifyFingerSelected func(string)
	VerifyStatus         func(int32, string)
}
type DeviceInfos []*DeviceInfo

func (info *DeviceInfo) Claim(username string) error {
	info.userLocker.Lock()
	if info.curUser != "" {
		return fmt.Errorf("Device has been claimed by %s", info.curUser)
	}
	info.curUser = username
	info.userLocker.Unlock()
	return nil
}

func (info *DeviceInfo) Release() error {
	// TODO: check whether can be released
	info.userLocker.Lock()
	info.curUser = ""
	info.userLocker.Unlock()
	return nil
}

func (info *DeviceInfo) ListEnrolledFingers() ([]string, error) {
	info.userLocker.Lock()
	defer info.userLocker.Unlock()
	if info.curUser == "" {
		return nil, fmt.Errorf("Device has not been claimed")
	}
	return getEnrooledFingers(info.curUser)
}

func (info *DeviceInfo) DeleteEnrolledFinger(finger uint32) error {
	info.userLocker.Lock()
	defer info.userLocker.Unlock()
	return nil
}

func (info *DeviceInfo) CleanEnrolledFinger() error {
	info.userLocker.Lock()
	defer info.userLocker.Unlock()
	return nil
}

func (info *DeviceInfo) EnrollStart(finger int32) error {
	info.userLocker.Lock()
	defer info.userLocker.Unlock()
	if info.curUser == "" {
		return fmt.Errorf("Device has not been claimed")
	}
	doEnroll(info.Name, info.index, info.DriverId, finger, info.curUser)
	return nil
}

func (info *DeviceInfo) EnrollStop() error {
	return nil
}

func (info *DeviceInfo) VerifyStart() error {
	info.userLocker.Lock()
	defer info.userLocker.Unlock()
	if info.curUser == "" {
		return fmt.Errorf("Device has not been claimed")
	}
	enrolled, err := getEnrooledFingers(info.curUser)
	if err != nil {
		return err
	}

	for _, name := range enrolled {
		finger := fingerNameToId(name)
		doEnroll(info.Name, info.index, info.DriverId, finger, info.curUser)
	}
	return nil
}

func (info *DeviceInfo) VerifyStop() error {
	info.userLocker.Lock()
	defer info.userLocker.Unlock()
	return nil
}

func (info *DeviceInfo) GetDBusInfo() dbus.DBusInfo {
	return dbus.DBusInfo{
		Dest:       dbusDest,
		ObjectPath: fmt.Sprintf("%s%d", devDBusPath, info.index),
		Interface:  devDBusIFC,
	}
}

func newDeviceInfo(name string, idx int) *DeviceInfo {
	var info = DeviceInfo{
		Name:    name,
		index:   int32(idx),
		curUser: "",
	}
	return &info
}
