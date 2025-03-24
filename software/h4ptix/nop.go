package h4ptix

var _ Trigger = (*NopHaptix)(nil)

type NopHaptix struct{}

func NewNopHaptix() *NopHaptix {
	return &NopHaptix{}
}

func (h *NopHaptix) Trigger(_ TriggerReq) error {
	return nil
}

func (h *NopHaptix) Open() error {
	return nil
}

func (h *NopHaptix) Close() error {
	return nil
}
