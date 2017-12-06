package main

import (
	"fmt"
)

func main() {
	fpInit()
	defer fpExit()
	devices := doGetDeviceList()
	for _, info := range devices {
		fmt.Println(info.index, info.Name, info.DriverId)
	}
}
