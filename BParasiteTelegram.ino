#include "BParasite.h"
#include "esp32-hal-log.h" 

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <numeric> 

#ifdef CORE_DEBUG_LEVEL
#undef CORE_DEBUG_LEVEL
#endif

#define CORE_DEBUG_LEVEL 3
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

const int scanTime = 5; // BLE scan time in seconds
// List of known sensors' BLE addresses
std::vector<std::string> knownBLEAddresses = {"cb:1b:5f:bf:07:aa", "EF:59:22:76:F7:EC", "D4:8C:FB:66:EA:17"};
std::vector<std::string> plantNames = {"ivy", "avocado", "not a plant"};
#define NUMBER_OF_PLANTS 3

// Wifi network station credentials
#define WIFI_SSID "***REMOVED***"
#define WIFI_PASSWORD "***REMOVED***"
// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN "***REMOVED***"
#define CHAT_ID "***REMOVED***"

#define TELEGRAM_DEBUG

//level beyond which the warning message is sent
#define LOW_MOISTURE_LEVEL 35.0

//critical warnings are sent continuously
#define CRITICAL_WARNING_LEVEL 15.0


//send a thank you if watering is back up above this level after being low
#define WATERING_THANKYOU_LEVEL 60.0

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
BParasite parasite(knownBLEAddresses);

bool warningNOTDelivered[NUMBER_OF_PLANTS]= {0};
bool moistureLow[NUMBER_OF_PLANTS] = {0};
bool moistureCritical[NUMBER_OF_PLANTS]= {0};
time_t lastTimeDataReceived[NUMBER_OF_PLANTS] = {0};
time_t lastTimeoutCheck = 0;

void connectToWifi(){
  Serial.print("Connecting to Wifi SSID ");
    Serial.print(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    pinMode(LED_BUILTIN, OUTPUT);
    //turn LED on
    digitalWrite(LED_BUILTIN, LOW); 
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(500);
    }
    Serial.print("\nWiFi connected. IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Retrieving time: ");
    configTime(0, 3600, "pool.ntp.org"); // get UTC time via NTP
    time_t now = time(nullptr);
    while (now < 24 * 3600)
    {
      Serial.print(".");
      delay(100);
      now = time(nullptr);
      configTime(0, 3600, "pool.ntp.org"); // get UTC time via NTP
    }
    Serial.println(now);

    //update time stamps
    for(int i = 0; i<NUMBER_OF_PLANTS; i++){
      lastTimeDataReceived[i] = time(nullptr);
    }
    lastTimeoutCheck = time(nullptr);

    digitalWrite(LED_BUILTIN, HIGH);
}

void setup() {
    Serial.begin(115200);    
    // Initialization
    parasite.begin();
    connectToWifi();   

    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
    String message = String("Reciever of soil moisture of ");
    for(int i = 0; i< plantNames.size(); i++){
      if(i == plantNames.size() -1 && i > 0){
        message += " and ";
      } else if(i > 0){
        message += ", ";
      }
      message += plantNames[i].c_str();
    }
    message = String(message + " is online \xF0\x9F\x8C\xB1 _quiet woof_");
    //markdownv2 requires escape of . characters, so v1 is used for simplicity
    if(!bot.sendMessage(CHAT_ID, message, "Markdown")){
      Serial.println("SENDING MESSAGE FAILED");
    };
}

void loop() {
    // Set sensor data invalid
    parasite.resetData();
    
    // Get sensor data - run BLE scan for <scanTime>
    unsigned found = parasite.getData(scanTime);

    for (int i=0; i < parasite.data.size(); i++) {  
        if (parasite.data[i].valid) {
            Serial.println();
            Serial.printf("Sensor of %d: %s\n", i, plantNames[i].c_str());
            Serial.printf("%.2f°C\n", parasite.data[i].temperature/100.0);
            Serial.printf("%.2f%% humidity\n", parasite.data[i].humidity/100.0);
            Serial.printf("%.3fV\n",  parasite.data[i].batt_voltage/1000.0);
            Serial.printf("%.2f%% soil moisture\n", parasite.data[i].soil_moisture/100.0);
            Serial.printf("%.2flux\n", parasite.data[i].illuminance/100.0);
            Serial.printf("%ddBm\n",  parasite.data[i].rssi);
            Serial.println();
            
            //update the last time it received data
            lastTimeDataReceived[i] = time(nullptr);

            //reconnect if WiFi connection is lost
            //this is done here because the sensor (as long as there is only one or they are synced) 
            //will send again only in 10 minutes so we do have plenty of time
            if(WiFi.status() != WL_CONNECTED){
              connectToWifi();
            }
            
            if( parasite.data[i].soil_moisture/100.0 <= CRITICAL_WARNING_LEVEL){
              warningNOTDelivered[i]=true;
              moistureLow[i]=true;
              moistureCritical[i]=true;
              //continously send critcally low messages
              String message = "\xF0\x9F\x90\xB6 _WOOF_ \xF0\x9F\x90\xB6 moisture of ";
              message += plantNames[i].c_str();
              message += "'s soil CRITICALLY low \xF0\x9F\x98\xB1 ";
              message += parasite.data[i].soil_moisture/100.0;
              message += "%";
              if(!bot.sendMessage(CHAT_ID, message, "markdown")){
                Serial.println("SENDING MESSAGE FAILED");
              }else{
                  warningNOTDelivered[i]=false;
                  Serial.println("critical warning delivered");
              }
            }else if( parasite.data[i].soil_moisture/100.0 <= LOW_MOISTURE_LEVEL){
              //LOW MOISTURE
              Serial.println("LOW MOISTURE");
              moistureCritical[i] = false;
              //detect dropping peak
              if(!moistureLow[i]){
                moistureLow[i] = true;
                warningNOTDelivered[i] = true;
              }
              //deliver warning if neccessarry and connected
              if(warningNOTDelivered[i] && WiFi.status() == WL_CONNECTED){
                String message = "\xF0\x9F\x90\xB6 _woof_ moisture of ";
                message += plantNames[i].c_str();
                message += "'s soil low ";
                message += parasite.data[i].soil_moisture/100.0;
                message += "% \xF0\x9F\x9A\xB1";
                if(!bot.sendMessage(CHAT_ID, message, "Markdown")){
                  Serial.println("SENDING MESSAGE FAILED");
                }else{
                  Serial.println("warning delivered");
                  warningNOTDelivered[i]=false;
                }
              }
            } else if( parasite.data[i].soil_moisture/100.0 >= WATERING_THANKYOU_LEVEL && (moistureLow[i] || moistureCritical[i])){
              moistureCritical[i] = false;
              moistureLow[i] = false;
              warningNOTDelivered[i]=false;
              String message = "Thank you for watering ";
              message += plantNames[i].c_str();
              message += ", soil moisture went up to ";
              message += parasite.data[i].soil_moisture/100.0;
              message += "% \xF0\x9F\x90\xB3 \xF0\x9F\x90\xB3 \xF0\x9F\x90\xB3 _happy panting_";
              if(!bot.sendMessage(CHAT_ID, message, "Markdown")){
                Serial.println("SENDING MESSAGE FAILED");
              }else{
                Serial.println("thank you for watering delivered");
              }
            }
            
         }
    }
    Serial.println("BLE Devices found (total): " + String(found));

    // Delete results from BLEScan buffer to release memory
    parasite.clearScanResults();

    //timeout check every 25 minutes
    if(time(nullptr) - lastTimeoutCheck > 60*25){
      lastTimeoutCheck = time(nullptr);
      for(int i=0; i<NUMBER_OF_PLANTS; i++){
        //if more than 55 minutes since last reading
        if(time(nullptr) - lastTimeDataReceived[i] > 60*55){
          //make sure internet connection is there
          if(WiFi.status() != WL_CONNECTED){
              connectToWifi();
          }
          String message = "\xF0\x9F\x90\xB6 _BARK_ \xF0\x9F\x90\xB6 sensor of ";
              message += plantNames[i].c_str();
              message += "'s has not delivered any new data since ";
              message += time(nullptr) - lastTimeDataReceived[i] / 60;
              message += " minutes! \xf0\x9f\xa7\x90";
              if(!bot.sendMessage(CHAT_ID, message, "Markdown")){
                Serial.println("SENDING MESSAGE FAILED");
              }else{
                  warningNOTDelivered[i]=false;
                  Serial.println("critical warning delivered");
              }
        }
      }
    }

    //LED STUFF
    //at least one with low moisture and none with failed to send warning
    if(std::accumulate(moistureLow, moistureLow+NUMBER_OF_PLANTS,0) && !std::accumulate(warningNOTDelivered, warningNOTDelivered+NUMBER_OF_PLANTS, 0)){
      for(auto i=0; i<1700;){
          digitalWrite(LED_BUILTIN, LOW);  
          delay(200);                      
          digitalWrite(LED_BUILTIN, HIGH);   
          delay(200); 
          i+= 400;                     
      }
    //at least one with low moisture and at least with failed to send warning (which do not have to be the same ones but supposedly should, unless bug)
    }else if(std::accumulate(moistureLow, moistureLow+NUMBER_OF_PLANTS,0) && std::accumulate(warningNOTDelivered, warningNOTDelivered+NUMBER_OF_PLANTS, 0)){
      for(auto i=0; i<=1700;){
          digitalWrite(LED_BUILTIN, LOW);  
          delay(500);                      
          digitalWrite(LED_BUILTIN, HIGH);   
          delay(500); 
          i+= 1000;                     
      }
    }else if(std::accumulate(moistureCritical, moistureCritical+NUMBER_OF_PLANTS,0)){
      for(auto i=0; i<=1800;){
          digitalWrite(LED_BUILTIN, LOW);  
          delay(100);                      
          digitalWrite(LED_BUILTIN, HIGH);   
          delay(200); 
          i+= 300;                     
      }
    }else{
      //readings okay
      delay(2000);
    }
}

