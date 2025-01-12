idf_component_register(SRCS "sntp_sync.c"
                    INCLUDE_DIRS "."
                    REQUIRES "config")

# Add Kconfig file
register_component()
