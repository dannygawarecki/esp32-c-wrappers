#ifndef HTTP_SERVER_UTIL_H
#define HTTP_SERVER_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <esp_http_server.h>
#include <esp_err.h>

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

esp_err_t start_http_server(const char *base_path);
void stop_http_server(void);

#endif // HTTP_SERVER_UTIL_H
