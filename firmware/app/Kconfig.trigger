menuconfig APP_TRIGGER
  bool "Trigger config"
  default y

if APP_TRIGGER

config APP_TRIGGER_DEFAULT_DURATION
  int "Trigger default duration (ms)"
  default 500

config APP_TRIGGER_TASK_PRIORITY
  int "Trigger work task priority"
  default 5

endif # UI
