// HardwareSerial s(0);  // use UART0
HardwareSerial A(2);  // use UART0

TaskHandle_t hCore1task;

#include "FS.h"
#include "SD_MMC.h"

File file;
byte purpose = 255;
const uint32_t ringSize = 1024 * 32;
byte ring[ringSize];
uint32_t head = 0;
uint32_t tail = 0;
char buf1[10];
uint32_t buf1Idx = 0;
uint16_t tsHR = 0;
uint16_t tsMN = 0;
uint16_t tsSE = 0;
uint16_t ts100 = 0;


uint32_t ringBytes(void) {
  return head > tail ? head - tail : head + ringSize - tail;
}
void headPP() {
  head++;
  if (head == ringSize) head = 0;
}
void tailPP() {
  tail++;
  if (tail == ringSize) tail = 0;
}

String path;

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

bool sysTimeValid = false;  // A-ból a pontos idő század másodperc felbontásban érkezik, amikor a GPS képes küldeni
uint32_t sysTimeZero;       // a kontroller millis() számlálója, amikor megkaptuk a pontos napi időt
uint32_t sysTimeOffset;     // a napból eltelt milliszekundumok száma, amikor megkaptuk a pontos időt

void sysTime(String& st) {  // 0:00.00 óta eltelt ms-ot ad vissza 26-os számrendszerben: char a = 0, char z = 25
  char buf[9];
  uint32_t todayMillis;
  if (sysTimeValid) {
    todayMillis = millis() - sysTimeZero + sysTimeOffset;
  } else {
    todayMillis = millis();
  }
  for (int i = 0; i < 8; i++) {
    buf[7 - i] = 97 + todayMillis % 26;
    todayMillis /= 26;
  }
  buf[8] = 0;
  st = buf;
}

String most = "";

uint32_t tSDtrigger = 0;
uint32_t tSDdelay = 200;
uint32_t tParserTrigger = 0;
uint32_t tParserDelay = 100;

taskState sSD = sStart;


void core1task(void* parameter) {
  for (;;) {              // a.k.a. loop
    if (firstRunCore1) {  // a.k.a. setup
      s.begin(115200);
      s.print("Rx Buffer size ");
      s.println(A.setRxBufferSize(2048 * 8));
      A.begin(230400, SERIAL_8N1, RXD2, -1);
      firstRunCore1 = false;
    }


    if (millis() - tParserTrigger > tParserDelay) {
      tParserTrigger = millis();
      while (A.available()) {
        char inChar = A.read();
        switch (inChar) {
          case 0:
            {
              purpose = 0;
              break;
            }
          case 1:
            {
              purpose = 1;
              buf1Idx = 0;
              break;
            }
          case 10:
            {
              purpose = 255;
              break;
            }
          case 11:  // time stamp sent
            {
              purpose = 255;
              buf1[buf1Idx] = 0;
              s.printf("TS buf Idx: %d ", buf1Idx);
              for (int i = 0; i < buf1Idx; i++)
                s.printf(" %#02x", buf1[i]);
              s.println();
              String ts = buf1;
              tsHR = ts.substring(0, 2).toInt();
              tsMN = ts.substring(2, 4).toInt();
              tsSE = ts.substring(4, 6).toInt();
              ts100 = ts.substring(7, 9).toInt();
              s.printf("Timestamp %02d:%02d:%02d.%02d\n", tsHR, tsMN, tsSE, ts100);
              sysTimeOffset = ts100 * 10 + tsSE * 1000 + tsMN * 60000 + tsHR * 3600000;
              sysTimeZero = millis();
              sysTimeValid = true;
              break;
            }
          default:
            {
              switch (purpose) {
                case 0:  // fill data to ringbuffer for SD card write
                  {
                    ring[head] = inChar;
                    headPP();
                    if (tail == head) {
                      s.println("ring buffer overrun, CanSat failed...; jajj, de baj!!!!");
                    }
                    break;
                  }
                case 1:  // collect timestamp data
                  {
                    if (buf1Idx < 9)
                      buf1[buf1Idx++] = inChar;
                    break;
                  }
                default:
                  {
                    s.printf("stray char %#02x, ajjajj!\n", inChar);
                  }
              }
            }
        }
      }
    }

    if (millis() - tSDtrigger > tSDdelay) {
      tSDtrigger = millis();
      switch (sSD) {
        case sRun:
          {
            sysTime(most);
            s.printf("%s buffer: %05.2f\n", most, (double)ringBytes() / ringSize * 100.0);

            // write out to file if needed
            break;
          }
        case sError:
          {
            break;
          }
        case sStart:
          {
            if (SD_MMC.begin("/sdcard", true)) {
              s.println("SD opened!");
              sSD = sRun;
            } else {
              s.println("SD card not open!");
              sSD = sError;
            }
            break;
          }
        default:
          {  //itt baj van....}
          }
      }
    }

    vTaskDelay(1);
  }  // end of loop
  // *** heap_caps_get_free_size(MALLOC_CAP_8BIT);
}
