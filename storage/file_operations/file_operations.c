#include "file_operations.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

static const char* TAG = "file_operations";

esp_err_t read_file(const char *path, char *buffer, size_t size) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", path);
        return ESP_FAIL;
    }

    size_t bytesRead = fread(buffer, 1, size, file);
    fclose(file);

    if (bytesRead < size) {
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

esp_err_t write_file(const char *path, const uint8_t *data, size_t bytes_to_write) {
    ESP_LOGI(TAG, "Writing to file: %s", path);
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", path);
        return ESP_FAIL;
    }

    size_t bytesWritten = fwrite(data, 1, bytes_to_write, file);
    fclose(file);

    if (bytesWritten < bytes_to_write) {
        return ESP_ERR_INVALID_SIZE;
    }
    ESP_LOGI(TAG, "Wrote %zu bytes to file: %s", bytesWritten, path);

    return ESP_OK;
}
