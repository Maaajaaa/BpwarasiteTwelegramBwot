#include "Logger.h"

Logger::Logger(std::vector<std::string> localPlantNames){
    logFileNames.resize(localPlantNames.size());
    for(int i = 0; i<localPlantNames.size(); i++){
        //generate names
        logFileNames.at(i) = std::string("/") + std::to_string(i) + std::string("_") + std::string(localPlantNames[i])+ std::string(".csv");
    }
}

bool Logger::begin(){
    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS formating and mount Failed");
        return false;
    }
    for(int i = 0; i<logFileNames.size(); i++){
        if(!SPIFFS.exists(logFileNames.at(i).c_str())){
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
            writeFile(logFileNames.at(i).c_str(), headerString.c_str());
            File file = SPIFFS.open(logFileNames.at(i).c_str(), FILE_READ);
            Serial.print("Log File ");
            Serial.print(file.name());
            Serial.print(" created, size: ");
            Serial.println(file.size());
            file.close();
        }else{
            File file = SPIFFS.open(logFileNames.at(i).c_str(), FILE_READ);
            Serial.print("Log File exists, size: ");
            Serial.println(file.size());
            file.close();
        }
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
    Serial.println("logging to file");
    logError(logtext);
    //still write to stdout
    return vprintf(logtext, args);
}

int Logger::logError(const char *logtext)
{
    return appendFile(ERROR_LOG_FILE, logtext);
}
int Logger::writeFile(const char *filepath, const char *data, const char *mode)
{
    if( SPIFFS.totalBytes() - SPIFFS.usedBytes() <= sizeof(data)+2) return ENOSPC;
    //if(!SPIFFS.exists(filepath)) return ENOENT;
    File file = SPIFFS.open(filepath, mode);
    if(file.isDirectory())  return EISDIR;
    if(!file) return ENOENT;

    if(file.print(data)){
        Serial.println("- file written/appended");
    } else {
        return EIO;
    }
    file.close();
    return 0;
}

int Logger::appendFile(const char * filepath, const char * data){
    return writeFile(filepath, data, FILE_APPEND);
}