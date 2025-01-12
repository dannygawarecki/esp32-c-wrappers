#include "sdcard.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "sdkconfig.h"
#include "sdmmc_cmd.h"

#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"

static const char* TAG = "sdcard";

static bool sdcard_mounted = false;
static const char* mount_point = NULL;
static sdmmc_card_t *card;

/**
 * @brief Check if the SD card is currently mounted.
 *
 * @return true if the SD card is mounted, false otherwise.
 */
bool is_sdcard_mounted()
{
    return sdcard_mounted;
}

/**
 * @brief Set the mount point for the SD card.
 *
 * @param base_path The base path where the SD card will be mounted.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t set_sdcard_mount_point(const char* base_path)
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
 * @brief Mount the SD card with the given configurations.
 *
 * @param host_config The SDMMC host configuration (optional).
 * @param slot_config The SDMMC slot configuration (optional).
 * @param mount_config The VFS FAT mount configuration (optional).
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t mount_sdcard(const sdmmc_host_t* host_config, const sdmmc_slot_config_t* slot_config, const esp_vfs_fat_sdmmc_mount_config_t* mount_config)
{
    if (sdcard_mounted)
    {
        ESP_LOGE(TAG, "Filesystem already mounted at %s", mount_point);
        return ESP_ERR_INVALID_STATE;
    }

    if (mount_point == NULL)
    {
        ESP_LOGE(TAG, "Mount point not set");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Initializing SD card");

    esp_err_t err = esp_vfs_fat_sdmmc_mount(mount_point, host_config, slot_config, mount_config, &card);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to mount filesystem: %s", esp_err_to_name(err));
        return err;
    }
    sdmmc_card_print_info(stdout, card);
    ESP_LOGI(TAG, "Filesystem mounted successfully");
    sdcard_mounted = true;
    return ESP_OK;
}

/**
 * @brief Mount the SD card with default configurations.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t mount_sdcard_default()
{
    sdmmc_host_t default_host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t default_slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t default_mount_config = {
        .format_if_mount_failed = false,
        .max_files = 3,
    };

    return mount_sdcard(&default_host, &default_slot_config, &default_mount_config);
}

/**
 * @brief Unmount the SD card from the previously set mount point.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t unmount_sdcard()
{
    if (!sdcard_mounted)
    {
        ESP_LOGE(TAG, "Filesystem not mounted at %s", mount_point);
        return ESP_ERR_INVALID_STATE;
    }
    
    if (mount_point == NULL)
    {
        ESP_LOGE(TAG, "Mount point not set");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = esp_vfs_fat_sdcard_unmount(mount_point, card);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to unmount filesystem: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Filesystem unmounted successfully from %s", mount_point);
    sdcard_mounted = false;
    return ESP_OK;
}