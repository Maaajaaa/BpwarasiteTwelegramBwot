#include "BParasite.h"
#include "esp32-hal-log.h" 
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include <numeric> 
#include <WiFi.h>

// List of known sensors' BLE addresses
std::vector<std::string> knownBLEAddresses = {"EF:59:22:76:F7:EC", "D4:8C:FB:66:EA:17"};
std::vector<std::string> plantNames = {"avocado", "Maja's ivy"};
//std::vector<std::string> knownBLEAddresses = {"cb:1b:5f:bf:07:aa"};
//std::vector<std::string> plantNames = {"***REMOVED***' ivy"};
//#define NUMBER_OF_PLANTS 1
#define NUMBER_OF_PLANTS 2

#define HOSTNAME "Plantbase One"

// Wifi network station credentials
//#define WIFI_SSID "***REMOVED***"
//#define WIFI_PASSWORD "***REMOVED***"
#define WIFI_SSID "***REMOVED***"
#define WIFI_PASSWORD "***REMOVED***"
// Telegram BOT Token (Get from Botfather)
//#define BOT_TOKEN "***REMOVED***" //***REMOVED*** Bot
#define BOT_TOKEN "***REMOVED***" // Maja Bot
#define CHAT_ID "***REMOVED***" // Maja ***REMOVED*** Group at ***REMOVED*** Bot
//#define CHAT_ID_USER "***REMOVED***" //***REMOVED***
#define CHAT_ID_USER "***REMOVED***" //Maja

#define CHAT_ID_MAJA "***REMOVED***" //Maja @ ***REMOVED*** Bot

//level beyond which the warning message is sent
#define LOW_MOISTURE_LEVEL 35.0

//critical warnings are sent continuously
#define CRITICAL_WARNING_LEVEL 15.0

//time that the sensor has to be offline for a warning to appear in minutes, remember that b-parasite only updates every 10 minutes per default
#define OFFLINE_WARNING_TIME 55


//send a thank you if watering is back up above this level after being low
#define WATERING_THANKYOU_LEVEL 50



//#define TELEGRAM_DEBUG
//#ifdef CORE_DEBUG_LEVEL
//#undef CORE_DEBUG_LEVEL
//#endif

//#define CORE_DEBUG_LEVEL 3
//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG