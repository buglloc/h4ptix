package main

import (
	"fmt"
	"log"
	"math/rand"
	"os"
	"os/signal"
	"slices"
	"syscall"
	"time"

	"github.com/spf13/cobra"

	"github.com/buglloc/h4ptix/software/h4ptix"
)

var partyArgs struct {
	ports    []int
	pause    time.Duration
	duration time.Duration
	delay    time.Duration
}

var partyCmd = &cobra.Command{
	Use:           "party",
	SilenceUsage:  true,
	SilenceErrors: true,
	Short:         "Triggers party",
	RunE: func(_ *cobra.Command, _ []string) error {
		if len(partyArgs.ports) == 0 {
			return fmt.Errorf("must specify a ports for party")
		}

		h, err := h4ptix.NewH4ptix()
		if err != nil {
			return fmt.Errorf("create haptix: %w", err)
		}

		stopChan := make(chan os.Signal, 1)
		signal.Notify(stopChan, syscall.SIGINT, syscall.SIGTERM)

		for {
			select {
			case <-stopChan:
				log.Println("shutting down")
				return nil
			default:
			}

			ports := slices.Clone(partyArgs.ports)
			rand.Shuffle(len(ports), func(i, j int) {
				ports[i], ports[j] = ports[j], ports[i]
			})

			for _, port := range ports {
				err := h.Trigger(h4ptix.TriggerReq{
					Port:     port,
					Duration: partyArgs.duration,
					Delay:    partyArgs.delay,
				})
				if err != nil {
					log.Printf("unable to trigger port %d: %v\n", port, err)
				} else {
					log.Printf("triggered port %d\n", port)
				}

				time.Sleep(partyArgs.pause)
			}
		}
	},
}

func init() {
	flags := partyCmd.PersistentFlags()
	flags.IntSliceVar(&partyArgs.ports, "port", []int{1, 2}, "Ports number to trigger on")
	flags.DurationVar(&partyArgs.pause, "pause", 1000*time.Millisecond, "Pause between triggers")
	flags.DurationVar(&partyArgs.duration, "duration", 500*time.Millisecond, "Trigger duration")
	flags.DurationVar(&partyArgs.delay, "delay", 0, "Trigger delay")
}
