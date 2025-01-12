#include "sntp_sync.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <regex.h>

#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_err.h"
#include "sdkconfig.h"

static const char *TAG = "sntp_sync";

#define MAX_TIMEZONE_LENGTH 32

/**
 * @brief Callback function for time synchronization events.
 *
 * @param tv Pointer to the timeval structure.
 */
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

/**
 * @brief Initialize the SNTP service.
 */
void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, CONFIG_SNTP_NTP_SERVER);
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
}

/**
 * @brief Apply daylight savings time based on the current date.
 */
static void apply_daylight_savings_time(void)
{
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // US DST: starts on the second Sunday in March and ends on the first Sunday in November
    bool is_dst = false;
    if ((timeinfo.tm_mon > 2 && timeinfo.tm_mon < 10) || 
        (timeinfo.tm_mon == 2 && (timeinfo.tm_wday + 7 - timeinfo.tm_mday % 7) % 7 < 7) || 
        (timeinfo.tm_mon == 10 && (timeinfo.tm_wday + 7 - timeinfo.tm_mday % 7) % 7 >= 7)) {
        is_dst = true;
    }

    char tz_with_dst[64];
    if (is_dst) {
        snprintf(tz_with_dst, sizeof(tz_with_dst), "%sDST,M3.2.0,M11.1.0", CONFIG_SNTP_TIMEZONE);
    } else {
        snprintf(tz_with_dst, sizeof(tz_with_dst), "%s", CONFIG_SNTP_TIMEZONE);
    }
    setenv("TZ", tz_with_dst, 1);
    tzset();
    ESP_LOGI(TAG, "Daylight savings time %s", is_dst ? "enabled" : "disabled");
}

/**
 * @brief Set the timezone.
 *
 * @param timezone The timezone string (e.g., "EST5EDT").
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG on invalid argument.
 */
static esp_err_t set_timezone(const char *timezone)
{
    if (timezone == NULL || strlen(timezone) == 0 || strlen(timezone) >= MAX_TIMEZONE_LENGTH) {
        ESP_LOGE(TAG, "Invalid timezone: %s", timezone);
        return ESP_ERR_INVALID_ARG;
    }

    // Validate timezone format using a regular expression
    regex_t regex;
    int reti = regcomp(&regex, "^[A-Z]{3}[0-9]{1,2}(DST)?$", REG_EXTENDED);
    if (reti) {
        ESP_LOGE(TAG, "Could not compile regex");
        return ESP_ERR_INVALID_ARG;
    }

    reti = regexec(&regex, timezone, 0, NULL, 0);
    regfree(&regex);
    if (reti) {
        ESP_LOGE(TAG, "Invalid timezone format: %s", timezone);
        return ESP_ERR_INVALID_ARG;
    }

    char current_timezone[MAX_TIMEZONE_LENGTH];
    strncpy(current_timezone, timezone, sizeof(current_timezone) - 1);
    current_timezone[sizeof(current_timezone) - 1] = '\0'; // Ensure null-termination
    setenv("TZ", current_timezone, 1);
    tzset();
    ESP_LOGI(TAG, "Timezone set to: %s", current_timezone);

    // Check if the timezone string already includes DST
    if (strstr(current_timezone, "DST") == NULL) {
        apply_daylight_savings_time();
    }

    return ESP_OK;
}

/**
 * @brief Set the system time using SNTP and configure the timezone.
 *
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG on invalid argument, ESP_FAIL on failure.
 */
esp_err_t set_system_time()
{
    initialize_sntp();

    if (set_timezone(CONFIG_SNTP_TIMEZONE) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set timezone");
        return ESP_ERR_INVALID_ARG;
    }

    // Wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < CONFIG_SNTP_RETRY_COUNT) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, CONFIG_SNTP_RETRY_COUNT);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (retry == CONFIG_SNTP_RETRY_COUNT) {
        ESP_LOGE(TAG, "Failed to synchronize time");
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Time synchronized: %s", asctime(&timeinfo));
        return ESP_OK;
    }
}
