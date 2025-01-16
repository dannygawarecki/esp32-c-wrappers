#include "camera_util.h"

#include "esp_camera.h"
#include "esp_log.h"

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

static const char *TAG = "camera_util";

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

static bool camera_initialized = false;

/**
 * @brief Initialize the camera.
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t camera_init(void)
{
    if (camera_initialized)
    {
        return ESP_OK;
    }

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    camera_initialized = true;
    return ESP_OK;
}

/**
 * @brief Deinitialize the camera.
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t camera_deinit(void)
{
    if (!camera_initialized)
    {
        return ESP_OK;
    }

    esp_camera_deinit();
    camera_initialized = false;
    return ESP_OK;
}

/**
 * @brief Convert a frame buffer to JPEG format.
 * 
 * @param fb The frame buffer to convert.
 * @param jpg_buf Pointer to the output JPEG buffer.
 * @param jpg_len Pointer to the length of the output JPEG buffer.
 * @param quality JPEG quality (0-100).
 * @return true on success, false on failure.
 */
static bool convert_frame_to_jpeg(camera_fb_t *fb, uint8_t **jpg_buf, size_t *jpg_len, uint8_t quality) 
{ 
    if (!frame2jpg(fb, quality, jpg_buf, jpg_len)) 
    { 
        ESP_LOGE(TAG, "Failed to convert frame to JPEG"); 
        return false; 
    } 
    return true; 
}

/**
 * @brief Capture an image and convert it to JPEG format.
 * 
 * @param jpg_buf Pointer to the output JPEG buffer.
 * @param jpg_len Pointer to the length of the output JPEG buffer.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t camera_capture_jpeg(uint8_t **jpg_buf, size_t *jpg_len)
{
    esp_err_t err = camera_init();
    if (err != ESP_OK)
    {
        return err;
    }

    ESP_LOGI(TAG, "Taking picture...");
    //gpio_set_level(GPIO_FLASH_PIN, 1);
    //vTaskDelay(100);
    camera_fb_t *pic = esp_camera_fb_get();
    //gpio_set_level(GPIO_FLASH_PIN, 0);
    if (!pic)
    {
        ESP_LOGE(TAG, "Failed to capture image");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Picture taken...");

    if(pic->format != PIXFORMAT_JPEG)
    {
        if (!convert_frame_to_jpeg(pic, jpg_buf, jpg_len, 80))
        { 
            ESP_LOGE(TAG, "JPEG conversion failed"); 
            esp_camera_fb_return(pic); 
            return ESP_FAIL; 
        }
        ESP_LOGI(TAG, "Picture converted to JPEG...");
    }

    esp_camera_fb_return(pic);
    return ESP_OK;
}