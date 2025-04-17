package h4ptix

import (
	"errors"
	"fmt"

	"github.com/buglloc/usbhid"
)

const (
	VID          = 0x1209
	PID          = 0xF601
	HIDUsagePage = 0xFF
	HIDUsage     = 0xCE
)

type Device struct {
	dev *usbhid.Device
}

func FirstDevice() (*Device, error) {
	devices, err := Enumerate()
	if err != nil {
		return nil, err
	}

	if len(devices) == 0 {
		return nil, errors.New("not found")
	}

	return devices[0], nil
}

func Enumerate(filters ...func(d *usbhid.Device) bool) ([]*Device, error) {
	devices, err := usbhid.Enumerate(
		usbhid.WithVidFilter(VID),
		usbhid.WithPidFilter(PID),
		usbhid.WithDeviceFilterFunc(func(d *usbhid.Device) bool {
			if d.UsagePage() != HIDUsagePage {
				return false
			}

			if d.Usage() != HIDUsage {
				return false
			}

			for _, filter := range filters {
				if !filter(d) {
					return false
				}
			}

			return true
		}),
	)
	if err != nil {
		return nil, fmt.Errorf("enumerate HID devices: %w", err)
	}

	var out []*Device
	for _, dev := range devices {
		out = append(out, &Device{
			dev: dev,
		})
	}

	return out, nil
}

func (d *Device) Path() string {
	return d.dev.Path()
}

func (d *Device) Open() error {
	return convertHIDErr(d.dev.Open(true))
}

func (d *Device) IsOpen() bool {
	return d.dev.IsOpen()
}

func (d *Device) Close() error {
	return convertHIDErr(d.dev.Close())
}

func (d *Device) Send(report []byte) error {
	return d.dev.SetOutputReport(0, report)
}

func (d *Device) Recv() ([]byte, error) {
	id, data, err := d.dev.GetInputReport()
	if id != 0 {
		return nil, fmt.Errorf("unexpected report id: 0 (expected) != %d", id)
	}

	return data, err
}

func convertHIDErr(err error) error {
	if err == nil {
		return nil
	}

	switch {
	case errors.Is(err, usbhid.ErrNoDeviceFound):
		return &Error{
			Code: ErrorCodeNoDev,
			Msg:  err.Error(),
		}

	case errors.Is(err, usbhid.ErrDeviceLocked):
		return &Error{
			Code: ErrorCodeDevBusy,
			Msg:  err.Error(),
		}

	default:
		return err
	}
}
