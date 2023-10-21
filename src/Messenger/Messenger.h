#include <WiFiClientSecure.h>
#include <numeric>
#include <WiFi.h>
#include <vector>
#include <UniversalTelegramBot.h>
#include <BParasite.h>
#include <config.h>
#include <SPIFFS.h>
#include <fstream>

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
        String chartSVGFirstBlock(int width, int height, int padding);
        String chartSVGLastBlock(int height, int width, int padding, float minTemp, float maxTemp, float minPercent, float maxPercent, unsigned long minTime, unsigned long maxTime, String title);
        String chartSVGGraph(std::string filename, long timeframe);

    private:
        long getCellOfLine(std::string line, int column);
        String chartSVGFirstAndLastBlock(int width, int height, int padding, float minTemp, float maxTemp, float minPercent, float maxPercent, unsigned long minTime, unsigned long maxTime, String title, bool lastBlock);
        void handleNewMessages(int numNewMessages, std::vector<BParasite_Data_S> parasiteData, time_t lastTimeDataReceived[], std::vector<std::string> logFileNames);
        void serialDebug(bool messageSent, String typeOfMessage);
        WiFiClientSecure secured_client;
        UniversalTelegramBot bot = UniversalTelegramBot(BOT_TOKEN, secured_client);
};