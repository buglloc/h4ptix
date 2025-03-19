package haptix

import "time"

type Trigger interface {
	Trigger(pin int, delay time.Duration) error
}
