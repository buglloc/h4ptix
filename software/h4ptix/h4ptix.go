package h4ptix

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"time"
)

var _ Trigger = (*H4ptix)(nil)

type H4ptix struct {
	dev    Device
	reader *bufio.Scanner
}

func NewH4ptix(opts ...Option) (*H4ptix, error) {
	h := &H4ptix{}
	for _, opt := range opts {
		switch v := opt.(type) {
		case optDevice:
			h.dev = v.dev

		case optDevicePath:
			h.dev = &SerialDevice{
				path: v.path,
			}

		default:
			return nil, fmt.Errorf("unsupported option: %T", opt)
		}
	}

	if h.dev == nil {
		dev, err := FirstSerialDevice()
		if err != nil {
			return nil, fmt.Errorf("unable to find device: %w", err)
		}
		h.dev = dev
	}

	h.reader = bufio.NewScanner(h.dev)
	return h, nil
}

func (h *H4ptix) Open() error {
	return h.dev.Open()
}

func (h *H4ptix) Trigger(port int, duration time.Duration) error {
	var rsp HwMsg[BodyAck]
	return h.roundTrip(
		HwMsg[BodyTrigger]{
			Kind: MsgKindTrigger,
			Body: BodyTrigger{
				Port:     port,
				Duration: duration.Milliseconds(),
			},
		},
		&rsp,
	)
}

func (h *H4ptix) roundTrip(req any, rsp any) error {
	if !h.dev.IsOpened() {
		if err := h.Open(); err != nil {
			return fmt.Errorf("open device: %w", err)
		}
		defer func() { _ = h.dev.Close() }()
	}

	reqBytes, err := json.Marshal(req)
	if err != nil {
		return fmt.Errorf("invalid request: %w", err)
	}

	reqBytes = append(reqBytes, '\n')
	n, err := h.dev.Write(reqBytes)
	if err != nil {
		return fmt.Errorf("write request: %w", err)
	}

	if n != len(reqBytes) {
		return fmt.Errorf("unexpected writed len: %d (expected) != %d (actual)", len(reqBytes), n)
	}

	if !h.reader.Scan() {
		if h.reader.Err() != nil {
			return fmt.Errorf("read response: %w", h.reader.Err())
		}

		return fmt.Errorf("read response: %w", io.EOF)
	}

	b := h.reader.Bytes()
	err = json.Unmarshal(b, rsp)
	if err != nil {
		return fmt.Errorf("invalid response: %w", err)
	}

	return nil
}

func (h *H4ptix) Close() error {
	return h.dev.Close()
}
