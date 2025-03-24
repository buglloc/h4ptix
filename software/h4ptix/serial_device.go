package h4ptix

import (
	"errors"
	"fmt"
	"strings"
	"time"

	"go.bug.st/serial"
	"go.bug.st/serial/enumerator"
)

const (
	VID  = "1209"
	PID  = "F601"
	Name = "H4ptiX"

	DefaultReadTimeout = 30 * time.Second
)

var _ Device = (*SerialDevice)(nil)

type SerialDevice struct {
	path string
	port serial.Port
}

func FirstSerialDevice() (*SerialDevice, error) {
	devices, err := SerialDevices()
	if err != nil {
		return nil, err
	}

	if len(devices) == 0 {
		return nil, errors.New("not found")
	}

	return &devices[0], nil
}

func SerialDevices() ([]SerialDevice, error) {
	ports, err := enumerator.GetDetailedPortsList()
	if err != nil {
		return nil, fmt.Errorf("list detailed ports: %w", err)
	}

	var out []SerialDevice
	for _, port := range ports {
		if !port.IsUSB {
			continue
		}

		if !strings.EqualFold(port.VID, VID) {
			continue
		}

		if !strings.EqualFold(port.PID, PID) {
			continue
		}

		out = append(out, SerialDevice{
			path: port.Name,
		})
	}

	return out, nil
}

func (d *SerialDevice) Path() string {
	return d.path
}

func (d *SerialDevice) Open() error {
	if d.port != nil {
		return nil
	}

	serialPort, err := serial.Open(d.path, &serial.Mode{
		BaudRate: 115200,
	})
	if err != nil {
		return fmt.Errorf("open serial port %s: %w", d.path, convertSerialErr(err))
	}

	_ = serialPort.SetReadTimeout(DefaultReadTimeout)
	d.port = serialPort
	return nil
}

func (d *SerialDevice) IsOpened() bool {
	return d.port != nil
}

func (d *SerialDevice) Close() error {
	err := d.port.Close()
	d.port = nil
	return convertSerialErr(err)
}

func (d *SerialDevice) Write(p []byte) (n int, err error) {
	n, err = d.port.Write(p)
	return n, convertSerialErr(err)
}

func (d *SerialDevice) Read(p []byte) (n int, err error) {
	n, err = d.port.Read(p)
	return n, convertSerialErr(err)
}

func convertSerialErr(err error) error {
	if err == nil {
		return nil
	}

	var portErr *serial.PortError
	if !errors.As(err, &portErr) {
		return err
	}

	switch portErr.Code() {
	case serial.PortNotFound, serial.PortClosed:
		return &Error{
			Code: ErrorCodeNoDev,
			Msg:  portErr.Error(),
		}

	case serial.PermissionDenied:
		return &Error{
			Code: ErrorCodeNoPerm,
			Msg:  portErr.Error(),
		}

	case serial.PortBusy:
		return &Error{
			Code: ErrorCodeDevBusy,
			Msg:  portErr.Error(),
		}

	default:
		return err
	}
}
