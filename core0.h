#include "camera_pins.h"
#include "esp32_pins.h"
#include <esp_camera.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>
#include <driver/rtc_io.h>
#include <Adafruit_MLX90640.h>
#include <Wire.h>

TaskHandle_t hCore0task;

bool debug = 1;

//Egyelőre csak hogy legyen condition az if-be, mert nem tudom milyen triggerel indul
bool TakeRgb = 1;
bool TakeThermal = 1;

Adafruit_MLX90640 mlx;
float frame[32 * 24];  // buffer for full frame of temperatures

enum taskState { sRun,
                 sError,
                 sStart };
taskState sMlx = sStart;
taskState sRgb = sStart;

bool firstRunCore0 = true;

void core0task(void* parameter);

void core0setup() {  // a.k.a. setup
  xTaskCreatePinnedToCore(
    core0task,
    "core0task",
    10000,
    NULL,
    CORE0TASKPRIO,
    &hCore0task,
    0);
}

void core0task(void* parameter) {  // a.k.a. loop
  for (;;) {                       // the loop
    if (firstRunCore0) {
      Wire.begin(SDA, CLK, 400000);
      byte error, address;
      int nDevices;
      s.println("Scanning I2C");
      s.println("MLX     0x33");
      s.println("");
      nDevices = 0;
      for (address = 1; address < 127; address++) {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
          s.print("I2C device found at address 0x");
          if (address < 16)
            s.print("0");
          s.print(address, HEX);
          s.println("  !");

          nDevices++;
        } else if (error == 4) {
          s.print("Unknown error at address 0x");
          if (address < 16)
            s.print("0");
          s.println(address, HEX);
        }
      }
      if (nDevices == 0)
        s.println("No I2C devices found\n");
      else
        s.println("done\n");
      firstRunCore0 = false;
    }

    if (TakeRgb) {
      switch (sRgb) {
        case sRun:
          {
            //Csinál egy fotót
            camera_fb_t* fb = NULL;

            fb = esp_camera_fb_get();
            if (!fb) {
              if (debug) s.println("Camera Failed to Capture");
              sRgb = sError;
            }

            //itt kell menteni SD kártyára

            esp_camera_fb_return(fb);
            break;
          }
        case sError:
          {
            break;
          }
        case sStart:
          {
            WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //disable brownout detector
            camera_config_t config;
            config.ledc_channel = LEDC_CHANNEL_0;
            config.ledc_timer = LEDC_TIMER_0;
            config.pin_d0 = Y2_GPIO_NUM;
            config.pin_d1 = Y3_GPIO_NUM;
            config.pin_d2 = Y4_GPIO_NUM;
            config.pin_d3 = Y5_GPIO_NUM;
            config.pin_d4 = Y6_GPIO_NUM;
            config.pin_d5 = Y7_GPIO_NUM;
            config.pin_d6 = Y8_GPIO_NUM;
            config.pin_d7 = Y9_GPIO_NUM;
            config.pin_xclk = XCLK_GPIO_NUM;
            config.pin_pclk = PCLK_GPIO_NUM;
            config.pin_vsync = VSYNC_GPIO_NUM;
            config.pin_href = HREF_GPIO_NUM;
            config.pin_sscb_sda = SIOD_GPIO_NUM;
            config.pin_sscb_scl = SIOC_GPIO_NUM;
            config.pin_pwdn = PWDN_GPIO_NUM;
            config.pin_reset = RESET_GPIO_NUM;
            config.xclk_freq_hz = 20000000;
            config.pixel_format = PIXFORMAT_JPEG;

            if (psramFound()) {
              config.frame_size = FRAMESIZE_VGA;  // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
              config.jpeg_quality = 25;
              config.fb_count = 1;
            } else {
              config.frame_size = FRAMESIZE_VGA;
              config.jpeg_quality = 25;
              config.fb_count = 1;
            }

            esp_err_t err = esp_camera_init(&config);
            if (err != ESP_OK) {
              Serial.printf("Camera init failed with error 0x%x", err);
              sRgb = sError;
            }

            sensor_t* s = esp_camera_sensor_get();
            s->set_gain_ctrl(s, 0);      // auto gain off (1 or 0)
            s->set_exposure_ctrl(s, 0);  // auto exposure off (1 or 0)
            s->set_agc_gain(s, 0);       // set gain manually (0 - 30)
            s->set_aec_value(s, 600);    // set exposure manually (0-1200)

            sRgb = sRun;
            break;
          }
        default:
          {  //itt baj van....}
          }
      }
    }

    if (TakeThermal) {
      switch (sRgb) {
        case sRun:
          {
            if (mlx.getFrame(frame) != 0) {
              if (debug) s.println("Failed");
              sMlx = sError;
            }
            for (uint8_t h = 0; h < 24; h++) {
              for (uint8_t w = 0; w < 32; w++) {
                float t = frame[h * 32 + w];
              }
            }
            break;
          }
        case sError:
          {
            break;
          }
        case sStart:
          {
            if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
              if (debug) s.println("MLX90640 not found!");
              sMlx = sError;
            }
            mlx.setMode(MLX90640_CHESS);
            mlx.setResolution(MLX90640_ADC_18BIT);
            sMlx = sRun;
            break;
          }
        default:
          {  //itt baj van....}
          }
      }
    }
    vTaskDelay(1);
  }  // end of loop
}
