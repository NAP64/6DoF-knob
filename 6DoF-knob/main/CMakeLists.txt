idf_component_register(SRCS "knob.c"
                    PRIV_REQUIRES esp_adc
                    PRIV_REQUIRES esp_driver_gpio
                    INCLUDE_DIRS ".")