
#define TAG "majaStuff"
#define myTAG "majaStuff"
#define ERROR_LOG_FILE "/errors.log"

#define MAJA_conf 1
//#define ***REMOVED***_conf 1
#define CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL_ERROR 1
#define CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL 1
#define CONFIG_ARDUHAL_ESP_LOG 1

#define TIME_BEFORE_RECONNECT 120
#define WIFI_CONNECTION_WAIT 5*60
#define NO_REPEATING_ONLINE_MESSAGES 0

#ifdef MAJA_conf
    #define HOSTNAME "Plantbase One"
    #define WIFI_SSID "***REMOVED***"
    #define WIFI_PASSWORD "***REMOVED***"
    //#define WIFI_SSID "Redmi"
    //#define WIFI_PASSWORD "***REMOVED***"
    // Telegram BOT Token (Get from Botfather)
    #define BOT_TOKEN "***REMOVED***" // Maja Bot
    #define CHAT_ID "***REMOVED***" // Maja ***REMOVED*** Group at ***REMOVED*** Bot
    #define CHAT_ID_USER "***REMOVED***" //Maja
#endif
    
#ifdef ***REMOVED***_conf
    #define HOSTNAME "Plantbase Two"
    // Wifi network station credentials
    #define WIFI_SSID "***REMOVED***"
    #define WIFI_PASSWORD "***REMOVED***"
    #define BOT_TOKEN "***REMOVED***" //***REMOVED*** Bot
    #define CHAT_ID "***REMOVED***" // Maja ***REMOVED*** Group at ***REMOVED*** Bot
    #define CHAT_ID_USER "***REMOVED***" //***REMOVED***
#endif

#define CHAT_ID_MAJA "***REMOVED***" //Maja @ ***REMOVED*** Bot

//level beyond which the warning message is sent
#define LOW_MOISTURE_LEVEL 35.0

//critical warnings are sent continuously
#define CRITICAL_WARNING_LEVEL 15.0

//time that the sensor has to be offline for a warning to appear in minutes, remember that b-parasite only updates every 10 minutes per default
#define OFFLINE_WARNING_TIME 55


//send a thank you if watering is back up above this level after being low
#define WATERING_THANKYOU_LEVEL 50


//Serial Debugging in Messenger Class
#define MESSENGER_SERIAL_DEBUG 1

//Serial Debugging of UniversalTelegramBot
#define TELEGRAM_DEBUG

//ESP Core debug
//#ifdef CORE_DEBUG_LEVEL
//#undef CORE_DEBUG_LEVEL
//#endif

//#define CORE_DEBUG_LEVEL 3
//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG