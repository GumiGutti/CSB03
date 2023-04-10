#include "camera_pins.h"
#include "esp32_pins.h"
#include <Wire.h>

TaskHandle_t hCore0task;

bool debug = 1;

enum taskState { sRun,
                 sError,
                 sStart };

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
    vTaskDelay(1);

  }  // end of loop
}
