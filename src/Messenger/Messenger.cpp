#include <Messenger/Messenger.h>

Messenger::Messenger(){
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
    if(!bot.sendMessage(CHAT_ID, message, "Markdown")){
      Serial.println("SENDING MESSAGE FAILED");
    };
}

//returns 1 if message was sent successfully
bool Messenger::sendOfflineEntwarnung(BParasite_Data_S parasiteData){
    String message = "_woof_  Sensor of ";
    message += parasiteData.name.c_str();
    message += " is back online _happy woof_, signal strength is: ";
    message += parasiteData.rssi;
    message += " dBm";
    bool sent = bot.sendMessage(CHAT_ID, message, "mMrkdown");
    if(MESSENGER_SERIAL_DEBUG) serialDebug(sent, "offlineWarning");
    return sent;
}

bool Messenger::sendCriticallyLowMessage(BParasite_Data_S parasiteData){
    String message = "\xF0\x9F\x90\xB6 _WOOF_ \xF0\x9F\x90\xB6 moisture of ";
    message += parasiteData.name.c_str();
    message += "'s soil CRITICALLY low \xF0\x9F\x98\xB1 ";
    message += parasiteData.soil_moisture/100.0;
    message += "%";
    bool sent = bot.sendMessage(CHAT_ID, message, "Markdown");
    if(MESSENGER_SERIAL_DEBUG) serialDebug(sent, "ciritcally low");
    return sent;
}

bool Messenger::sendLowMessage(BParasite_Data_S parasiteData){
    String message = "\xF0\x9F\x90\xB6 _woof_ moisture of ";
    message += parasiteData.name.c_str();
    message += "'s soil low ";
    message += parasiteData.soil_moisture/100.0;
    message += "% \xF0\x9F\x9A\xB1";
    bool sent = bot.sendMessage(CHAT_ID, message, "Markdown");
    if(MESSENGER_SERIAL_DEBUG) serialDebug(sent, "low");
    return sent;
}

bool Messenger::sendThankYouMessage(BParasite_Data_S parasiteData){
    String message = "Thank you for watering ";
    message += parasiteData.name.c_str();
    message += ", soil moisture went up to ";
    message += parasiteData.soil_moisture/100.0;
    message += "% \xF0\x9F\x90\xB3 \xF0\x9F\x90\xB3 \xF0\x9F\x90\xB3 _happy panting_";
    bool sent = bot.sendMessage(CHAT_ID, message, "Markdown");
    if(MESSENGER_SERIAL_DEBUG) serialDebug(sent, "thank you");
    return sent;
}

bool Messenger::sendOfflineWarning(int minutesOffline, BParasite_Data_S parasiteData){
    String message = "\xF0\x9F\x90\xB6 _BARK_ \xF0\x9F\x90\xB6 sensor of ";
    message += parasiteData.name.c_str();
    message += " has not delivered any new data since ";
    message += minutesOffline;
    message += " minutes! \xf0\x9f\xa7\x90";
    bool sent = bot.sendMessage(CHAT_ID, message, "Markdown");
    if(MESSENGER_SERIAL_DEBUG) serialDebug(sent, "offline warning");
    return sent;
}

void Messenger::serialDebug(bool messageSent, String typeOfMessage){
    if(!messageSent){
        Serial.print("SENDING of "); 
        Serial.print(typeOfMessage); 
        Serial.println(" message FAILED"); 
    }else{
        Serial.print(typeOfMessage); 
        Serial.println(" message sent");
    }
}

void Messenger::handleUpdates(std::vector<BParasite_Data_S> parasiteData, time_t lastTimeDataReceived[], std::vector<std::string> logFileNames){
    //Handle Bot Updates
    if(bot.getUpdates(bot.last_message_received + 1))
    {
      Serial.println("got response");
      handleNewMessages(1, parasiteData, lastTimeDataReceived, logFileNames);
    }
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
            Serial.print("Sending request for: ");
            Serial.println(logFileNames.at(j).c_str());
            std::string fileNameWOSlash = logFileNames.at(j).substr(1,logFileNames.at(j).size()- 1);
            bot.sendMultipartFormDataToTelegram("sendDocument", "document", fileNameWOSlash.c_str(), "document/csv", bot.messages[i].chat_id, myFile);
            Serial.println("request sent");
          }
        }
      }else{
        String message = bot.messages[i].text;
        for(int j = 0; j < NUMBER_OF_PLANTS; j++){
            message += "\n\n*";
            message += parasiteData[j].name.c_str();
            message += "*\nsoil moisture: ";
            message += parasiteData[j].soil_moisture/100.0;
            message += "%\ntemperature: ";
            message += parasiteData[j].temperature/100.0;
            message += "°C\nhumidity (air): ";
            message += parasiteData[j].humidity/100.0;
            message += " %rH\nmeasured: ";
            message += (time(nullptr) - lastTimeDataReceived[j]) / 60;
            message += " minutes ago";
        }
        bot.sendMessage(bot.messages[i].chat_id, message, "Markdown");
      }
    }
  }
}

bool Messenger::ping(){
    if(secured_client.connected()){
        return true;
    }
    return secured_client.connect(TELEGRAM_HOST, TELEGRAM_SSL_PORT);
}

/// @brief generates the first section of the svg, meaning the diagram background with lines and such. It is also reused to generate the labels as the coordinates are very similar to the ticks and lines
/// @param width width of the chart
/// @param height height of the chart
/// @param padding padding on all sides
/// @param minTemp highest temperature on the scale
/// @param maxTemp lowest Temperature on the scale
/// @param minPercent lowest percentage on the scale
/// @param maxPercent highest percentage on the scale
/// @param minTime lower end of the x axis
/// @param maxTime higher end of the x axis
/// @param title title of the chart
/// @return 
String chartSVGFirstAndLastBlock(int width, int height, int padding, int minTemp, int maxTemp, int minPercent, int maxPercent, unsigned long minTime, unsigned long maxTime, String title, bool lastBlock = false){
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
      //left label
      svg += std::string("\n<text x=\"") + std::to_string(padding-font_padding) + std::string("\" y=\"") +  std::to_string(offset_y) + 
        std::string("\" dominant-baseline=\"middle\" text-anchor=\"end\" font-size=\"12\" fill=\"#E6FCFB\" font-weight=\"bold\" >") + 
        std::to_string(maxPercent - i* (maxPercent-minPercent)/horizontalLines) + std::string("%</text>");
      
      //right label
      svg += std::string("\n<text x=\"") + std::to_string(width + padding + font_padding) + std::string("\" y=\"") +  std::to_string(offset_y) + 
        std::string("\" dominant-baseline=\"middle\" text-anchor=\"start\" font-size=\"12\" fill=\"#00aaff\" font-weight=\"bold\" >") + 
        std::to_string(maxTemp - i*(maxTemp-minTemp)/horizontalLines) + std::string("°C</text>");

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
          time_t currentTime = maxTime + j* (maxTime - minTime)/verticalTicks;
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
    svg+= std::string("\n\n<text x=\"50%\" y=\"25\" dominant-baseline=\"middle\" text-anchor=\"middle\" font-size=\"18\" fill=\"#FFF\" font-weight=\"700\" >") +
      std::string(title.c_str()) + std::string("</text>");

    //svg end tag
    svg += std::string("</svg>");
  }

  return String(svg.c_str());
}