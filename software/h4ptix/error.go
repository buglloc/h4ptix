package h4ptix

import (
	"fmt"

	"github.com/buglloc/h4ptix/software/h4ptix/rpcpb"
)

// fwd
type ErrCode = rpcpb.ErrCode

const (
	ErrorCodeNone           = rpcpb.ErrCode_ErrCodeNone
	ErrorCodeInvalidCommand = rpcpb.ErrCode_ErrCodeInvalidCommand
	ErrorCodeNotSupported   = rpcpb.ErrCode_ErrCodeNotSupported
	ErrorCodeInternal       = rpcpb.ErrCode_ErrCodeInternal
	ErrorCodeInvalidReq     = rpcpb.ErrCode_ErrCodeInvalidReq
	ErrorCodePortInvalid    = rpcpb.ErrCode_ErrCodePortInvalid
	ErrorCodePortBusy       = rpcpb.ErrCode_ErrCodePortBusy

	ErrorCodeNoDev   = rpcpb.ErrCode_ErrCodeNoDev
	ErrorCodeDevBusy = rpcpb.ErrCode_ErrCodeDevBusy
)

type Error struct {
	Code ErrCode
	Msg  string
}

func NewError(code ErrCode, msg string) *Error {
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
		return fmt.Sprintf("HwError[%d]: %s", e.Code, errCodeString(e.Code))
	}

	return fmt.Sprintf("HwError[%d]: %s: %s", e.Code, errCodeString(e.Code), e.Msg)
}

func (e *Error) IsPermanent() bool {
	switch e.Code {
	case ErrorCodeNone, ErrorCodeInternal, ErrorCodePortBusy:
		return false
	case ErrorCodeNoDev:
		return true
	}

	return false
}

func errCodeString(c ErrCode) string {
	switch c {
	case ErrorCodeNone:
		return "none"
	case ErrorCodeInvalidCommand:
		return "invalid command"
	case ErrorCodeNotSupported:
		return "not supported"
	case ErrorCodeInternal:
		return "internal error"
	case ErrorCodeInvalidReq:
		return "invalid request"
	case ErrorCodePortInvalid:
		return "invalid port"
	case ErrorCodePortBusy:
		return "port busy"
	case ErrorCodeNoDev:
		return "h4ptix device not found"
	case ErrorCodeDevBusy:
		return "h4ptix device busy"
	default:
		return "unknown"
	}
}
