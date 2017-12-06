package main

// #cgo pkg-config: libfprint
// #include <stdlib.h>
// #include "device.h"
import "C"

import (
	"fmt"
	"io/ioutil"
	"os"
	"pkg.deepin.io/lib/strv"
	"unsafe"
)

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
			index:    int32(i),
			Name:     C.GoString(cdev.name),
			DriverId: int32(cdev.drv_id),
		})
	}

	return infos
}

func doEnroll(devName string, devIdx, drvId, finger int32, username string) {
	var cdevName = C.CString(devName)
	defer C.free(unsafe.Pointer(cdevName))
	var cusername = C.CString(username)
	defer C.free(unsafe.Pointer(cusername))

	ret := C.enroll_finger(cdevName, C.int(devIdx), C.int(drvId), C.uint32_t(finger), cusername)
	if ret != 0 {
		return
	}
}

func doIdentify(devName string, devIdx, drvId, finger int32, username string) {
	var cdevName = C.CString(devName)
	defer C.free(unsafe.Pointer(cdevName))
	var cusername = C.CString(username)
	defer C.free(unsafe.Pointer(cusername))

	ret := C.identify_finger(cdevName, C.int(devIdx), C.int(drvId), C.uint32_t(finger), cusername)
	if ret != 0 {
		return
	}
}

func getEnrooledFingers(username string) ([]string, error) {
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
		if !checkFingerData(dataDir + "/" + dirName.Name()) {
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
