package h4ptix

type Option interface {
	isOption()
}

type optDevice struct {
	Option
	dev *Device
}

func WithDevice(dev *Device) Option {
	return optDevice{
		dev: dev,
	}
}

type optDeviceSerial struct {
	Option
	serial string
}

func WithDeviceSerial(serial string) Option {
	return optDeviceSerial{
		serial: serial,
	}
}
