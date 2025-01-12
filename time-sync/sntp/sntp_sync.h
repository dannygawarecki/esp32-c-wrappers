#ifndef SNTP_SYNC_H
#define SNTP_SYNC_H

#include "esp_err.h"

/**
 * @brief Set the system time using SNTP and configure the timezone.
 *
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG on invalid argument, ESP_FAIL on failure.
 */
esp_err_t set_system_time();

#endif // SNTP_SYNC_H
