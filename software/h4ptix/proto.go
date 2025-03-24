package h4ptix

import (
	"encoding"
	"encoding/json"
	"errors"
	"fmt"
)

var _ json.Unmarshaler = (*MsgKind)(nil)
var _ json.Marshaler = (*MsgKind)(nil)
var _ encoding.TextUnmarshaler = (*MsgKind)(nil)

type MsgKind uint8

const (
	MsgKindNone MsgKind = iota
	MsgKindErr
	MsgKindAck
	MsgKindTrigger
)

func (k MsgKind) String() string {
	switch k {
	case MsgKindNone:
		return "none"
	case MsgKindErr:
		return "err"
	case MsgKindAck:
		return "ack"
	case MsgKindTrigger:
		return "trg"
	default:
		return fmt.Sprintf("kind_%d", k)
	}
}

func (k *MsgKind) fromString(v string) error {
	switch v {
	case "none", "":
		*k = MsgKindNone
	case "err":
		*k = MsgKindErr
	case "ack":
		*k = MsgKindAck
	case "trg":
		*k = MsgKindTrigger
	default:
		return fmt.Errorf("unknown msg kind: %s", v)
	}

	return nil
}

func (k MsgKind) MarshalJSON() ([]byte, error) {
	return json.Marshal(k.String())
}

func (k *MsgKind) UnmarshalJSON(in []byte) error {
	var v string
	if err := json.Unmarshal(in, &v); err != nil {
		return err
	}

	return k.fromString(v)
}

func (k *MsgKind) UnmarshalText(v []byte) error {
	return k.fromString(string(v))
}

type HwMsg[T BodyTrigger | BodyAck] struct {
	Kind MsgKind `json:"kind"`
	Body T       `json:"body"`
}

func (m *HwMsg[T]) UnmarshalJSON(in []byte) error {
	var box struct {
		Kind MsgKind         `json:"kind"`
		Body json.RawMessage `json:"body"`
	}

	if err := json.Unmarshal(in, &box); err != nil {
		return fmt.Errorf("invalid json: %w", err)
	}

	switch box.Kind {
	case MsgKindNone:
		return errors.New("no kind in msg")

	case MsgKindAck:
		return nil

	case MsgKindErr:
		var body struct {
			Code ErrorCode `json:"code"`
			Msg  string    `json:"msg"`
		}
		if err := json.Unmarshal(box.Body, &body); err != nil {
			return fmt.Errorf("invalid body: %w", err)
		}

		return NewError(body.Code, body.Msg)

	default:
		return json.Unmarshal(box.Body, &m.Body)
	}
}

type BodyTrigger struct {
	Port     int   `json:"port"`
	Duration int64 `json:"duration,omitempty"`
	Delay    int64 `json:"delay,omitempty"`
}

type BodyAck struct {
}
