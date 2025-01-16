#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <stdio.h>
#include <dirent.h>
#include "stdbool.h"
#include "esp_err.h"

esp_err_t read_file(const char *path, char *buffer, size_t size);
esp_err_t write_file(const char *path, const uint8_t *data, size_t bytes_to_write);
esp_err_t list_files(const char *path, bool include_dirs, bool recursive, struct dirent ***results, size_t *count);

#endif // FILE_OPERATIONS_H
