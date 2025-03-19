if (CONFIG_GPIO_TRIGGER)

zephyr_library()
zephyr_library_sources(gpio_trigger.c)

endif (CONFIG_GPIO_TRIGGER)
