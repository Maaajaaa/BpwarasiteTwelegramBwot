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
    bool sent = bot.sendMessage(CHAT_ID, message, "Markdown");
    if(MESSENGER_SERIAL_DEBUG) serialDebug(sent, "offline entwarning");
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
      }else if(bot.messages[i].text == "graph"){
        for(int j=0; j<logFileNames.size(); j++){
          Serial.println(ESP.getFreeHeap());
          String html = String(" <!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body>");
          html += chartSVGFirstBlock(840, 300, 50);
          Serial.println("first block passed");
          html += chartSVGGraph(std::string("/spiffs") + logFileNames.at(j), 3 * 24 * 60 * 60);
          html += String("</body></html>");
          File file = SPIFFS.open("/tmp.html", FILE_WRITE);
          if(!file){
            Serial.println("- failed to open file for writing");
            return;
          }
          if(file.print(html.c_str())){
              Serial.println("- file written");
          } else {
              Serial.println("- write failed");
          }
          file.close();
          std::string fileNameHTML = logFileNames.at(j).substr(1,logFileNames.at(j).size()- 1) + std::string(".html");
          file = SPIFFS.open("/tmp.html");
          bot.sendMultipartFormDataToTelegram("sendDocument", "document", fileNameHTML.c_str(), "document/html", bot.messages[i].chat_id, file);
            
        }
      }
      else{
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
            message += " %rH\nsignal strength : ";
            message += parasiteData[j].rssi;
            message += " dbM\nmeasured: ";
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

String Messenger::chartSVGGraph(std::string filename, long timeframe){
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


  int svg_x_stepsize = 10;
  //maxs and mins
  long maxTime = 0;
  long minTime = 0;

  int moisMin = 0;
  int moisMax = 0;

  int tempMax = 0;
  int tempMin = 0;
  
  int humiMax = 0;
  int humiMin = 0;

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
    Serial.println(line.c_str());
    //PROCESS LINE INTO VARIABLES 
    long time = getCellOfLine(line, 0);
    int mois = getCellOfLine(line, 1);
    int temp = getCellOfLine(line, 2);
    int humi = getCellOfLine(line, 3);

    //SET MAX MIN
    if(lineNumber == 0){
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
    moisLine += std::to_string(svg_x_stepsize*lineNumber) + std::string(" ") + std::to_string(mois);
    tempLine += std::to_string(svg_x_stepsize*lineNumber) + std::string(" ") + std::to_string(temp);
    humiLine += std::to_string(svg_x_stepsize*lineNumber) + std::string(" ") + std::to_string(humi);

    //NEXT LINE
    //else the -1 won't wort as length() returns unsigned int
    int len = line.length();
    file.seekg(-len-4,std::ios_base::cur); //Two steps back, this means we will NOT check the last character
    if((int)file.tellg() <= length_first_line){        //If we get to the first line, stop
        break;
    }
    file.get(ch);                      //Check the next character

    Serial.print("Line: ");
    Serial.print(lineNumber);
    Serial.print(" maxT: ");
    Serial.println(maxTime);
    Serial.print(" minT: ");
    Serial.println(minTime);
    lineNumber++;
  }
  //close tag
  int length = 840;
  int height = 300;
  int padding = 50;
  float x_scale = (float) length / (lineNumber * svg_x_stepsize);

  float y_scale_temp = (float) height / (tempMax - tempMin);
  float y_translate_temp = -1.0*tempMax*y_scale_temp+padding+height;

  int maxPercent = max(moisMax, humiMax);
  int minPercent = min(moisMin, humiMin);


  float y_scale_perc = (float) height / (maxPercent - minPercent);
  float y_translate_perc = -1.0*maxPercent*y_scale_perc+padding+height;

  moisLine += std::string("\" stroke=\"#8ff0a4\" stroke-linejoin=\"round\" stroke-width=\"2.0\"  vector-effect=\"non-scaling-stroke\" fill=\"none\" ");
  moisLine += std::string("transform=\"translate(") + std::to_string(padding) +std::string(",") + std::to_string(y_translate_perc) + std::string("), scale(") + std::to_string(x_scale) +
    std::string(",") + std::to_string(y_scale_perc) + std::string(")\"/>");


  humiLine += std::string("\" stroke=\"#51CE44\" stroke-linejoin=\"round\" stroke-width=\"2.0\" vector-effect=\"non-scaling-stroke\" fill=\"none\" ");
  humiLine += std::string("transform=\"translate(") + std::to_string(padding) +std::string(",") + std::to_string(y_translate_perc) + std::string("), scale(") + std::to_string(x_scale) +
    std::string(",") + std::to_string(y_scale_perc) + std::string(")\"/>");

  tempLine += std::string("\" stroke=\"#56BDB6\" stroke-linejoin=\"round\" stroke-width=\"2\" vector-effect=\"non-scaling-stroke\" fill=\"none\" ");
  tempLine += std::string("transform=\"translate(") + std::to_string(padding) +std::string(",") + std::to_string(y_translate_temp) + std::string("), scale(") + std::to_string(x_scale) +
    std::string(",") + std::to_string(y_scale_temp) + std::string(")\"/>");

  //print max/min
  Serial.print("<!-- time ");
  Serial.print(maxTime);
  Serial.print(" ");
  Serial.print(minTime);

  Serial.print(" mois ");
  Serial.print(moisMax);
  Serial.print(" ");
  Serial.print(moisMin);

  Serial.print(" ");
  Serial.print((tempMax/100.0 - tempMin/100.0));

  Serial.print(" temp ");
  Serial.print(tempMax);
  Serial.print(" ");
  Serial.print(tempMin);

  Serial.print(" humi ");
  Serial.print(humiMax);
  Serial.print(" ");
  Serial.print(humiMin);
  Serial.println(" -->");

  return String(moisLine.c_str())  + String("\n") + String(tempLine.c_str()) + String("\n") + String(humiLine.c_str()) + String("\n") +  chartSVGLastBlock(840,300, 50, tempMin/100.0, tempMax/100.0, minPercent/100.0, maxPercent/100.0, (unsigned long)minTime, (unsigned long)maxTime, "test2");
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
String Messenger::chartSVGLastBlock(int width, int height, int padding, float minTemp, float maxTemp, float minPercent, float maxPercent, unsigned long minTime, unsigned long maxTime, String title){
  return chartSVGFirstAndLastBlock(width, height, padding, minTemp, maxTemp, minPercent, maxPercent, minTime, maxTime, title, true);
}

/// @brief generates the first section of the svg, meaning the diagram background with lines and such, but no text
/// @param width width of the chart
/// @param height height of the chart
/// @param padding padding on all sides
/// @return svg as String
String Messenger::chartSVGFirstBlock(int width, int height, int padding){
  return chartSVGFirstAndLastBlock(width, height, padding, 0, 0, 0, 0, 0, 0, String(""), false);
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
/// @return svg as String
String Messenger::chartSVGFirstAndLastBlock(int width, int height, int padding, float minTemp, float maxTemp, float minPercent, float maxPercent, unsigned long minTime, unsigned long maxTime, String title, bool lastBlock = false){
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
        std::string(String(maxPercent - i* (maxPercent-minPercent)/horizontalLines,1).c_str()) + std::string("%</text>");
      
      //right label
      svg += std::string("\n<text x=\"") + std::to_string(width + padding + font_padding) + std::string("\" y=\"") +  std::to_string(offset_y) + 
        std::string("\" dominant-baseline=\"middle\" text-anchor=\"start\" font-size=\"12\" fill=\"#00aaff\" font-weight=\"bold\" >") + 
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
    svg+= std::string("\n\n<text x=\"50%\" y=\"20\" dominant-baseline=\"middle\" text-anchor=\"middle\" font-size=\"18\" fill=\"#FFF\" font-weight=\"700\" >") +
      std::string(title.c_str()) + std::string("</text>");

    //subtitle
    svg+= std::string("<text x=\"50%\" y=\"40\" text-anchor=\"middle\" font-size=\"12\" font-weight=\"700\" fill=\"#8ff0a4\">Moisture<tspan fill=\"#56BDB6\"> Temperature </tspan><tspan fill=\"#51CE44\">Humidity</tspan></text>");

    
    //svg end tag
    svg += std::string("\n\n</svg>");
  }

  return String(svg.c_str());
}