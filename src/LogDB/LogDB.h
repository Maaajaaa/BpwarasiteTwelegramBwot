#include <FS.h>
#include <LittleFS.h>
#include <time.h>
#include <sqlite3.h>

#include "BParasite.h"
#include "config.h"
#include <fstream>
#include "MaaajaaaClient/MaaajaaaClient.h"
/* You only need to format LittleFS the first time you run a
   test or else use the LITTLEFS plugin to create a partition
   https://github.com/lorol/arduino-esp32littlefs-plugin
   */

#define FORMAT_LITTLEFS_IF_FAILED true

#define LOG_TEMPERATURE
#define LOG_HUMIDITY
#define LOG_SIGNAL
#define LOG_SYNC

#define FILE_WRITTEN_SUCCESSFULLY 200
#define FILE_EPERM 1 //operation not permitted
#define FILE_EISDIR 21 //Is a directory
#define FILE_ENOENT 2 //no such file or directory
#define FILE_ENOSPC 28 //No space left on device

#define DATA_LOG_DB_DIR "/littlefs/parasiteDataLog.db"

class LogDB{
    public:
        LogDB();
        int begin();
    private:
        int db_exec(sqlite3*, const char *);
        static int callback(void *data, int argc, char **argv, char **azColName);
        
        sqlite3 *db;
};