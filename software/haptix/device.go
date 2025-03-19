package haptix

import (
	"errors"
	"fmt"
	"io"
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

var _ io.ReadWriteCloser = (*Device)(nil)

type Device struct {
	path        string
	readTimeout time.Duration
	port        serial.Port
}

func FirstDevice() (*Device, error) {
	devices, err := Devices()
	if err != nil {
		return nil, err
	}

	if len(devices) == 0 {
		return nil, errors.New("not found")
	}

	return &devices[0], nil
}

func Devices() ([]Device, error) {
	ports, err := enumerator.GetDetailedPortsList()
	if err != nil {
		return nil, fmt.Errorf("list detailed ports: %w", err)
	}

	var out []Device
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

		out = append(out, Device{
			path: port.Name,
		})
	}

	return out, nil
}

func (d *Device) Path() string {
	return d.path
}

func (d *Device) Open() error {
	if d.port != nil {
		return nil
	}

	serialPort, err := serial.Open(d.path, &serial.Mode{
		BaudRate: 115200,
	})
	if err != nil {
		return fmt.Errorf("open serial port %s: %w", d.path, err)
	}

	_ = serialPort.SetReadTimeout(DefaultReadTimeout)
	d.port = serialPort
	return nil
}

func (d *Device) IsOpened() bool {
	return d.port != nil
}

func (d *Device) Close() error {
	err := d.port.Close()
	d.port = nil
	return err
}

func (d *Device) Write(p []byte) (n int, err error) {
	return d.port.Write(p)
}

func (d *Device) Read(p []byte) (n int, err error) {
	return d.port.Read(p)
}
