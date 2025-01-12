#include "spiffs.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"

const char* TAG = "spiffs";

static bool spiffs_mounted = false;
static const char* mount_point = NULL;

/**
 * @brief Check if SPIFFS is currently mounted.
 *
 * @return true if SPIFFS is mounted, false otherwise.
 */
bool is_spiffs_mounted()
{
    return spiffs_mounted; // Corrected variable name
}

/**
 * @brief Set the mount point for SPIFFS.
 *
 * @param base_path The base path where SPIFFS will be mounted.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t set_spiffs_mount_point(const char* base_path)
{
    if (base_path == NULL || strlen(base_path) == 0)
    {
        ESP_LOGE(TAG, "Invalid base path");
        return ESP_ERR_INVALID_ARG;
    }
    mount_point = base_path;
    ESP_LOGI(TAG, "Mount point set to %s", base_path);
    return ESP_OK;
}

/**
 * @brief Mount SPIFFS with the given configuration.
 *
 * @param conf Pointer to the SPIFFS configuration structure.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t mount_spiffs(esp_vfs_spiffs_conf_t* conf)
{
    if (spiffs_mounted)
    {
        ESP_LOGE(TAG, "Filesystem already mounted at %s", mount_point);
        return ESP_ERR_INVALID_STATE;
    }

    if (mount_point == NULL)
    {
        ESP_LOGE(TAG, "Mount point not set");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        esp_vfs_spiffs_unregister(NULL); // Unregister SPIFFS in case of error
        return ret;
    }

    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    spiffs_mounted = true;
     return ESP_OK;
}

/**
 * @brief Mount SPIFFS with default configuration values.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t mount_spiffs_default()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = mount_point,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };

    return mount_spiffs(&conf);
}

/**
 * @brief Unmount SPIFFS.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t unmount_spiffs()
{
    if (!spiffs_mounted)
    {
        ESP_LOGE(TAG, "Filesystem not mounted at %s", mount_point);
        return ESP_ERR_INVALID_STATE;
    }

    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(TAG, "Filesystem unmounted successfully from %s", mount_point);
    spiffs_mounted = false;
    return ESP_OK;
}
