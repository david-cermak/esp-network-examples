idf_component_register(SRCS "station_example_main.cpp"
                    PRIV_REQUIRES esp_wifi nvs_flash
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE -freflection)