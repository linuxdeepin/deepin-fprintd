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

// #cgo pkg-config: libfprint
// #include <stdlib.h>
// #include "storage.h"
// #include "wrapper.h"
import "C"

import (
	"fmt"
	"io/ioutil"
	"os"
	"pkg.deepin.io/lib/strv"
	"unsafe"
)

type DeviceInfo struct {
	Name     string
	DriverId int32
}
type DeviceInfos []*DeviceInfo

const (
	// supported finger names
	LeftThumb   = C.LEFT_THUMB
	LeftIndex   = C.LEFT_INDEX
	LeftMiddle  = C.LEFT_MIDDLE
	LeftRing    = C.LEFT_RING
	LeftLittle  = C.LEFT_LITTLE
	RightThumb  = C.RIGHT_THUMB
	RightIndex  = C.RIGHT_INDEX
	RightMiddle = C.RIGHT_MIDDLE
	RightRing   = C.RIGHT_RING
	RightLittle = C.RIGHT_LITTLE
)

var _fingerStrv = strv.Strv{
	"left-thumb",
	"left-index",
	"left-middle",
	"left-ring",
	"left-little",
	"right-thumb",
	"right-index",
	"right-middle",
	"right-ring",
	"right-little",
}

func fingerNameToId(name string) int32 {
	switch name {
	case "left-thumb":
		return LeftThumb
	case "left-index":
		return LeftIndex
	case "left-middle":
		return LeftMiddle
	case "left-ring":
		return LeftRing
	case "left-little":
		return LeftLittle
	case "right-thumb":
		return RightThumb
	case "right-index":
		return RightIndex
	case "right-middle":
		return RightMiddle
	case "right-ring":
		return RightRing
	case "right-little":
		return RightLittle
	}
	return -1
}

func fpInit() {
	ret := C.fp_init()
	if ret != 0 {
		fmt.Println("Failed to init fprint, exit")
		os.Exit(-1)
	}
}

func fpExit() {
	C.fp_exit()
}

func (infos DeviceInfos) Names() []string {
	var names []string
	for _, info := range infos {
		names = append(names, info.Name)
	}
	return names
}

func doGetDeviceList() DeviceInfos {
	var length C.int = 0
	var cdevs = C.list_devices(&length)
	if cdevs == nil || length == 0 {
		return nil
	}
	defer C.free_devices(cdevs, length)

	var infos DeviceInfos
	clist := uintptr(unsafe.Pointer(cdevs))
	itemLen := unsafe.Sizeof(*cdevs)
	for i := C.int(0); i < length; i++ {
		cdev := (*C.Device)(unsafe.Pointer(clist + uintptr(i)*(itemLen)))
		infos = append(infos, &DeviceInfo{
			Name:     C.GoString(cdev.name),
			DriverId: int32(cdev.drv_id),
		})
	}

	return infos
}

func doEnroll(devName string, drvId, devIdx, finger int32, username string) error {
	logger.Info("Will enroll:", devName, devIdx, drvId, finger, username)
	var cdevName = C.CString(devName)
	defer C.free(unsafe.Pointer(cdevName))
	var cusername = C.CString(username)
	defer C.free(unsafe.Pointer(cusername))

	ret := C.enroll_finger_wrapper(cdevName, C.int(drvId), C.int(devIdx), C.uint32_t(finger), cusername)
	if ret != 0 {
		return fmt.Errorf("Failed to enroll")
	}
	return nil
}

func doIdentify(devName string, drvId, devIdx int32, username string) error {
	logger.Info("Will identify:", devName, devIdx, drvId, username)
	var cdevName = C.CString(devName)
	defer C.free(unsafe.Pointer(cdevName))
	var cusername = C.CString(username)
	defer C.free(unsafe.Pointer(cusername))

	ret := C.identify_user_wrapper(cdevName, C.int(drvId), C.int(devIdx), cusername)
	if ret != 0 {
		return fmt.Errorf("Failed to identify")
	}
	return nil
}

func doDeleteFinger(finger uint32, drvId int32, username string) error {
	// TODO: using go impl
	var cusername = C.CString(username)
	defer C.free(unsafe.Pointer(cusername))
	ret := C.print_data_delete(C.uint32_t(finger), C.int(drvId), cusername)
	if ret != 0 {
		return fmt.Errorf("Failed to delete finger")
	}
	return nil
}

func doCleanUserFingers(username string) error {
	var dataDir = "/var/lib/deepin/fprintd" + "/" + username
	err := os.RemoveAll(dataDir)
	if err != nil && err != os.ErrNotExist {
		return err
	}
	return nil
}

func getEnrooledFingers(username string, drvId int32) ([]string, error) {
	var dataDir = "/var/lib/deepin/fprintd" + "/" + username
	dirNames, err := ioutil.ReadDir(dataDir)
	if err != nil {
		return nil, err
	}

	var fingerNames []string
	for _, dirName := range dirNames {
		if !dirName.IsDir() {
			continue
		}
		if !_fingerStrv.Contains(dirName.Name()) {
			continue
		}
		if !checkFingerData(fmt.Sprintf("%s/%s/%d", dataDir, dirName.Name(), drvId)) {
			continue
		}
		fingerNames = append(fingerNames, dirName.Name())
	}
	return fingerNames, nil
}

func checkFingerData(dir string) bool {
	names, err := ioutil.ReadDir(dir)
	if err != nil {
		return false
	}

	for _, name := range names {
		file := dir + "/" + name.Name()
		cfile := C.CString(file)
		ret := C.check_print_data_file(cfile)
		C.free(unsafe.Pointer(cfile))
		if ret == 1 {
			return true
		}
	}
	return false
}

//export handleEnrollStatus
func handleEnrollStatus(status C.int) {
	var msg string
	switch status {
	case C.FP_ENROLL_COMPLETE:
		msg = "Enroll complete!"
	case C.FP_ENROLL_FAIL:
		msg = "Enroll failed :(!"
	case C.FP_ENROLL_PASS:
		msg = "Enroll stage passed!"
	case C.FP_ENROLL_RETRY:
		msg = "Didn't quite catch that. Please try again!"
	case C.FP_ENROLL_RETRY_TOO_SHORT:
		msg = "Your swipe was too short, try again!"
	case C.FP_ENROLL_RETRY_CENTER_FINGER:
		msg = "Please center your finger on the sensor, try again!"
	case C.FP_ENROLL_RETRY_REMOVE_FINGER:
		msg = "Scan failed, please remove your finger and try again!"
	}
	fmt.Println("Enroll status:", msg)
	emitSignal("EnrollStatus", int(status), msg)
}

//export handleVerifyStatus
func handleVerifyStatus(status C.int) {
	var msg string
	switch status {
	case C.FP_VERIFY_NO_MATCH:
		msg = "No match!"
	case C.FP_VERIFY_MATCH:
		msg = "Matched!"
	case C.FP_VERIFY_RETRY:
		msg = "Didn't quite catch that. Please try again!"
	case C.FP_VERIFY_RETRY_TOO_SHORT:
		msg = "Your swipe was too short, try again!"
	case C.FP_VERIFY_RETRY_CENTER_FINGER:
		msg = "Please center your finger on the sensor, try again!"
	case C.FP_ENROLL_RETRY_REMOVE_FINGER:
		msg = "Scan failed, please remove your finger and try again!"
	}
	fmt.Println("Identify status:", msg)
	emitSignal("VerifyStatus", int(status), msg)
}
