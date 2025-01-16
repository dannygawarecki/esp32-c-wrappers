#include "http_server_util.h"
#include "file_operations.h"
#include "camera_util.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"
#include "esp_timer.h"

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>

static httpd_handle_t server = NULL;
static const char *TAG = "http_server_util";

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)
#define MAX_FILE_SIZE (200*1024) // 200 KB
#define MAX_FILE_SIZE_STR "200KB"
#define SCRATCH_BUFSIZE 8192

#define PART_BOUNDARY "123456789000000000000987654321"

struct file_server_data {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
};

#define HTTP_RESP_SEND_ERR(req, status, msg) \
    do { \
        httpd_resp_send_err(req, status, msg); \
        return ESP_FAIL; \
    } while (0)

static esp_err_t send_html_header(httpd_req_t *req) {
    httpd_resp_sendstr_chunk(req, "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">:<title>ESP32-CAM</title>"
    "<style>body {margin: 0; padding: 0; box-sizing: border-box;} table {width: 95%; margin: auto; table-layout: fixed; border-collapse: collapse;} th, td {border: 1px solid #000; padding: 10px; text-align: center; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;}</style>"
    "</head><body>");
    return ESP_OK;
}

static esp_err_t send_html_footer(httpd_req_t *req) {
    httpd_resp_sendstr_chunk(req, "</body></html>");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t send_file_list(httpd_req_t *req, const char *dirpath) {
    char entrypath[FILE_PATH_MAX];
    char entrysize[16];
    char date[30];
    const char *entrytype;

    struct dirent **entries;
    size_t entry_count;
    esp_err_t ret;

    ret = list_files(dirpath, true, false, &entries, &entry_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to list directory : %s", dirpath);
        HTTP_RESP_SEND_ERR(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to list directory");
    }

    httpd_resp_sendstr_chunk(req, "<h2>Files in ");
    httpd_resp_sendstr_chunk(req, dirpath);
    httpd_resp_sendstr_chunk(req, "</h2><table border=\"1\">"
        "<thead><tr><th>Name</th><th>Type</th><th>Size (Bytes)</th><th>Date</th><th>Delete</th></tr></thead>"
        "<tbody>");

    for (size_t i = 0; i < entry_count; i++) {
        struct dirent *entry = entries[i];
        entrytype = (entry->d_type == DT_DIR ? "directory" : "file");

        strlcpy(entrypath, dirpath, sizeof(entrypath));
        strlcat(entrypath, "/", sizeof(entrypath));
        strlcat(entrypath, entry->d_name, sizeof(entrypath));

        struct stat entry_stat;
        if (stat(entrypath, &entry_stat) == -1) {
            ESP_LOGE(TAG, "Failed to stat %s : %s", entrytype, entry->d_name);
            continue;
        }
        snprintf(entrysize, sizeof(entrysize), "%ld", entry_stat.st_size);
        ESP_LOGI(TAG, "Found %s : %s (%s bytes)", entrytype, entry->d_name, entrysize);
        struct tm *tm_info = localtime(&entry_stat.st_mtime); 
        strftime(date, sizeof(date), "%m/%d/%Y %I:%M:%S %p", tm_info);

        httpd_resp_sendstr_chunk(req, "<tr><td><a href=\"");
        httpd_resp_sendstr_chunk(req, req->uri);
        httpd_resp_sendstr_chunk(req, entry->d_name);
        if (entry->d_type == DT_DIR) {
            httpd_resp_sendstr_chunk(req, "/");
        }
        httpd_resp_sendstr_chunk(req, "\">");
        httpd_resp_sendstr_chunk(req, entry->d_name);
        httpd_resp_sendstr_chunk(req, "</a></td><td>");
        httpd_resp_sendstr_chunk(req, entrytype);
        httpd_resp_sendstr_chunk(req, "</td><td>");
        httpd_resp_sendstr_chunk(req, entrysize);
        httpd_resp_sendstr_chunk(req, "</td><td>");
        httpd_resp_sendstr_chunk(req, date);
        httpd_resp_sendstr_chunk(req, "</td><td>");
        httpd_resp_sendstr_chunk(req, "<form method=\"post\" action=\"/delete");
        httpd_resp_sendstr_chunk(req, req->uri);
        httpd_resp_sendstr_chunk(req, entry->d_name);
        httpd_resp_sendstr_chunk(req, "\"><button type=\"submit\">Delete</button></form>");
        httpd_resp_sendstr_chunk(req, "</td></tr>\n");

        free(entry);
    }
    free(entries);

    httpd_resp_sendstr_chunk(req, "</tbody></table>");
    return ESP_OK;
}

static esp_err_t index_html_get_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t file_list_html(httpd_req_t *req, const char *dirpath) {
    char clean_dirpath[FILE_PATH_MAX];
    strlcpy(clean_dirpath, dirpath, sizeof(clean_dirpath));
    size_t dirpath_len = strlen(clean_dirpath);
    if (clean_dirpath[dirpath_len - 1] == '/') {
        clean_dirpath[dirpath_len - 1] = '\0';
    }

    ESP_LOGI(TAG, "Request to list directory : %s", clean_dirpath);
    send_html_header(req);

    send_file_list(req, clean_dirpath);
    send_html_footer(req);
    return ESP_OK;
}

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename) {
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    }
    return httpd_resp_set_type(req, "text/plain");
}

static const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize) {
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize) {
        return NULL;
    }

    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);
    return dest + base_pathlen;
}

static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[]   asm("_binary_favicon_ico_end");
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_size);
    return ESP_OK;
}

static esp_err_t download_file_get_handler(httpd_req_t *req) {
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;

    const char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path, req->uri, sizeof(filepath));
    ESP_LOGI(TAG, "Filename: %s", filename);

    if (!filename) {
        ESP_LOGE(TAG, "Filename is too long");
        HTTP_RESP_SEND_ERR(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
    }

    if (filename[strlen(filename) - 1] == '/') {
        ESP_LOGI(TAG, "Responding with directory contents of %s", filepath);
        return file_list_html(req, filepath);
    }

    if (stat(filepath, &file_stat) == -1) {
        if (strcmp(filename, "/index.html") == 0) {
            return index_html_get_handler(req);
        } else if (strcmp(filename, "/favicon.ico") == 0) {
            return favicon_get_handler(req);
        }
        ESP_LOGE(TAG, "Failed to stat file : %s", filepath);
        HTTP_RESP_SEND_ERR(req, HTTPD_404_NOT_FOUND, "File does not exist");
    }

    fd = fopen(filepath, "r");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
        HTTP_RESP_SEND_ERR(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
    }

    ESP_LOGI(TAG, "Sending file : %s (%ld bytes)...", filename, file_stat.st_size);
    set_content_type_from_file(req, filename);

    char *chunk = ((struct file_server_data *)req->user_ctx)->scratch;
    size_t chunksize;
    do {
        chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);
        if (chunksize > 0) {
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                fclose(fd);
                ESP_LOGE(TAG, "File sending failed!");
                httpd_resp_sendstr_chunk(req, NULL);
                HTTP_RESP_SEND_ERR(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
            }
        }
    } while (chunksize != 0);

    fclose(fd);
    ESP_LOGI(TAG, "File sending complete");
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t delete_file_post_handler(httpd_req_t *req) {
    char filepath[FILE_PATH_MAX];
    struct stat file_stat;

    const char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path, req->uri + sizeof("/delete") - 1, sizeof(filepath));
    if (!filename) {
        HTTP_RESP_SEND_ERR(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
    }

    if (filename[strlen(filename) - 1] == '/') {
        ESP_LOGE(TAG, "Invalid filename : %s", filename);
        HTTP_RESP_SEND_ERR(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid filename");
    }

    if (stat(filepath, &file_stat) == -1) {
        ESP_LOGE(TAG, "File does not exist : %s", filename);
        HTTP_RESP_SEND_ERR(req, HTTPD_400_BAD_REQUEST, "File does not exist");
    }

    ESP_LOGI(TAG, "Deleting file : %s", filename);
    unlink(filepath);
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_sendstr(req, "File deleted successfully");
    return ESP_OK;
}

esp_err_t jpg_stream_handler(httpd_req_t *req){
    const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
    const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
    const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

    esp_err_t res = ESP_OK;
    size_t jpg_len = 0;
    uint8_t * jpg_buf = NULL;
    char part_buf[64]; // Corrected type from char* to char
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true)
    {
        // ...existing code...
        res = camera_capture_jpeg(&jpg_buf, &jpg_len); // Corrected pointer types
        if (res != ESP_OK || jpg_buf == NULL || jpg_len == 0) {
            ESP_LOGE(TAG, "Failed to capture JPEG image");
            if (jpg_buf) {
                free(jpg_buf); // Free the buffer if it was allocated
            }
            break;
        }

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, jpg_len);

            res = httpd_resp_send_chunk(req, part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)jpg_buf, jpg_len);
        }

        if (jpg_buf) {
            free(jpg_buf); // Free the buffer after sending
        }

        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        //ESP_LOGI(TAG, "MJPG: %uKB %ums (%.1ffps)",
        //    (uint32_t)(_jpg_buf_len/1024),
        //    (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);

        // Add delay to control frame rate (e.g., 30 FPS)
        vTaskDelay(pdMS_TO_TICKS(33)); // 33 ms delay for ~30 FPS
    }

    last_frame = 0;
    return res;
}

esp_err_t start_http_server(const char *base_path) {
    static struct file_server_data *server_data = NULL;

    if (server_data) {
        ESP_LOGE(TAG, "File server already started");
        return ESP_ERR_INVALID_STATE;
    }

    server_data = calloc(1, sizeof(struct file_server_data));
    if (!server_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return ESP_ERR_NO_MEM;
    }
    strlcpy(server_data->base_path, base_path, sizeof(server_data->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) 
    { 
        httpd_uri_t uri_handler = { 
            .uri = "/image-stream", 
            .method = HTTP_GET, 
            .handler = jpg_stream_handler, 
            .user_ctx = server_data 
        }; 
        httpd_register_uri_handler(server, &uri_handler); 

        httpd_uri_t file_download = {
            .uri = "/*",
            .method = HTTP_GET,
            .handler = download_file_get_handler,
            .user_ctx = server_data
        };
        httpd_register_uri_handler(server, &file_download);

        httpd_uri_t file_delete = {
            .uri = "/delete/*",
            .method = HTTP_POST,
            .handler = delete_file_post_handler,
            .user_ctx = server_data
        };
        httpd_register_uri_handler(server, &file_delete);
    } 
    else
    {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void stop_http_server(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
}
