package haptix

import "time"

var _ Trigger = (*NopHaptix)(nil)

type NopHaptix struct{}

func NewNopHaptix() *NopHaptix {
	return &NopHaptix{}
}

func (h *NopHaptix) Trigger(_ int, _ time.Duration) error {
	return nil
}

func (h *NopHaptix) Open() error {
	return nil
}

func (h *NopHaptix) Close() error {
	return nil
}
