#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "esp_vfs_fat.h"

/**
 * @brief Set the mount point for the SD card.
 *
 * @param base_path The base path where the SD card will be mounted.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t set_sdcard_mount_point(const char* base_path);

/**
 * @brief Mount the SD card with the given configurations.
 *
 * @param host_config The SDMMC host configuration (optional).
 * @param slot_config The SDMMC slot configuration (optional).
 * @param mount_config The VFS FAT mount configuration (optional).
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t mount_sdcard(const sdmmc_host_t* host_config, const sdmmc_slot_config_t* slot_config, const esp_vfs_fat_sdmmc_mount_config_t* mount_config);

/**
 * @brief Mount the SD card with default configurations.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t mount_sdcard_default();

/**
 * @brief Unmount the SD card from the previously set mount point.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t unmount_sdcard();

/**
 * @brief Check if the SD card is currently mounted.
 *
 * @return true if the SD card is mounted, false otherwise.
 */
bool is_sdcard_mounted();
