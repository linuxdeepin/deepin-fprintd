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
	"os"
	"pkg.deepin.io/lib/log"
)

var logger = log.NewLogger("Test Fprintd")

func main() {
	if len(os.Args) != 2 {
		fmt.Printf("Usage: %s <operation>\n", os.Args[0])
		fmt.Println("Operation: enroll, identify")
		return
	}

	fpInit()
	defer fpExit()

	devs := doGetDeviceList()
	if len(devs) == 0 {
		fmt.Println("No device found")
		return
	}
	fmt.Println("Devices:")
	for _, dev := range devs {
		fmt.Println("\t", dev.Name, dev.DriverId)
	}

	dev := devs[0]
	switch os.Args[1] {
	case "enroll":
		fmt.Println("Will exec 2 times enroll operation using:", dev.Name, dev.DriverId)
		for i := 0; i < 2; i++ {
			err := doEnroll(dev.Name, dev.DriverId, 0, 1, "deepin")
			if err != nil {
				fmt.Printf("Failed to enroll at %d times\n", i)
			}
		}
		return
	case "identify":
		fmt.Println("Will exec identify operation using:", dev.Name, dev.DriverId)
		err := doIdentify(dev.Name, dev.DriverId, 0, "deepin")
		if err != nil {
			fmt.Println("Failed to identify:", err)
		}
		return
	}

	fmt.Println("Unknown operation:", os.Args[1])
}
