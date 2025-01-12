idf_component_register(SRCS "wifi.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_netif)

# Add the Kconfig file
set(COMPONENT_ADD_INCLUDEDIRS ".")
register_component()
