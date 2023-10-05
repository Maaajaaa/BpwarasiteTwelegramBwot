#include <Logger/Logger.h>

Logger::Logger(std::vector<std::string> plantNames){
    logFileNames.resize(plantNames.size());
    for(int i = 0; i<plantNames.size(); i++){
        //generate names
        logFileNames.at(i) = std::string("/") + std::to_string(i) + std::string("_") + std::string(plantNames[i])+ std::string(".csv");
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
            headerString += "\n";
            writeFile(SPIFFS, logFileNames.at(i).c_str(), headerString.c_str());
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
    timeString += "\n";
    appendFile(SPIFFS, logFileNames.at(sensor_id).c_str(),timeString.c_str());
    return 1;
}

std::vector<std::string> Logger::getLogFileNames()
{
    return logFileNames;
}
void Logger::listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
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