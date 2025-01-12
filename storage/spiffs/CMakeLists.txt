cmake_minimum_required(VERSION 3.5)

# Define the project name
project(spiffs_component)

# Add the source files
set(SOURCES
    spiffs.c
)

# Add the header files
set(HEADERS
    spiffs.h
)

# Create a static library
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

# Link against the required ESP-IDF components
target_link_libraries(${PROJECT_NAME} PRIVATE
    idf::esp_common
    idf::esp_spiffs
    idf::esp_vfs
    idf::log
)

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
