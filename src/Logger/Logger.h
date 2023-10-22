#include <FS.h>
#include <SPIFFS.h>
#include <time.h>

#include "BParasite.h"
#include "config.h"

/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_SPIFFS_IF_FAILED true

#define LOG_TEMPERATURE
#define LOG_HUMIDITY
#define LOG_SIGNAL

#define FILE_WRITTEN_SUCCESSFULLY 200
#define FILE_EPERM 1 //operation not permitted
#define FILE_EISDIR 21 //Is a directory
#define FILE_ENOENT 2 //no such file or directory
#define FILE_ENOSPC 28 //No space left on device

class Logger{
    public:
        Logger(std::vector<std::string>);
        bool begin();
        bool logData(int sensor_id, BParasite_Data_S data, time_t time);
        std::vector<std::string> getLogFileNames();
        static int logError(const char *logtext, va_list args);
        static int logError(const char *logtext);
    private:
        static int writeFile(const char * path, const char * message, const char * mode= FILE_WRITE);
        static int appendFile(const char * path, const char * message);

        bool successfulStart;
        std::vector<std::string> logFileNames;
};