#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <stdio.h>
#include "esp_err.h"

// Function to read a file
esp_err_t read_file(const char *path, char *buffer, size_t size);

// Function to write to a file
esp_err_t write_file(const char *path, const uint8_t *data, size_t bytes_to_write);

#endif // FILE_OPERATIONS_H
