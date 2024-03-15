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
    esp_log_level_set("b-Logger", ESP_LOG_INFO);     // enable debug logs from b-Messenger
    esp_log_level_set("maaajaaaClient", ESP_LOG_INFO);     // enable debug logs from MaaajaaaClient

    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LittleFS Mount Failed");
        return false;
    }
    for(int i = 0; i<logFileNames.size(); i++){
        if(!LittleFS.exists(logFileNames.at(i).c_str())){
            
            writeFile(logFileNames.at(i).c_str(), headerString().c_str());
            printFileSizeAndName(logFileNames.at(i));
        }else{
            printFileSizeAndName(logFileNames.at(i));
        }
    }
    if(!LittleFS.exists("/tmp.html")){
        writeFile("/tmp.html", "hiii");
    }
    if(!LittleFS.exists(ERROR_LOG_FILE)){
        //create empty log file
        writeFile(ERROR_LOG_FILE, "ESP32 log file\n");
        printFileSizeAndName(ERROR_LOG_FILE);
    }
    return true;
}

void Logger::printFileSizeAndName(std::string filenamepath){
    File file = LittleFS.open(filenamepath.c_str(), FILE_READ);
    Serial.print(filenamepath.c_str());
    Serial.print(" exists, size: ");
    Serial.println(file.size());
    file.close();
}

bool Logger::logData(int sensor_id, BParasite_Data_S data, time_t time, bool logged){
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
    #ifdef LOG_SYNC
    timeString += ";";
    timeString += String(logged);
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
    if( LittleFS.totalBytes() - LittleFS.usedBytes() <= sizeof(data)+2) return ENOSPC;
    //if(!SPIFFS.exists(filepath)) return ENOENT;
    File file = LittleFS.open(filepath, mode);
    //check if we need to start a new file
    if(filepath == ERROR_LOG_FILE && file.available() &&  file.size() > ERROR_LOG_FILE_LIMIT ||
    filepath != ERROR_LOG_FILE && file.size() > LOG_FILE_LIMIT_BYTES){
        //close and move the file
        file.close();
        String newFilePath = String(PATH_2) + String(filepath);
        if(LittleFS.exists(newFilePath)){
            LittleFS.remove(newFilePath.c_str());
        }
        bool success = LittleFS.rename(filepath, newFilePath.c_str());
        //create header if it's a data log file
        if(filepath != ERROR_LOG_FILE){
            writeFile(filepath, headerString().c_str());
        }
        file = LittleFS.open(filepath, mode);

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
    #ifdef LOG_SYNC
    headerString += ";synced to server";
    #endif
    headerString += "\n";
    return headerString;
}

int Logger::checkForUnsyncedData(std::vector<std::string> knownBLEAdresses, bool syncWithServer = false){
    #ifndef LOG_SYNC
        return -1;
    #endif 
    int numberOfUnsynchedLines = 0;
    int faultyLines = 0;
    Serial.printf("memory free before Maajaaclient: %i\n",ESP.getFreeHeap());
    MaaajaaaClient maaajaaaClient;
    Serial.printf("memory free after Maajaaclient: %i\n",ESP.getFreeHeap());
    std::vector<std::string> filesToCheck = logFileNames;// {std::string("/test.csv")};
    //filesToCheck[0] = logFileNames[0];
    Serial.print("a local file look like: ");
    Serial.println(logFileNames.at(0).c_str());
    Serial.println("checking file");
    Serial.println(filesToCheck.at(0).c_str());
    LittleFS.remove(logFileNames.at(0).c_str());

    //time, moisture, temperature, humidity, signal, sync
    if(!LittleFS.exists(filesToCheck[0].c_str())){
        Serial.println("writing to fileToCheck");
        File f = LittleFS.open(filesToCheck[0].c_str(), "w", true);
        f.print("time;moisture;temp;humi;signal;sync\n");
        f.print("1672613200;1212;2323;4545;5656;0\n");
        f.print("1672613400;1212;2323;4545;5656;1\n");
        f.print("1672613600;1212;2323;4545;5656;1\n");
        f.print("1672613800;1212;2323;4545;5656;0\n");
        f.close();
    }
    Serial.println("checked littlefs");
    //std::vector<std::string> filesToCheck = logFileNames;
    for(int i = 0; i<filesToCheck.size(); i++){
        ESP_LOGE(lTag, "running main loop round %i", i);
        if(LittleFS.exists(filesToCheck.at(i).c_str())){
            //file exists, parse for snychronisation
            ESP_LOGE(lTag, "checking file %i: %s for snycronization",i,  filesToCheck.at(i).c_str());
            //start with the latest, on the bottom of the file
            std::fstream file(std::string("/littlefs") + filesToCheck.at(i), std::ios::out|std::ios::in|std::ios::trunc );
            //std::ofstream file2(logFileNames.at(i));
            //std::fstream file3(logFileNames[i]);
            if(!file.is_open())
                ESP_LOGE(lTag, "file %s could not be opened for sync (check) by std::fstream", filesToCheck.at(i).c_str());

            //get length of first line to know where to stop when reading backwards
            std::string line = "";
            std::getline(file, line);
            int length_first_line = line.length() + 1;
            //as per https://stackoverflow.com/a/11876406

            //read backwards
            int lineNumber = 0;
            file.seekg(0,std::ios_base::end);      //Start at end of file
            char ch = ' ';                        //Init ch not equal to '\n'
            bool firstLineReached = false;
            while(!firstLineReached){
                while(ch != '\n'){
                    file.seekg(-2,std::ios_base::cur); //Two steps back, this means we will NOT check the last character
                    if((int)file.tellg() <= length_first_line){        //if we get to the first line, end it
                        firstLineReached = true;
                        break;
                    }
                    file.get(ch);                      //Check the next character
                }
                std::getline(file,line);
                Serial.print(lineNumber);
                Serial.print(" ");
                Serial.println(line.c_str());
                int numberOfValues = 2; //time and moisture always get logged
                
                #ifdef LOG_TEMPERATURE
                    numberOfValues++;
                #endif
                #ifdef LOG_HUMIDITY
                    numberOfValues++;
                #endif
                #ifdef LOG_SIGNAL
                    numberOfValues++;
                #endif
                #ifdef LOG_SYNC
                    numberOfValues++;
                #endif 
                Serial.println(numberOfValues);
                
                long time = 0;
                //only try to process the line if it cotains ;
                if(std::count(line.begin(), line.end(), ';') >= numberOfValues-1){
                    //PROCESS LINE INTO VARIABLES 
                    time = getCellOfLine(line, 0);
                    Serial.println(time);
                }
                //sanity check of time, if it's completely off we'll not use this dataset for plotting and keep going
                // if time is after 01.01.2023 and there's at least 3 ; in this line of the file
                if(time  > 1672613192){
                    if(syncWithServer){
                        //time, moisture, temperature, humidity, signal, sync
                        std::vector<long> values(6);
                        values = getCellsOfLine(line, 6);
                        //sync
                        Serial.print("val size");
                        Serial.println(values.size());
                        if(values[numberOfValues-1] != 1){
                            Serial.println("connecting to server");
                            if(!maaajaaaClient.connected()){ Serial.println("line 268");
                                maaajaaaClient.connectToServer(); Serial.println("line 269");
                            }
                            Serial.println("line 271");
                            if(maaajaaaClient.connected()){
                                Serial.println("line 273");
                                int resp = 201;//maaajaaaClient.logReading(parasiteDataFromVector(values), knownBLEAdresses[i].c_str(), String(time));
                                Serial.println("line 275");
                                if(resp == 201 || resp%100 == 2){
                                    ESP_LOGI(lTag, "sync of value succeeded");
                                    //replace the 0 with 1
                                    Serial.println("seeking ln 279");
                                    Serial.println(file.tellg());
                                    Serial.println(file.tellp());
                                    file.seekp(line.length()-1,std::ios_base::cur);
                                    Serial.println("seeked");
                                    Serial.println(file.tellg());
                                    Serial.println(file.tellp());
                                    Serial.println("writing");
                                    file.put('1');
                                    //Serial.println("written");
                                }else{
                                    numberOfUnsynchedLines++;
                                }
                            }else{
                                Serial.println("line 288");
                                ESP_LOGD(lTag, "connecting to DB server for sync failed");
                            }
                        }
                    }else{
                        numberOfUnsynchedLines++;
                    }
                    
                }
                Serial.println("reading next line");
                delay(200);
                //NEXT LINE
                //else the -1 won't wort as length() returns unsigned int
                int len = line.length();
                file.seekg(-len-4,std::ios_base::cur); //Two steps back, this means we will NOT check the last character
                Serial.println("next seekg");
                if((int)file.tellg() <= length_first_line){        //If we get to the first line, stop
                    break;
                }
                file.get(ch);                      //Check the next character
                Serial.println("got next char");
                lineNumber++;
        }
        file.close();
        File filereopen = LittleFS.open(filesToCheck.at(0).c_str(), "r");
        Serial.println("changed file: \n");
        while(filereopen.available()){
            Serial.println(filereopen.readStringUntil('\n'));
        }

        delay(60000);
        }
        else{
            ESP_LOGD(lTag, "expected file not found during sync check: %s", logFileNames.at(i));
        }
    }
    return numberOfUnsynchedLines;
}

long Logger::getCellOfLine(std::string line, int column){
    int beginOfCell = 0;
    int endOfCell = 0;
    if(column != 0){
    //determine begin of cell at column - 1st semicolon
    for(int i=0; i<column; i++){
        beginOfCell = line.find(";", beginOfCell) + 1;
    }
    }
    endOfCell = line.find(";", beginOfCell);

    if(endOfCell != -1){
    return std::stol(line.substr(beginOfCell, (endOfCell-beginOfCell)));
    }else{
    return std::stol(line.substr(beginOfCell, (line.length()-beginOfCell)));
    }
}

std::vector<long> Logger::getCellsOfLine(std::string line, int numberOfColumns){
    //Serial.print("getting line ");
    Serial.println(line.c_str());
    std::vector<long> cells(numberOfColumns);
    int semicolonIndeces[numberOfColumns];
    semicolonIndeces[0] = 0;
    //Serial.print("getting indeces: 0");
    for(int i=1; i<numberOfColumns; i++){
        semicolonIndeces[i] = line.find(";", semicolonIndeces[i-1]+1); //+1 to skip the previously found semicolon
        //Serial.printf(", %i", semicolonIndeces[i]);
    }
    //Serial.println("got indeces");
    for(int i=0; i<numberOfColumns; i++){
        //Serial.printf("INDEX %i\n", i);
        cells.at(i) = -1;
        
        int beginOfCell, endOfCell;
        //assign 0th cell if we're at the first cell and reset the +1
        if(i==0){
            beginOfCell = 0;
            endOfCell = semicolonIndeces[1];
        }else{
            beginOfCell = semicolonIndeces[i] + 1; //+1 to skip the semicolon itself
            endOfCell = semicolonIndeces[i+1];
        }

        
        std::string substring;
        if(!(semicolonIndeces[i] == std::string::npos || semicolonIndeces[i-1] == std::string::npos)){
            try{
                if(i==numberOfColumns-1 && endOfCell == -1){
                    substring = line.substr(beginOfCell, (line.length()-beginOfCell));
                } else{
                    substring = line.substr(beginOfCell, (endOfCell-beginOfCell));
                }
                //Serial.println(substring.c_str());
                cells[i] = std::stol(substring);
            }
            catch(std::out_of_range& excep){
                ESP_LOGE(lTag, "substring failed, out of range: %s at column %i of %i", line.c_str(), i, numberOfColumns);
            }catch(std::invalid_argument& excep){
                ESP_LOGE(lTag, "stol failed invalid argument: %s" , substring.c_str());
            }
        }else{
            //Serial.printf("index %i not found\n", i);
            cells[i] = -1;
        }
        /*
        Serial.print(i);
        Serial.print(" ; ");
        Serial.print(substring.c_str());
        Serial.print(":");
        Serial.print(cells[i]);

        Serial.print("[");
        Serial.print(beginOfCell);
        Serial.print(":");
        Serial.print(endOfCell);
        Serial.println("]");
        */
    } 
    return cells;  
}

BParasite_Data_S Logger::parasiteDataFromVector(std::vector<long> vector){
    //time, moisture, temperature, humidity, signal, sync
    BParasite_Data_S data;
    data.soil_moisture = vector.at(1);
    data.temperature = vector.at(2);
    data.humidity = vector.at(3);
    data.rssi = vector.at(4);
    return data;
}