
#include <Logger/Logger.h>

Logger::Logger(){
}

bool Logger::begin(){
    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS formating and mount Failed");
        return false;
    }
    //create LOG FILES if not existent
    if(!SPIFFS.exists(mstLogFile0)){
        //format spiffs
        Serial.println("log file does not exist");
        //create first line
        writeFile(SPIFFS, mstLogFile0, "unix timestamp (this formula =R2/86400000+DATE(1970;1;1) , where R2 is the cell containing the timestamp);moisture (divide by 100 to get %)\n");
    }else{
        File file = SPIFFS.open(mstLogFile0, FILE_READ);
        Serial.print("Log File exists, size: ");
        Serial.println(file.size());
        file.close();
    }
    return true;
}

bool Logger::logData(int sensor_id, BParasite_Data_S data, time_t time){
    String timeString = String(time);
    timeString += ";";
    timeString += data.soil_moisture;
    #ifdef LOG_TEMPERATURE
    timeString += ";";
    timeString += data.temperature;
    #endif
    #ifdef LOG_HUMIDITY
    timeString += ";";
    timeString += data.humidity;
    #endif
    timeString += "\n";
    appendFile(SPIFFS, mstLogFile0 ,timeString.c_str());
    return 1;
}

void Logger::listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void Logger::readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void Logger::writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void Logger::appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);
    int tBytes = SPIFFS.totalBytes(); 
    int uBytes = SPIFFS.usedBytes();
    if(tBytes - uBytes <= sizeof(message)+2){
        Serial.println("ERROR\n\nERROR storage full\n\nERROR");
    }
    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}