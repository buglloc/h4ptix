menuconfig APP_TRIGGER
  bool "Trigger config"
  default y

if APP_TRIGGER

config APP_TRIGGER_CHAN_ID
  int "Trigger channel id"
  default 31337

config APP_TRIGGER_CHAN_PUB_TIMEOUT
  int "Trigger channel pub timeout (ms)"
  default 250

config APP_TRIGGER_DEFAULT_DURATION
  int "Trigger default duration (ms)"
  default 500

config APP_TRIGGER_TASK_PRIORITY
  int "Trigger work task priority"
  default 5

endif # APP_TRIGGER
