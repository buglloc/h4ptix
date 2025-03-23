package h4ptix

import "time"

type Trigger interface {
	Trigger(pin int, delay time.Duration) error
}
