package main

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"
)

var rootCmd = &cobra.Command{
	Use:           "haptix",
	SilenceUsage:  true,
	SilenceErrors: true,
	Short:         `H4ptiX client`,
}

func main() {
	rootCmd.AddCommand(
		triggerCmd,
		partyCmd,
	)

	if err := rootCmd.Execute(); err != nil {
		_, _ = fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
