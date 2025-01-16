#include "file_operations.h"

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "stdbool.h"
#include <limits.h>

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

esp_err_t list_files(const char *path, bool include_dirs, bool recursive, struct dirent ***results, size_t *count) {
    ESP_LOGI(TAG, "Listing files in directory: %s", path);
    DIR *dir = opendir(path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory: %s", path);
        return ESP_FAIL;
    }

    struct dirent *entry;
    size_t num_entries = 0;
    size_t max_entries = 10;
    struct dirent **entries = malloc(max_entries * sizeof(struct dirent *));
    if (!entries) {
        closedir(dir);
        return ESP_ERR_NO_MEM;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (!include_dirs && entry->d_type == DT_DIR) {
            continue;
        }

        if (strcmp(entry->d_name, "System Volume Information") == 0 ||
            strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0) 
        { 
            continue; 
        }

        if (num_entries >= max_entries) {
            max_entries *= 2;
            struct dirent **new_entries = realloc(entries, max_entries * sizeof(struct dirent *));
            if (!new_entries) {
                for (size_t i = 0; i < num_entries; i++) {
                    free(entries[i]);
                }
                free(entries);
                closedir(dir);
                return ESP_ERR_NO_MEM;
            }
            entries = new_entries;
        }

        entries[num_entries] = malloc(sizeof(struct dirent));
        if (!entries[num_entries]) {
            for (size_t i = 0; i < num_entries; i++) {
                free(entries[i]);
            }
            free(entries);
            closedir(dir);
            return ESP_ERR_NO_MEM;
        }
        memcpy(entries[num_entries], entry, sizeof(struct dirent));
        num_entries++;
    }

    closedir(dir);
    *results = entries;
    *count = num_entries;
    ESP_LOGI(TAG, "Listed %zu entries in directory: %s", num_entries, path);
    return ESP_OK;
}
