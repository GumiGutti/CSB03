// #include <EEPROM.h>

TaskHandle_t hCore1task;

bool firstRunCore1 = true;

void core1task(void* parameter);
void core1setup(void) {  // a.k.a. setup
  xTaskCreatePinnedToCore(
    core1task,
    "core1task",
    10000,
    NULL,
    CORE1TASKPRIO,
    &hCore1task,
    1);
}


void core1task(void* parameter) {
  for (;;) {  // a.k.a. loop
    if (firstRunCore1) {
    }  // end of loratrigger
    vTaskDelay(1);
  }  // end of loop
  // *** heap_caps_get_free_size(MALLOC_CAP_8BIT);
}
