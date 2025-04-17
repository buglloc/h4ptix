package h4ptix

import (
	"fmt"
	"math/rand/v2"

	"github.com/buglloc/usbhid"
	"google.golang.org/protobuf/proto"

	"github.com/buglloc/h4ptix/software/h4ptix/rpcpb"
)

var _ Trigger = (*H4ptix)(nil)

type H4ptix struct {
	dev *Device
}

func NewH4ptix(opts ...Option) (*H4ptix, error) {
	h := &H4ptix{}
	for _, opt := range opts {
		switch v := opt.(type) {
		case optDevice:
			h.dev = v.dev

		case optDeviceSerial:
			devs, err := Enumerate(func(d *usbhid.Device) bool {
				return d.SerialNumber() == v.serial
			})
			if err != nil {
				return nil, fmt.Errorf("device enumeration: %w", err)
			}

			if len(devs) == 0 {
				return nil, fmt.Errorf("device with serial %q not found", v.serial)
			}

			if len(devs) > 1 {
				return nil, fmt.Errorf("more than one device with serial %q was found", v.serial)
			}

			h.dev = devs[0]

		default:
			return nil, fmt.Errorf("unsupported option: %T", opt)
		}
	}

	if h.dev == nil {
		dev, err := FirstDevice()
		if err != nil {
			return nil, fmt.Errorf("unable to find device: %w", err)
		}
		h.dev = dev
	}

	return h, nil
}

func (h *H4ptix) Open() error {
	return h.dev.Open()
}

func (h *H4ptix) Close() error {
	return h.dev.Close()
}

func (h *H4ptix) Location() string {
	return h.dev.Location()
}

func (h *H4ptix) Trigger(req TriggerReq) error {
	var rsp rpcpb.Rsp
	return h.roundTrip(
		&rpcpb.Req{
			Id: rand.Uint32(),
			Payload: &rpcpb.Req_Trigger{
				Trigger: &rpcpb.Trigger{
					Port:       uint32(req.Port),
					DurationMs: uint32(req.Duration.Milliseconds()),
					DelayMs:    uint32(req.Delay.Milliseconds()),
				},
			},
		},
		&rsp,
	)
}

func (h *H4ptix) roundTrip(req *rpcpb.Req, rsp *rpcpb.Rsp) error {
	reqBytes, err := proto.Marshal(req)
	if err != nil {
		return fmt.Errorf("marshal req: %w", err)
	}

	if !h.dev.IsOpen() {
		if err := h.Open(); err != nil {
			return fmt.Errorf("open device: %w", err)
		}
		defer func() { _ = h.dev.Close() }()
	}

	err = h.dev.Send(reqBytes)
	if err != nil {
		return fmt.Errorf("send req: %w", err)
	}

	rspBytes, err := h.dev.Recv()
	if err != nil {
		return fmt.Errorf("recv rsp: %w", err)
	}

	err = proto.Unmarshal(rspBytes, rsp)
	if err != nil {
		return fmt.Errorf("invalid response: %w", err)
	}

	if req.Id != rsp.Id {
		return fmt.Errorf("invalid req id in response: %d (expected) != %d (actual)", req.Id, rsp.Id)
	}

	if rspErr, ok := rsp.Payload.(*rpcpb.Rsp_Err); ok {
		return NewError(rspErr.Err.Code, fmt.Sprintf("request id: %d", req.Id))
	}

	return nil
}
