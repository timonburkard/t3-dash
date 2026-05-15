idf_component_register(SRCS "esp32c3.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash bt esp_driver_uart esp_driver_gpio)
