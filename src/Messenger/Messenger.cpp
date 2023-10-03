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
            bot.sendMultipartFormDataToTelegram("sendDocument", "document", "log.csv", "document/csv", bot.messages[i].chat_id, myFile);
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