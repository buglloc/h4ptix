package h4ptix

import (
	"fmt"

	"github.com/buglloc/h4ptix/software/h4ptix/rpcpb"
)

type ErrorCode rpcpb.ErrCode

// fwd
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
	Code rpcpb.ErrCode
	Msg  string
}

func NewError(code rpcpb.ErrCode, msg string) *Error {
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
		return false
	case ErrorCodeNoDev:
		return true
	}

	return false
}
