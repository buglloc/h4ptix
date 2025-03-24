package h4ptix

import "time"

type Trigger interface {
	Trigger(req TriggerReq) error
}

type TriggerReq struct {
	Port     int
	Duration time.Duration
	Delay    time.Duration
}
