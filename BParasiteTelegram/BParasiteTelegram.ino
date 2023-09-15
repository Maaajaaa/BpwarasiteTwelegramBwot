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
std::vector<std::string> knownBLEAddresses = {"cb:1b:5f:bf:07:aa"};
std::vector<std::string> plantNames = {"ivy"};
#define NUMBER_OF_PLANTS 1

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

//time that the sensor has to be offline for a warning to appear in minutes, remember that b-parasite only updates every 10 minutes per default
#define OFFLINE_WARNING_TIME 55


//send a thank you if watering is back up above this level after being low
#define WATERING_THANKYOU_LEVEL 50

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
BParasite parasite(knownBLEAddresses);

BParasite_Data_S parasiteData[NUMBER_OF_PLANTS];
SemaphoreHandle_t mutex;

bool warningNOTDelivered[NUMBER_OF_PLANTS]= {0};
bool criticalWarningNOTDelivered[NUMBER_OF_PLANTS] = {0};
bool offlineWarningNOTDelivered[NUMBER_OF_PLANTS] = {0};
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
    configTime(0, 3600, "time1.google.com"); // get UTC time via NTP
    time_t now = time(nullptr);
    while (now < 24 * 3600)
    {
      Serial.print(".");
      delay(100);
      now = time(nullptr);
    }
    Serial.println(now);

    //update time stamps
    lastTimeoutCheck = time(nullptr);

    digitalWrite(LED_BUILTIN, HIGH);
}

void setup() {
    Serial.begin(115200);    
    // Initialization
    parasite.begin();
    //initialize mutex semaphore
    mutex = xSemaphoreCreateMutex();
    xTaskCreate(parasiteReadingTask,   "parasiteReadingTask",      10000,  NULL,        1,   NULL); // stack size: tried 1000, not sufficient
  //            task function, name of task, stack size, param, priority, handle
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
     
    
    // Get sensor data - run BLE scan for <scanTime>
    //unsigned found = parasite.getData(scanTime);

    for (int i=0; i < NUMBER_OF_PLANTS; i++) { 

        // gets a local copy of the ith sensor reading
        // mutex protection ensures reading struct integrity
        BParasite_Data_S prstDatCpyAtIndexI;
        if(mutex != NULL){
           if (xSemaphoreTake(mutex, (TickType_t)10)==pdTRUE) {
            prstDatCpyAtIndexI = parasiteData[i];
            xSemaphoreGive(mutex);
          } else prstDatCpyAtIndexI.valid = false;
        } else{
           prstDatCpyAtIndexI.valid = false;
           Serial.println("Mutex null");
        }


        //valid means new data and no error in this case
        if (prstDatCpyAtIndexI.valid) {
            Serial.println();
            Serial.printf("Sensor of %d: %s\n", i, plantNames[i].c_str());
            Serial.printf("%.2f°C\n", prstDatCpyAtIndexI.temperature/100.0);
            Serial.printf("%.2f%% humidity\n", prstDatCpyAtIndexI.humidity/100.0);
            Serial.printf("%.3fV\n",  prstDatCpyAtIndexI.batt_voltage/1000.0);
            Serial.printf("%.2f%% soil moisture\n", prstDatCpyAtIndexI.soil_moisture/100.0);
            Serial.printf("%.2flux\n", prstDatCpyAtIndexI.illuminance/100.0);
            Serial.printf("%ddBm\n",  prstDatCpyAtIndexI.rssi);
            Serial.println();

            
            bot.sendMessage("***REMOVED***", "Hellow I've received new data", "markdown");

            //reset offline warning
            offlineWarningNOTDelivered[i] = true;
            
            //send entwarnung if sensor has been offline for too long (as it's reasonable to assume that a warning had been sent)
            if(time(nullptr)-lastTimeDataReceived[i] >= OFFLINE_WARNING_TIME * 60 && lastTimeDataReceived[i] != 0){
              criticalWarningNOTDelivered[i]=false;
              offlineWarningNOTDelivered[i]=false;
              String message = "_woof_  Sensor of ";
              message += plantNames[i].c_str();
              message += " is back online _happy woof_, signal strength is: ";
              message += prstDatCpyAtIndexI.rssi;
              message += " dBm";
              bot.sendMessage(CHAT_ID, message, "markdown");
            }
            //update the last time it received data
            lastTimeDataReceived[i] = time(nullptr);

            //reconnect if WiFi connection is lost
            //this is done here because the sensor (as long as there is only one or they are synced) 
            //will send again only in 10 minutes so we do have plenty of time
            if(WiFi.status() != WL_CONNECTED){
              connectToWifi();
            }
            
            if( prstDatCpyAtIndexI.soil_moisture/100.0 <= CRITICAL_WARNING_LEVEL){
              warningNOTDelivered[i]=false;
              moistureLow[i]=true;
              if(!moistureCritical[i]){
                criticalWarningNOTDelivered[i]=true;
                moistureCritical[i]=true;
              }
              if(criticalWarningNOTDelivered[i]){
                //send critcally low messages
                String message = "\xF0\x9F\x90\xB6 _WOOF_ \xF0\x9F\x90\xB6 moisture of ";
                message += plantNames[i].c_str();
                message += "'s soil CRITICALLY low \xF0\x9F\x98\xB1 ";
                message += prstDatCpyAtIndexI.soil_moisture/100.0;
                message += "%";
                if(!bot.sendMessage(CHAT_ID, message, "markdown")){
                  Serial.println("SENDING MESSAGE FAILED");
                }else{
                    
                    Serial.println("critical warning delivered");
                }
              }
              
            }else if( prstDatCpyAtIndexI.soil_moisture/100.0 <= LOW_MOISTURE_LEVEL){
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
                message += prstDatCpyAtIndexI.soil_moisture/100.0;
                message += "% \xF0\x9F\x9A\xB1";
                if(!bot.sendMessage(CHAT_ID, message, "Markdown")){
                  Serial.println("SENDING MESSAGE FAILED");
                }else{
                  Serial.println("warning delivered");
                  warningNOTDelivered[i]=false;
                }
              }
            } else if( prstDatCpyAtIndexI.soil_moisture/100.0 >= WATERING_THANKYOU_LEVEL && (moistureLow[i] || moistureCritical[i])){
              //reset all warnings
              moistureCritical[i] = false;
              moistureLow[i] = false;
              warningNOTDelivered[i]=false;
              criticalWarningNOTDelivered[i]=false;
              String message = "Thank you for watering ";
              message += plantNames[i].c_str();
              message += ", soil moisture went up to ";
              message += prstDatCpyAtIndexI.soil_moisture/100.0;
              message += "% \xF0\x9F\x90\xB3 \xF0\x9F\x90\xB3 \xF0\x9F\x90\xB3 _happy panting_";
              if(!bot.sendMessage(CHAT_ID, message, "Markdown")){
                Serial.println("SENDING MESSAGE FAILED");
              }else{
                Serial.println("thank you for watering delivered");
              }
            }
            
         }
    }
    //Serial.println("BLE Devices found (total): " + String(found));

    // Delete results from BLEScan buffer to release memory
    //parasite.clearScanResults();

    //timeout check every 25 minutes
    if(time(nullptr) - lastTimeoutCheck > 60*25){
      lastTimeoutCheck = time(nullptr);
      for(int i=0; i<NUMBER_OF_PLANTS; i++){
        //if more than 55 minutes since last reading and no warned yet
        if(time(nullptr) - lastTimeDataReceived[i] > 60*OFFLINE_WARNING_TIME && offlineWarningNOTDelivered[i]){
          //make sure internet connection is there
          if(WiFi.status() != WL_CONNECTED){
              connectToWifi();
          }
          String message = "\xF0\x9F\x90\xB6 _BARK_ \xF0\x9F\x90\xB6 sensor of ";
              message += plantNames[i].c_str();
              message += " has not delivered any new data since ";
              message += (time(nullptr) - lastTimeDataReceived[i]) / 60;
              message += " minutes! \xf0\x9f\xa7\x90";
              if(!bot.sendMessage(CHAT_ID, message, "Markdown")){
                Serial.println("SENDING offline warning MESSAGE FAILED");
              }else{
                  offlineWarningNOTDelivered[i]=false;
                  Serial.println("offline warning delivered");
              }
        }
      }
    }

    //LED STUFF
    //at least one with low moisture and none with failed to send warning
    if(std::accumulate(moistureLow, moistureLow+NUMBER_OF_PLANTS,0) && !std::accumulate(warningNOTDelivered, warningNOTDelivered+NUMBER_OF_PLANTS, 0)){
      for(auto i=0; i<1400;){
          digitalWrite(LED_BUILTIN, LOW);  
          delay(200);                      
          digitalWrite(LED_BUILTIN, HIGH);   
          delay(200); 
          i+= 400;                     
      }
    //at least one with low moisture and at least with failed to send warning (which do not have to be the same ones but supposedly should, unless bug)
    }else if(std::accumulate(moistureLow, moistureLow+NUMBER_OF_PLANTS,0) && 
      (std::accumulate(warningNOTDelivered, warningNOTDelivered+NUMBER_OF_PLANTS, 0) || std::accumulate(criticalWarningNOTDelivered, criticalWarningNOTDelivered+NUMBER_OF_PLANTS, 0))){
      for(auto i=0; i<=1400;){
          digitalWrite(LED_BUILTIN, LOW);  
          delay(500);                      
          digitalWrite(LED_BUILTIN, HIGH);   
          delay(500); 
          i+= 1000;                     
      }
    }else if(std::accumulate(moistureCritical, moistureCritical+NUMBER_OF_PLANTS,0)){
      for(auto i=0; i<=1400;){
          digitalWrite(LED_BUILTIN, LOW);  
          delay(100);                      
          digitalWrite(LED_BUILTIN, HIGH);   
          delay(200); 
          i+= 300;                     
      }
    }else{
      //readings okay
      delay(1400);
    }
}

void parasiteReadingTask(void *pvParameters) {
  while (1) {
    parasite.resetData(); // Set sensor data invalid
    parasite.getData(5); // get sensor data (run BLE scan for 5 seconds)
    // makes a copy of each sensor reading under mutex protection
    for (int i=0; i < NUMBER_OF_PLANTS; i++){
      if(mutex != NULL){
        if (xSemaphoreTake(mutex, (TickType_t)10)==pdTRUE) {
          parasiteData[i] = parasite.data[i];
          xSemaphoreGive(mutex);
        } else parasiteData[i].valid = false;
        } else{
           Serial.println("Mutex null");
        }
    } 
    parasite.clearScanResults(); // clear results from BLEScan buffer to release memory
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

