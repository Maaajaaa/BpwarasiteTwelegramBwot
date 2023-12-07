#include <Messenger/Messenger.h>
static std::string colourMois = std::string("#fbff05");
static std::string colourHumi = std::string("#ff05ff");
static std::string colourTemp = std::string("#05b8ff");

static const char *lTag = "b-Messenger";

Messenger::Messenger(std::vector<std::string> lPlantNames){
  localPlantNames = lPlantNames;
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
}

void Messenger::sendOnlineMessage(std::vector<BParasite_Data_S> parasiteData){
    String message = String("Reciever of soil moisture of ");
    for(int i = 0; i< NUMBER_OF_PLANTS; i++){
      if(i == NUMBER_OF_PLANTS -1 && i > 0){
        message += " and ";
      } else if(i > 0){
        message += ", ";
      }
      message += parasiteData[i].name.c_str();
    }
    message = String(message + " is online \xF0\x9F\x8C\xB1 _quiet woof_");
    //markdownv2 requires escape of . characters, so v1 is used for simplicity
    bool sent = bot.sendMessage(CHAT_ID, message, "Markdown");
    debug(sent, "online message", "this device");
    String data="test\nHello world\n";
    sent = bot.sendMultipartFormDataToTelegramByString("sendDocument", "document", "test.txt", "document/txt", CHAT_ID, data);
    debug(sent, "offline document test", "this device");
    Serial.print("test did");
    Serial.println(sent);
}

//returns 1 if message was sent successfully
bool Messenger::sendOfflineEntwarnung(BParasite_Data_S parasiteData){
    String message = "_woof_  Sensor of ";
    message += parasiteData.name.c_str();
    message += " is back online _happy woof_, signal strength is: ";
    message += parasiteData.rssi;
    message += " dBm";
    bool sent = bot.sendMessage(CHAT_ID, message, "Markdown");
    debug(sent, "offline entwarning", parasiteData.name);
    return sent;
}

bool Messenger::sendCriticallyLowMessage(BParasite_Data_S parasiteData){
    String message = "\xF0\x9F\x90\xB6 _WOOF_ \xF0\x9F\x90\xB6 moisture of ";
    message += parasiteData.name.c_str();
    message += "'s soil CRITICALLY low \xF0\x9F\x98\xB1 ";
    message += parasiteData.soil_moisture/100.0;
    message += "%";
    bool sent = bot.sendMessage(CHAT_ID, message, "Markdown");
    debug(sent, "ciritcally low", parasiteData.name);
    return sent;
}

bool Messenger::sendLowMessage(BParasite_Data_S parasiteData){
    String message = "\xF0\x9F\x90\xB6 _woof_ moisture of ";
    message += parasiteData.name.c_str();
    message += "'s soil low ";
    message += parasiteData.soil_moisture/100.0;
    message += "% \xF0\x9F\x9A\xB1";
    bool sent = bot.sendMessage(CHAT_ID, message, "Markdown");
    debug(sent, "low", parasiteData.name);
    return sent;
}

bool Messenger::sendThankYouMessage(BParasite_Data_S parasiteData){
    String message = "Thank you for watering ";
    message += parasiteData.name.c_str();
    message += ", soil moisture went up to ";
    message += parasiteData.soil_moisture/100.0;
    message += "% \xF0\x9F\x90\xB3 \xF0\x9F\x90\xB3 \xF0\x9F\x90\xB3 _happy panting_";
    bool sent = bot.sendMessage(CHAT_ID, message, "Markdown");
    debug(sent, "thank you", parasiteData.name);
    return sent;
}

bool Messenger::sendOfflineWarning(int minutesOffline, BParasite_Data_S parasiteData){
    String message = "\xF0\x9F\x90\xB6 _BARK_ \xF0\x9F\x90\xB6 sensor of ";
    message += parasiteData.name.c_str();
    message += " has not delivered any new data since ";
    message += minutesOffline;
    message += " minutes! \xf0\x9f\xa7\x90";
    bool sent = bot.sendMessage(CHAT_ID, message, "Markdown");
    debug(sent, "offline warning", parasiteData.name);
    return sent;
}

void Messenger::debug(bool messageSent, String typeOfMessage, std::string plantName){
    if(!messageSent){
        ESP_LOGE(TAG, "FAILED to send %s message for %s", typeOfMessage.c_str(), plantName.c_str());
    }else{        
        ESP_LOGD(TAG, "sent %s message for %s", typeOfMessage.c_str(), plantName.c_str());
    }
}

int Messenger::handleUpdates(std::vector<BParasite_Data_S> parasiteData, time_t lastTimeDataReceived[], std::vector<std::string> logFileNames){
    //Handle Bot Updates
    int numUpdates = bot.getUpdates(bot.last_message_received + 1);
    if(numUpdates > 0)
    {
      ESP_LOGI(lTag, "processing incoming message");
      handleNewMessages(1, parasiteData, lastTimeDataReceived, logFileNames);
      return 1;
    }else if(numUpdates == -1){
      ESP_LOGD(lTag, "connection issue when checking for messages");
      //empty response string meaning some kind of connection issue
      //we return the binary result of the ping to see what's up exactly
      return -1;
    }
    return 0;
}

void Messenger::handleNewMessages(int numNewMessages, std::vector<BParasite_Data_S> parasiteData, time_t lastTimeDataReceived[], std::vector<std::string> logFileNames)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    if(bot.messages[i].chat_id == CHAT_ID_USER || bot.messages[i].chat_id == CHAT_ID_MAJA){
      if(bot.messages[i].text == "log"){
        for(int j=0; j<logFileNames.size(); j++){
          File myFile = SPIFFS.open(logFileNames.at(j).c_str());
          if(myFile){
            ESP_LOGD(lTag, "Sending request for: %s", logFileNames.at(j).c_str());
            std::string fileNameWOSlash = logFileNames.at(j).substr(1,logFileNames.at(j).size()- 1);
            bool sent = bot.sendMultipartFormDataToTelegram("sendDocument", "document", fileNameWOSlash.c_str(), "document/csv", bot.messages[i].chat_id, myFile);
            debug(sent, "csv file", logFileNames.at(j).c_str());
          }
        }
      }else if(bot.messages[i].text == "graph"){
        bot.sendMessage(bot.messages[i].chat_id, String("generating graph, please wait"));
        for(int j=0; j<logFileNames.size(); j++){
          ESP_LOGI(lTag, "generating graph for %s", localPlantNames.at(j));
          unsigned long startTime = millis();
          //generate html with embedded svg String
          String html = String(" <!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body>");
          html += chartSVGFirstBlock(840, 300, 60);
          ESP_LOGD(lTag, "first SVG Block generated");
          html += chartSVGGraph(840, 300, 60, std::string("/spiffs") + logFileNames.at(j), 3 * 24 * 60 * 60, std::string(localPlantNames.at(j)));
          html += String("</body></html>");
          ESP_LOGD(lTag, "writing html to tmp.html file");
          //dump html String into file
          File file = SPIFFS.open("/tmp.html", FILE_WRITE);
          if(!file){
            ESP_LOGE(lTag, "FAILED to open tmp.html for writing");
            return;
          }
          if(file.print(html.c_str())){
            ESP_LOGD(lTag, "writing to tmp.html successful");
          } else {
            ESP_LOGE(lTag, "FAILED to write to tmp.html");
          }
          file.close();
          
          unsigned long endTimeTime = millis();
          ESP_LOGD(lTag, "generating %s graph took %ims", localPlantNames.at(j), endTimeTime - startTime);

          std::string fileNameHTML = logFileNames.at(j).substr(1,logFileNames.at(j).size()- 1) + std::string(".html");
          file = SPIFFS.open("/tmp.html");
          bot.sendChatAction(bot.messages[i].chat_id, "UPLOAD_DOCUMENT");
          bool sent = bot.sendMultipartFormDataToTelegram("sendDocument", "document", fileNameHTML.c_str(), "document/html", bot.messages[i].chat_id, file);
          debug(sent, "graph file", logFileNames.at(j));
          bot.sendMessage(bot.messages[i].chat_id, String(String(endTimeTime-startTime) + String("ms")));
        }
      }else if(bot.messages[i].text == "errors"){
        for(int j=0; j<logFileNames.size(); j++){
          File file = SPIFFS.open(ERROR_LOG_FILE);
          bool sent = bot.sendMultipartFormDataToTelegram("sendDocument", "document", "error_log.txt", "document/html", bot.messages[i].chat_id, file);
          debug(sent, "error log", "this device");
        }
      }
      else{
        String message = bot.messages[i].text;
        message += "\nWiFi signal strength: ";
        message += WiFi.RSSI() ;
        message += " dBm\nFree RAM/Storage: ";
        message += ESP.getFreeHeap();
        message += " bytes / ";
        int tBytes = SPIFFS.totalBytes(); int uBytes = SPIFFS.usedBytes();
        message += (tBytes - uBytes);
        message += " bytes \n";
        for(int j = 0; j < NUMBER_OF_PLANTS; j++){
            message += "\n\n*";
            message += parasiteData[j].name.c_str();
            message += "*\nsoil moisture: ";
            message += parasiteData[j].soil_moisture/100.0;
            message += "%\ntemperature: ";
            message += parasiteData[j].temperature/100.0;
            message += "°C\nhumidity (air): ";
            message += parasiteData[j].humidity/100.0;
            message += " %rH\nsignal strength : ";
            message += parasiteData[j].rssi;
            message += " dbM\nmeasured: ";
            message += (time(nullptr) - lastTimeDataReceived[j]) / 60;
            message += " minutes ago";
        }
        bool sent = bot.sendMessage(bot.messages[i].chat_id, message, "Markdown");
        debug(sent, "status message", "all plants");
      }
    }
  }
}

bool Messenger::ping(){
    if(secured_client.connected()){
        return true;
    }
    bool result = secured_client.connect(TELEGRAM_HOST, TELEGRAM_SSL_PORT);
    if(result == 0){
      ESP_LOGE(lTag, "connecting to Telegram at%s:%s FAILED", TELEGRAM_HOST, TELEGRAM_SSL_PORT);
    }
    return result;
}

String Messenger::chartSVGGraph(int width, int height, int padding, std::string filename, long timeframe, std::string title){
  if(!SPIFFS.begin()){
      return String("SPIFFS formating and mount Failed");
  }
  std::ifstream file(filename);
  if(!file.is_open())
    return String("File not found");

  //get length of first line to know where to stop when reading backwards

  std::string line = "";
  std::getline(file, line);
  int length_first_line = line.length() + 1;
  
  //as per https://stackoverflow.com/a/11876406

  //read backwards
  int lineNumber = 0;
  file.seekg(0,std::ios_base::end);      //Start at end of file
  char ch = ' ';                        //Init ch not equal to '\n'

  //maxs and mins
  long maxTime = 0;
  long minTime = 0;

  int moisMin = 0;
  int moisMax = 0;

  int tempMax = 0;
  int tempMin = 0;
  
  int humiMax = 0;
  int humiMin = 0;

  int skippedLines = 0;
  std::string moisLine("<path d=\"M0 0");
  std::string tempLine("<path d=\"");
  std::string humiLine("<path d=\"M0 0");
  while(maxTime - minTime < timeframe){
    //READ LINE
    while(ch != '\n'){
        file.seekg(-2,std::ios_base::cur); //Two steps back, this means we will NOT check the last character
        if((int)file.tellg() <= length_first_line){        //if we get to the first line, end it
            break;
        }
        file.get(ch);                      //Check the next character
    }
    std::getline(file,line);
    long time = 0;
    //only try to process the line if it cotains ;
    if(std::count(line.begin(), line.end(), ';') >= 3){
      //PROCESS LINE INTO VARIABLES 
      time = getCellOfLine(line, 0);
    }
    //sanity check of time, if it's completely off we'll not use this dataset for plotting and keep going
    // if time is after 01.01.2023 and there's at least 3 ; in this line of the file
    if(time  > 1672613192){
      int mois = getCellOfLine(line, 1);
      int temp = getCellOfLine(line, 2);
      int humi = getCellOfLine(line, 3);

      //SET MAX MIN if it hasn't been set yet
      if(maxTime == 0){
        maxTime = time;
        minTime = time;

        moisMin = mois;
        moisMax = mois;

        tempMax = temp;
        tempMin = temp;
        
        humiMax = humi;
        humiMin = humi;
      }else{
        minTime = time;

        if(mois > moisMax) moisMax = mois;
        if(temp > tempMax) tempMax = temp;
        if(humi > humiMax) humiMax = humi;

        if(mois < moisMin) moisMin = mois;
        if(temp < tempMin) tempMin = temp;
        if(humi < humiMin) humiMin = humi;
      }

      //GRAPH DRAWING
      //we draw in a -100 to 100 coordinate system for Y, X will be in steps of svg_x_stepsize
      //drawing will happen in reading order, and the path will be mirrored later on if needed
      if(lineNumber == 0){
        tempLine += std::string(" M ");
        moisLine += std::string(" M ");
        humiLine += std::string(" M ");
      }
      else{
        tempLine += std::string(" L ");
        moisLine += std::string(" L ");
        humiLine += std::string(" L ");
      }
      unsigned long x_position = (maxTime-time);
      moisLine += std::to_string(x_position) + std::string(" ") + std::to_string(mois);
      tempLine += std::to_string(x_position) + std::string(" ") + std::to_string(temp);
      humiLine += std::to_string(x_position) + std::string(" ") + std::to_string(humi);
    }else{
      skippedLines++;
    }
    //NEXT LINE
    //else the -1 won't wort as length() returns unsigned int
    int len = line.length();
    file.seekg(-len-4,std::ios_base::cur); //Two steps back, this means we will NOT check the last character
    if((int)file.tellg() <= length_first_line){        //If we get to the first line, stop
        break;
    }
    file.get(ch);                      //Check the next character

    lineNumber++;
  }
  //close tag
  float x_scale =  -1.0 * ( (float) width / (maxTime - minTime) );
  float x_translation = padding + width;

  double y_scale_temp = -1.0 * ( (float) height / (tempMax - tempMin) );
  float y_translate_temp =  -1.0*tempMax*y_scale_temp+padding;

  float y_scale_humi = -1.0 * ( (float) height / (humiMax - humiMin));
  float y_translate_humi = -1.0*humiMax*y_scale_humi+padding;

  float y_scale_mois =  -1.0 * ( (float) height / (moisMax - moisMin));
  float y_translate_mois = -1.0*moisMax*y_scale_mois+padding;

  moisLine += std::string("\" stroke=\"") + colourMois + std::string("\" stroke-linejoin=\"round\" stroke-width=\"2.0\"  vector-effect=\"non-scaling-stroke\" fill=\"none\" ");
  moisLine += std::string("transform=\"translate(") + std::to_string(x_translation) +std::string(",") + std::to_string(y_translate_mois) + std::string("), scale(") + std::to_string(x_scale) +
    std::string(",") + std::to_string(y_scale_mois) + std::string(")\"/>");


  humiLine += std::string("\" stroke=\"") + colourHumi + std::string("\" stroke-linejoin=\"round\" stroke-width=\"2.0\" vector-effect=\"non-scaling-stroke\" fill=\"none\" ");
  humiLine += std::string("transform=\"translate(") + std::to_string(x_translation) +std::string(",") + std::to_string(y_translate_humi) + std::string("), scale(") + std::to_string(x_scale) +
    std::string(",") + std::to_string(y_scale_humi) + std::string(")\"/>");

  tempLine += std::string("\" stroke=\"") + colourTemp + std::string("\" stroke-linejoin=\"round\" stroke-width=\"2\" vector-effect=\"non-scaling-stroke\" fill=\"none\" ");
  tempLine += std::string("transform=\"translate(") + std::to_string(x_translation) +std::string(",") + std::to_string(y_translate_temp) + std::string("), scale(") + std::to_string(x_scale) +
    std::string(",") + std::to_string(y_scale_temp) + std::string(")\"/>");

  //log max/min
  ESP_LOGD("SVG Generator", "time (max/min): %i/%i; mois (max/min), %i/%i; temp(max/min/range): %i/%i/%d; humi(max/min) %i/%i; skipped lines: %i",
    maxTime, minTime, moisMax, moisMin, tempMax, tempMin, (tempMax/100.0 - tempMin/100.0), humiMax, humiMin, skippedLines);

  return String(moisLine.c_str())  + String("\n") + String(tempLine.c_str()) + String("\n") + String(humiLine.c_str()) + String("\n") +  chartSVGLastBlock(width,height, padding, tempMin/100.0, tempMax/100.0, moisMin/100.0, moisMax/100.0, humiMin/100.0, humiMax/100.0, (unsigned long)minTime, (unsigned long)maxTime, title);
}

long Messenger::getCellOfLine(std::string line, int column){
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

/// @brief generates the first section of the svg, meaning the diagram background with lines and such, but no text
/// @param width width of the chart
/// @param height height of the chart
/// @param padding padding on all sides
/// @return svg as String
String Messenger::chartSVGLastBlock(int width, int height, int padding, float minTemp, float maxTemp, float minMoisture, float maxMoisture, float minHumidity, float maxHumidity, unsigned long minTime, unsigned long maxTime, std::string title, bool lastBlock){
  return chartSVGFirstAndLastBlock(width, height, padding, minTemp, maxTemp, minMoisture, maxMoisture, minHumidity, maxHumidity, minTime, maxTime, title, true);
}

/// @brief generates the first section of the svg, meaning the diagram background with lines and such, but no text
/// @param width width of the chart
/// @param height height of the chart
/// @param padding padding on all sides
/// @return svg as String
String Messenger::chartSVGFirstBlock(int width, int height, int padding){
  return chartSVGFirstAndLastBlock(width, height, padding, 0, 0, 0, 0, 0, 0, 0, 0, std::string(""), false);
}

/// @brief generates the first section of the svg, meaning the diagram background with lines and such. It is also reused to generate the labels as the coordinates are very similar to the ticks and lines
/// @param width width of the chart
/// @param height height of the chart
/// @param padding padding on all sides
/// @param minTemp highest temperature on the scale
/// @param maxTemp lowest Temperature on the scale
/// @param minMoisture lowest percentage on the scale
/// @param maxMoisture highest percentage on the scale
/// @param minTime lower end of the x axis
/// @param maxTime higher end of the x axis
/// @param title title of the chart
/// @return svg as String
String Messenger::chartSVGFirstAndLastBlock(int width, int height, int padding, float minTemp, float maxTemp, float minMoisture, float maxMoisture, float minHumidity, float maxHumidity, unsigned long minTime, unsigned long maxTime, std::string title, bool lastBlock){
  std::string space = std::string(" ");
  
  std::string svg = std::string("");
  if(!lastBlock){
    //initial stuff and font
    svg = std::string("<?xml version=\"1.0\" standalone=\"no\"?>"
    "\n<svg viewBox=\"0 0 ") + std::to_string(width+padding*2) + space + std::to_string(height+padding*2) 
    + std::string("\" preserveAspectRatio=\"xMidYMid meet\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<defs><defs><style>svg{font-family: -apple-system, system-ui, BlinkMacSystemFont, \"Segoe UI\", Roboto;}</style></defs></defs>");

    //background
    svg += std::string("\n<rect width=\"100%\" height=\"100%\" fill=\"#212733\" />");
  }
  
  //horizontal lines with labels
  int horizontalLines = 5;
  int verticalTicks = 6;
  int verticalLineLength = 10;
  for(int i = 0; i<horizontalLines+1; i++){
    int offset_y = padding + i*height/horizontalLines;
    int font_padding = 5;
    
    if(lastBlock){

      //left labels
      svg += std::string("\n<text x=\"") + std::to_string(padding-font_padding) + std::string("\" y=\"") +  std::to_string(offset_y) + 
        std::string("\" dominant-baseline=\"text-after-edge\" text-anchor=\"end\" font-size=\"12\" fill=\"") + colourMois + std::string("\" font-weight=\"bold\" >") + 
        std::string(String(maxMoisture - i* (maxMoisture-minMoisture)/horizontalLines,1).c_str()) + std::string("%</text>");

      svg += std::string("\n<text x=\"") + std::to_string(padding-font_padding) + std::string("\" y=\"") +  std::to_string(offset_y) + 
        std::string("\" dominant-baseline=\"text-before-edge\" text-anchor=\"end\" font-size=\"12\" fill=\"") + colourHumi + std::string("\" font-weight=\"bold\" >") + 
        std::string(String(maxHumidity - i* (maxHumidity-minHumidity)/horizontalLines,1).c_str()) + std::string("%rH</text>");
      
      //right label
      svg += std::string("\n<text x=\"") + std::to_string(width + padding + font_padding) + std::string("\" y=\"") +  std::to_string(offset_y) + 
        std::string("\" dominant-baseline=\"middle\" text-anchor=\"start\" font-size=\"12\" fill=\"") + colourTemp + std::string("\" font-weight=\"bold\" >") + 
        std::string(String(maxTemp - i*(maxTemp-minTemp)/horizontalLines,1).c_str()) + std::string("°C</text>");

    }else{
      //horizontal line
      if(i==horizontalLines){
        //final line is not dashed
        svg += std::string("\n<path stroke=\"#74838f99\" stroke-width=\"1\"  d=\"M ");
      }else{
        svg += std::string("\n<path stroke=\"#74838f99\" stroke-dasharray=\"10 6\" stroke-width=\"0.5\"  d=\"M ");
      }
      svg += std::to_string(padding) + space + std::to_string(offset_y) + std::string(" L ") + std::to_string(width + padding) + space + std::to_string(offset_y) + std::string("\" />");
    }
    
    //draw vertical ticks and text labels
    if(i == horizontalLines){
      svg+= std::string("\n\n");
      for(int j = 0; j < verticalTicks; j++){
        int offset_x = padding + j*width/horizontalLines;

        if(lastBlock){
          
          //time print as per https://stackoverflow.com/a/16358264
          time_t currentTime = round(minTime + j* (float) (maxTime - minTime)/ (float) (verticalTicks-1));
          struct tm * timeinfo;
          char hours[6];
          char day[11];

          //time (&currentTime);
          timeinfo = localtime(&currentTime);

          strftime(hours,sizeof(hours),"%H:%M",timeinfo);
          strftime(day,sizeof(day),"%d.%m.%Y",timeinfo);
          
          //horizontal labels (time)
          svg += std::string("\n<text x=\"") + std::to_string(offset_x) + std::string("\" y=\"") +  std::to_string(offset_y + verticalLineLength + font_padding) +
            std::string("\" dominant-baseline=\"hanging\" text-anchor=\"middle\" font-size=\"12\" fill=\"#74838f\" font-weight=\"bold\" >") +
            std::string(hours) + std::string("<tspan text-anchor=\"middle\" x=\"") + std::to_string(offset_x) + std::string("\" dy=\"1em\">") + std::string(day) + std::string("</tspan></text>");
        }else{
          //horizontal ticks
          svg += std::string("\n<path stroke=\"#74838f99\" stroke-width=\"1.0\"  d=\" M ") + std::to_string(offset_x) + space +  std::to_string(offset_y) + 
            std::string(" L ") + std::to_string(offset_x) + space + std::to_string(offset_y + verticalLineLength) + std::string("\" />");
        }
      }
    }
    
  }

  if(lastBlock){

    //title text
    svg+= std::string("\n\n<text x=\"50%\" y=\"20\" dominant-baseline=\"middle\" text-anchor=\"middle\" font-size=\"18\" fill=\"#FFF\" font-weight=\"700\" >") +
     title + std::string("</text>");

    //subtitle
    svg+= std::string("<text x=\"50%\" y=\"40\" text-anchor=\"middle\" font-size=\"12\" font-weight=\"700\" fill=\"") + colourMois + std::string("\">Soil Moisture<tspan fill=\"") + colourTemp + std::string("\"> Temperature </tspan><tspan fill=\"") + colourHumi + std::string("\">Humidity</tspan></text>");

    
    //svg end tag
    svg += std::string("\n\n</svg>");
  }

  return String(svg.c_str());
}