#include "LogDB.h"

LogDB::LogDB()
{
}

int LogDB::begin()
{
    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LittleFS Mount Failed");
        return -1;
    }

    File root = LittleFS.open("/");
    if(!root){
        Serial.println("failed to open root directory");
        return -2;
    }
    if (!root.isDirectory()) {
        Serial.println("root directory - not a directory error");
        return -3;
    }

    sqlite3_initialize();

    //open DB

    int rc = sqlite3_open(DATA_LOG_DB_DIR, &db);
    if (rc) {
        Serial.printf("Can't open database: %s\n", sqlite3_errmsg(db));
        return -4;
    } else {
        Serial.printf("Opened database successfully\n");
    }

    //check if the table exists

    rc = db_exec


    
}

int LogDB::db_exec(sqlite3 *db, const char *sql) {
    const char* data = "Callback function called";
    char *zErrMsg = 0;

    Serial.println(sql);
    long start = micros();
    int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
    if (rc != SQLITE_OK) {
        Serial.printf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        Serial.printf("Operation done successfully\n");
    }
    Serial.print(F("Time taken:"));
    Serial.println(micros()-start);
    return rc;
}

int LogDB::callback(void *data, int argc, char **argv, char **azColName) {
    const char* data = "Callback function called";
    int i;
    Serial.printf("%s: ", (const char*)data);
    for (i = 0; i<argc; i++){
        Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    Serial.printf("\n");
    return 0;
}