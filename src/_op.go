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
