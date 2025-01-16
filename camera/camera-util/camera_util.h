#ifndef CAMERA_UTIL_H
#define CAMERA_UTIL_H

#include "esp_err.h"
#include "esp_camera.h"

/**
 * @brief Initialize the camera.
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t camera_init(void);

/**
 * @brief Deinitialize the camera.
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t camera_deinit(void);

/**
 * @brief Capture an image and convert it to JPEG format.
 * 
 * @param jpg_buf Pointer to the output JPEG buffer.
 * @param jpg_len Pointer to the length of the output JPEG buffer.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t camera_capture_jpeg(uint8_t **jpg_buf, size_t *jpg_len);

#endif // CAMERA_UTIL_H