package main

import (
	"fmt"
	"time"

	"github.com/spf13/cobra"

	"github.com/buglloc/h4ptix/software/h4ptix"
)

var triggerArgs struct {
	port      int
	devSerial string
	duration  time.Duration
	delay     time.Duration
}

var triggerCmd = &cobra.Command{
	Use:           "trigger",
	SilenceUsage:  true,
	SilenceErrors: true,
	Short:         "Trigger port",
	RunE: func(_ *cobra.Command, _ []string) error {
		if triggerArgs.port <= 0 {
			return fmt.Errorf("must specify a port to trigger on")
		}

		var opts []h4ptix.Option
		if triggerArgs.devSerial != "" {
			opts = append(opts, h4ptix.WithDeviceSerial(
				triggerArgs.devSerial,
			))
		}

		h, err := h4ptix.NewH4ptix(opts...)
		if err != nil {
			return fmt.Errorf("create haptix: %w", err)
		}

		return h.Trigger(h4ptix.TriggerReq{
			Port:     triggerArgs.port,
			Duration: triggerArgs.duration,
			Delay:    triggerArgs.delay,
		})
	},
}

func init() {
	flags := triggerCmd.PersistentFlags()
	flags.IntVar(&triggerArgs.port, "port", 0, "Port number to trigger on")
	flags.DurationVar(&triggerArgs.duration, "duration", 0, "Trigger duration")
	flags.DurationVar(&triggerArgs.delay, "delay", 0, "Trigger delay")
	flags.StringVar(&triggerArgs.devSerial, "serial", "", "H4ptiX device serial to use")
}
