#include "Logger.h"

const char* lTag = "b-Logger";

Logger::Logger(std::vector<std::string> localPlantNames){
    logFileNames.resize(localPlantNames.size());
    for(int i = 0; i<localPlantNames.size(); i++){
        //generate names
        logFileNames.at(i) = std::string("/") + std::to_string(i) + std::string("_") + std::string(localPlantNames[i])+ std::string(".csv");
    }
}

bool Logger::begin(){
    //set log levels
    esp_log_level_set("*", ESP_LOG_WARN);      // enable WARN logs from WiFi stack
    esp_log_level_set("wifi", ESP_LOG_DEBUG);      // enable WARN logs from WiFi stack
    esp_log_level_set("dhcpc", ESP_LOG_DEBUG);     // enable WARN logs from DHCP client
    esp_log_level_set("majaStuff", ESP_LOG_DEBUG);     // enable debug logs from majaStuff
    esp_log_level_set("b-Messenger", ESP_LOG_INFO);     // enable debug logs from b-Messenger
    esp_log_level_set("b-Logger", ESP_LOG_DEBUG);     // enable debug logs from b-Messenger

    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS formating and mount Failed");
        return false;
    }
    for(int i = 0; i<logFileNames.size(); i++){
        if(!SPIFFS.exists(logFileNames.at(i).c_str())){
            
            writeFile(logFileNames.at(i).c_str(), headerString().c_str());
            printFileSizeAndName(logFileNames.at(i));
        }else{
            printFileSizeAndName(logFileNames.at(i));
        }
    }
    if(!SPIFFS.exists(ERROR_LOG_FILE)){
        //create empty log file
        writeFile(ERROR_LOG_FILE, "ESP32 log file\n");
        printFileSizeAndName(ERROR_LOG_FILE);
    }
    return true;
}

void Logger::printFileSizeAndName(std::string filenamepath){
    File file = SPIFFS.open(filenamepath.c_str(), FILE_READ);
    Serial.print(filenamepath.c_str());
    Serial.print(" exists, size: ");
    Serial.println(file.size());
    file.close();
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
    #ifdef LOG_SIGNAL
    timeString += ";";
    timeString += data.rssi;
    #endif    
    timeString += "\n";
    appendFile(logFileNames.at(sensor_id).c_str(),timeString.c_str());
    return 1;
}

std::vector<std::string> Logger::getLogFileNames()
{
    return logFileNames;
}

int Logger::logError(const char *logtext, va_list args)
{
    Serial.println("\n\nlogging to file");
    char* string;
    int retVal = vasprintf(&string, logtext, args);
    int writeResult = logErrorToFile(string);
    Serial.print(string);
    Serial.print(" vasprintf returned: ");
    Serial.println(retVal);
    Serial.print("log write returned: ");
    Serial.print(writeResult);
    //still write to stdout
    return vprintf(logtext, args);
}

int Logger::logErrorToFile(char *logtext)
{
    std::string logLine =  std::string("[") 
        + std::to_string(time(nullptr)) + std::string("] ") 
        + std::string(logtext) + std::string("\n");
    
    Serial.print("Log text: ");
    Serial.println(logLine.c_str());
    return appendFile(ERROR_LOG_FILE, logLine.c_str());
}
int Logger::writeFile(const char *filepath, const char *data, const char *mode)
{
    if( SPIFFS.totalBytes() - SPIFFS.usedBytes() <= sizeof(data)+2) return ENOSPC;
    //if(!SPIFFS.exists(filepath)) return ENOENT;
    File file = SPIFFS.open(filepath, mode);
    //check if we need to start a new file
    if(filepath == ERROR_LOG_FILE && file.size() > ERROR_LOG_FILE_LIMIT ||
    filepath != ERROR_LOG_FILE && file.size() > LOG_FILE_LIMIT_BYTES){
        //close and move the file
        file.close();
        String newFilePath = String(PATH_2) + String(filepath);
        if(SPIFFS.exists(newFilePath)){
            SPIFFS.remove(newFilePath.c_str());
        }
        bool success = SPIFFS.rename(filepath, newFilePath.c_str());
        //create header if it's a data log file
        if(filepath != ERROR_LOG_FILE){
            writeFile(filepath, headerString().c_str());
        }
        file = SPIFFS.open(filepath, mode);

        //do this after opening the file so if there's no possibility for an infite loop if we (failed to) move the log file
        if(success){
            ESP_LOGD(lTag, "moved file %s to %s", filepath, newFilePath.c_str());
        }else{
            ESP_LOGE(lTag, "FAILED TO move file %s to %s", filepath, newFilePath.c_str());
        }
    }
    if(file.isDirectory())  return EISDIR;
    if(!file) return ENOENT;
    if(file.println(data)){
        Serial.print(filepath);
        Serial.println(" written/appended");
    } else {
        return EIO;
    }
    file.close();
    return 0;
}

int Logger::appendFile(const char * filepath, const char * data){
    return writeFile(filepath, data, FILE_APPEND);
}

String Logger::headerString(){
    //create first line
    String headerString = "unix timestamp in seconds (this formula =R2/86400+DATE(1970;1;1) , where R2 is the cell containing the timestamp);moisture (divide by 100 to get %)";
    #ifdef LOG_TEMPERATURE
    headerString += ";Temperature (divide by 100 for degrees celcius)";
    #endif
    #ifdef LOG_HUMIDITY
    headerString += ";Humdity (divide by 100 for %)";
    #endif
    #ifdef LOG_SIGNAL
    headerString += ";signal strenth in dBm";
    #endif
    headerString += "\n";
    return headerString;
}