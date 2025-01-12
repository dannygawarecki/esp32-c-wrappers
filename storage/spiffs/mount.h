#include <stdio.h>
#include <string.h>
#include "esp_err.h"

esp_err_t mount_sdcard(const char* base_path);
esp_err_t unmount_sdcard();
esp_err_t mount_internal_storage(const char* base_path);