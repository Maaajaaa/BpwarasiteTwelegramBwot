#include "FS.h"
#include "SPIFFS.h"
#include "BParasite.h"
#include <time.h>

/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_SPIFFS_IF_FAILED true

#define mstLogFile "/moistLog0.csv"
#define LOG_TEMPERATURE
#define LOG_HUMIDITY
class Logger{
    public:
        Logger();
        bool begin();
        bool logData(int sensor_id, BParasite_Data_S data, time_t time);
    private:
        void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
        void readFile(fs::FS &fs, const char * path);
        void writeFile(fs::FS &fs, const char * path, const char * message);
        void appendFile(fs::FS &fs, const char * path, const char * message);

        bool successfulStart;
};