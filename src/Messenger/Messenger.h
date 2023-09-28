#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <BParasite.h>

#include <numeric> 
#include <WiFi.h>
#include <vector>
#include <config.h>

class Messenger{
    public:
        Messenger();
        void sendOnlineMessage();
        bool sendOfflineWarning(int index, BParasite_Data_S parasiteData);
        bool sendLowMessage(int index, BParasite_Data_S parasiteData);
        bool sendCriticallyLowMessage(int index, BParasite_Data_S parasiteData);
        bool sendThankYouMessage(int index, BParasite_Data_S parasiteData);
        bool sendOfflineWarning(int index, int minutesOffline);
        void handleUpdates(BParasite parasite);

    private:

        void handleNewMessages(int numNewMessages, BParasite parasite, time_t lastTimeDataReceived[]);
        void serialDebug(bool messageSent, String typeOfMessage, time_t lastTimeDataReceived[]);
        std::vector<std::string> plantNames;
        WiFiClientSecure secured_client;
        UniversalTelegramBot bot = UniversalTelegramBot(BOT_TOKEN, secured_client);
};