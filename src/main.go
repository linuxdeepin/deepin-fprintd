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
