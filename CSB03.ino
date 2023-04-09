#include "Arduino.h"
#include <HardwareSerial.h>

HardwareSerial s(0);     // use UART0 serial debug or unit A-B intercom

#define CORE0TASKPRIO 3
#define CORE1TASKPRIO 3
#include "core0.h"
#include "core1.h"



void setup() {

  s.begin(115200);
  s.println();
  s.println("CANSAT boot....");

  core0setup();
  core1setup();

}

void loop() {
  vTaskDelay(1);
}