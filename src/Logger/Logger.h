#include "FS.h"
#include "SPIFFS.h"
#include "BParasite.h"

/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_SPIFFS_IF_FAILED true

#define mstLogFile0 "/moistLog0.csv"

class Logger{
    public:
        Logger();
        bool begin();
        bool logData(int data_id, std::string data);
    private:
        void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
        void readFile(fs::FS &fs, const char * path);
        void writeFile(fs::FS &fs, const char * path, const char * message);
        void appendFile(fs::FS &fs, const char * path, const char * message);

        bool successfulStart;
};