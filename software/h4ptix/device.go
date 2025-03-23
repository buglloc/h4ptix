package h4ptix

import "io"

type Device interface {
	Path() string
	Open() error
	IsOpened() bool
	io.ReadWriteCloser
}
