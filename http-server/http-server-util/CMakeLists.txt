cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "http_server_util.c"
                            "C:/Users/danny/source/repos/esp32-c-wrappers/storage/file_operations/file_operations.c"
                            "C:/Users/danny/source/repos/esp32-c-wrappers/camera/camera-util/camera_util.c"
                       INCLUDE_DIRS "." "C:/Users/danny/source/repos/esp32-c-wrappers/storage/file_operations"
                                    "C:/Users/danny/source/repos/esp32-c-wrappers/camera/camera-util"
                       REQUIRES esp_http_server nvs_flash
                       EMBED_FILES "favicon.ico")
