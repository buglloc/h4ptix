package haptix

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

type optDevicePath struct {
	Option
	path string
}

func WithDevicePath(path string) Option {
	return optDevicePath{
		path: path,
	}
}
