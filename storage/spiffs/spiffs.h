#ifndef SPIFFS_H
#define SPIFFS_H

#include "esp_err.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

/**
 * @brief Check if SPIFFS is currently mounted.
 *
 * @return true if SPIFFS is mounted, false otherwise.
 */
bool is_spiffs_mounted();

/**
 * @brief Set the mount point for SPIFFS.
 *
 * @param base_path The base path where SPIFFS will be mounted.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t set_spiffs_mount_point(const char* base_path);

/**
 * @brief Mount SPIFFS with the given configuration.
 *
 * @param conf Pointer to the SPIFFS configuration structure.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t mount_spiffs(esp_vfs_spiffs_conf_t* conf);

/**
 * @brief Mount SPIFFS with default configuration values.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t mount_spiffs_default();

/**
 * @brief Unmount SPIFFS.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t unmount_spiffs();

#endif // SPIFFS_H