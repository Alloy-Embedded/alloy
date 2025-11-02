idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES wifi mqtt nvs_flash esp_event
)
