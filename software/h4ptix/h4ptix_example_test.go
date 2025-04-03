package h4ptix_test

import (
	"log"

	"github.com/buglloc/h4ptix/software/h4ptix"
)

func ExampleH4ptix_Trigger() {
	h, err := h4ptix.NewH4ptix()
	if err != nil {
		log.Fatalf("create haptix: %v\n", err)
	}

	usbPort := 1
	err = h.Trigger(h4ptix.TriggerReq{
		Port: usbPort,
	})
	if err != nil {
		log.Fatalf("ooops, ship happens: %v\n", err)
	}
}
