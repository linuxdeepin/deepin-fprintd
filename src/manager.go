package main

import (
	"pkg.deepin.io/lib/dbus"
)

const (
	dbusDest    = "com.deepin.daemon.Fprintd"
	dbusPath    = "/com/deepin/daemon/Fprintd"
	dbusIFC     = dbusDest
	devDBusPath = "/com/deepin/daemon/Device/"
	devDBusIFC  = dbusIFC + ".Device"
)

type Manager struct{}

func (*Manager) GetDeviceList() DeviceInfos {
	return doGetDeviceList()
}

func (*Manager) GetDefaultDevice() *DeviceInfo {
	return nil
}

func (*Manager) GetDBusInfo() dbus.DBusInfo {
	return dbus.DBusInfo{
		Dest:       dbusDest,
		ObjectPath: dbusPath,
		Interface:  dbusIFC,
	}
}
