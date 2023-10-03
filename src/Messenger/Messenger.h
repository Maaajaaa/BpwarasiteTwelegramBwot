#include <WiFiClientSecure.h>
#include <numeric>
#include <WiFi.h>
#include <vector>
#include <UniversalTelegramBot.h>
#include <BParasite.h>
#include <config.h>
#include <SPIFFS.h>

class Messenger{
    public:
        Messenger();
        void sendOnlineMessage(std::vector<BParasite_Data_S> parasiteData);
        bool sendOfflineEntwarnung(BParasite_Data_S parasiteData);
        bool sendLowMessage(BParasite_Data_S parasiteData);
        bool sendCriticallyLowMessage(BParasite_Data_S parasiteData);
        bool sendThankYouMessage(BParasite_Data_S parasiteData);
        bool sendOfflineWarning(int minutesOffline, BParasite_Data_S parasiteData);
        void handleUpdates(std::vector<BParasite_Data_S> parasiteData, time_t lastTimeDataReceived[], std::vector<std::string> logFileNames);
        bool ping();

    private:
        void handleNewMessages(int numNewMessages, std::vector<BParasite_Data_S> parasiteData, time_t lastTimeDataReceived[], std::vector<std::string> logFileNames);
        void serialDebug(bool messageSent, String typeOfMessage);
        WiFiClientSecure secured_client;
        UniversalTelegramBot bot = UniversalTelegramBot(BOT_TOKEN, secured_client);
};