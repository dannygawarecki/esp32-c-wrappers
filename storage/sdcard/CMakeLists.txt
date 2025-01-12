cmake_minimum_required(VERSION 3.5)

# Define the project name
project(sdcard_component)

# Add the source files
set(SRCS
    "sdcard.c"
)

# Add the include directories
set(INCLUDES
    "."
)

# Create a static library for the SD card component
idf_component_register(SRCS ${SRCS}
                       INCLUDE_DIRS ${INCLUDES}
                       REQUIRES driver esp_vfs fatfs)

# Specify the C standard
set(CMAKE_C_STANDARD 99)
