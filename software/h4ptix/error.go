package h4ptix

import (
	"fmt"
)

type ErrorCode uint32

const (
	ErrorCodeNone         ErrorCode = 0x00
	ErrorCodeNotSupported ErrorCode = 0x01
	ErrorCodeInternal     ErrorCode = 0x02
	ErrorCodeInvalidReq   ErrorCode = 0x03
	ErrorCodePortInvalid  ErrorCode = 0x04
	ErrorCodePortBusy     ErrorCode = 0x05

	ErrorCodeNoDev   ErrorCode = 0xFF01
	ErrorCodeNoPerm  ErrorCode = 0xFF02
	ErrorCodeDevBusy ErrorCode = 0xFF03
)

type Error struct {
	Code ErrorCode
	Msg  string
}

func NewError(code ErrorCode, msg string) *Error {
	return &Error{
		Code: code,
		Msg:  msg,
	}
}

func (e *Error) Is(err error) bool {
	o, ok := err.(*Error)
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

func (e *Error) Error() string {
	if e.Msg == "" {
		return fmt.Sprintf("HwError[%d]", e.Code)
	}
	return fmt.Sprintf("HwError[%d]: %s", e.Code, e.Msg)
}

func (e *Error) IsPermanent() bool {
	switch e.Code {
	case ErrorCodeNone, ErrorCodeInternal, ErrorCodePortBusy:
		return true
	case ErrorCodeNoDev, ErrorCodeNoPerm:
		return true
	}

	return false
}
