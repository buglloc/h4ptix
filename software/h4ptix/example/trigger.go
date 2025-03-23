package main

import (
	"fmt"
	"time"

	"github.com/spf13/cobra"

	"github.com/buglloc/h4ptix/software/h4ptix"
)

var triggerArgs struct {
	port     int
	devPath  string
	duration time.Duration
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
		if triggerArgs.devPath != "" {
			opts = append(opts, h4ptix.WithDevicePath(
				triggerArgs.devPath,
			))
		}

		h, err := h4ptix.NewH4ptix(opts...)
		if err != nil {
			return fmt.Errorf("create haptix: %w", err)
		}

		return h.Trigger(triggerArgs.port, triggerArgs.duration)
	},
}

func init() {
	flags := triggerCmd.PersistentFlags()
	flags.IntVar(&triggerArgs.port, "port", 0, "Port number to trigger on")
	flags.DurationVar(&triggerArgs.duration, "duration", 0, "Trigger duration")
	flags.StringVar(&triggerArgs.devPath, "dev", "", "H4ptiX device to use")
}
