idf_component_register(SRCS "lcd.c" "GUI.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_timer)