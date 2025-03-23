package haptix

import (
	"fmt"
)

type HwErrorCode uint32

const (
	HwErrorCodeNone         HwErrorCode = 0x00
	HwErrorCodeNotSupported HwErrorCode = 0x01
	HwErrorCodeInternal     HwErrorCode = 0x02
	HwErrorCodeInvalidReq   HwErrorCode = 0x03
	HwErrorCodePortInvalid  HwErrorCode = 0x04
	HwErrorCodePortBusy     HwErrorCode = 0x05
)

type HwError struct {
	Code HwErrorCode
	Msg  string
}

func NewError(code HwErrorCode, msg string) *HwError {
	return &HwError{
		Code: code,
		Msg:  msg,
	}
}

func (e *HwError) Is(err error) bool {
	o, ok := err.(*HwError)
	if !ok {
		return false
	}

	if e == nil && o == nil {
		return true
	}
	if e == nil || o == nil {
		return false
	}

	return e.Code == o.Code
}

func (e *HwError) Error() string {
	if e.Msg == "" {
		return fmt.Sprintf("HwError[%d]", e.Code)
	}
	return fmt.Sprintf("HwError[%d]: %s", e.Code, e.Msg)
}

func (e *HwError) IsPermanent() bool {
	switch e.Code {
	case HwErrorCodeNone, HwErrorCodeInternal, HwErrorCodePortBusy:
		return true
	}

	return false
}
