#include <Arduino.h>
#include <config.h>

const int scanTime = 5; // BLE scan time in seconds


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

void handleNewMessages(int);
void connectToWifiAndGetDST();
void parasiteReadingTask(void *pvParameters);

void connectToWifiAndGetDST(){
  //stop and destroy sntp service//stop smooth time adjustment and set local time manually
  timeval epoch = {0, 0}; //Jan 1 1970
  settimeofday((const timeval*)&epoch, 0);

  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.setHostname(HOSTNAME);
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
  while (now < 3 * 60)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  //if DST couldn't be aquired set manual time (which will be off, but maybe at least the year is close enough for a secure connection)
  if(now < 4 * 60){
    //stop smooth time adjustment and set local time manually
    timeval epoch = {1695159464, 0}; //Sep 19 2023
    settimeofday((const timeval*)&epoch, 0);
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
    xTaskCreate(parasiteReadingTask,   "parasiteReadingTask",      10000,  NULL,        1,   NULL);
    connectToWifiAndGetDST();   

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
        //everything will only be processed while the data is fresh here
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
        }

        //invalid data won't be proccessed unless a message needs to be resent
        if (prstDatCpyAtIndexI.valid || warningNOTDelivered[i] || criticalWarningNOTDelivered[i]){
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

            if (prstDatCpyAtIndexI.valid){   
              //update the last time it received data
              lastTimeDataReceived[i] = time(nullptr);
            }

            //reconnect if WiFi connection is lost
            //this is done here because the sensor (as long as there is only one or they are synced) 
            //will send again only in 10 minutes so we do have plenty of time
            if(WiFi.status() != WL_CONNECTED){
              WiFi.disconnect();
              connectToWifiAndGetDST();
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

            //set data as invalid to signify that it has been processed
            if(mutex != NULL){
              if (xSemaphoreTake(mutex, (TickType_t)10)==pdTRUE) {
                parasiteData[i].valid = false;
                xSemaphoreGive(mutex);
              }
            } else{
              Serial.println("Mutex null");
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
              connectToWifiAndGetDST();
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

    //Handle Bot Updates
    if(bot.getUpdates(bot.last_message_received + 1))
    {
      Serial.println("got response");
      handleNewMessages(1);
    }

    //LED STUFF
    //at least one with low moisture and none with failed to send warning
    bool mstLow  = ( std::accumulate(moistureLow, moistureLow+NUMBER_OF_PLANTS,0) >= 1);
    bool mstCritLow = (std::accumulate(moistureCritical, moistureCritical+NUMBER_OF_PLANTS,0) >= 1);
    bool warnNDel = (std::accumulate(warningNOTDelivered, warningNOTDelivered+NUMBER_OF_PLANTS, 0) >= 1);
    bool critWarnNDel = (std::accumulate(criticalWarningNOTDelivered, criticalWarningNOTDelivered+NUMBER_OF_PLANTS, 0) >= 1);
    if(warnNDel || critWarnNDel){
      WiFi.disconnect();
      connectToWifiAndGetDST();
    }
    if(mstLow && !warnNDel){
      for(auto i=0; i<1400;){
          digitalWrite(LED_BUILTIN, LOW);  
          delay(200);                      
          digitalWrite(LED_BUILTIN, HIGH);   
          delay(200); 
          i+= 400;                     
      }
    //at least one with low moisture and at least with failed to send warning (which do not have to be the same ones but supposedly should, unless bug)
    }else if(mstLow && (warnNDel || critWarnNDel)){
      for(auto i=0; i<=1400;){
          digitalWrite(LED_BUILTIN, LOW);  
          delay(500);                      
          digitalWrite(LED_BUILTIN, HIGH);   
          delay(500); 
          i+= 1000;                     
      }
    }else if(mstCritLow){
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
    int delayTime = 1000;
    parasite.resetData(); // Set sensor data invalid
    parasite.getData(5); // get sensor data (run BLE scan for 5 seconds)
    // makes a copy of each sensor reading under mutex protection
    for (int i=0; i < NUMBER_OF_PLANTS; i++){
      bool dataSaved = false;
      if(mutex != NULL){
        while(!dataSaved && delayTime >= 100){
          //try to get mutex to write data
          if (xSemaphoreTake(mutex, (TickType_t)10)==pdTRUE) {
            //only update data (including the validity) if temperature, soil moisture or humidity have changed
            if(parasiteData[i].temperature != parasite.data[i].temperature 
                || parasiteData[i].soil_moisture != parasite.data[i].soil_moisture 
                || parasiteData[i].humidity != parasite.data[i].humidity){
                  parasiteData[i]=parasite.data[i];
            }
            //else{
            //  parasiteData[i].valid = false;
            //}
            xSemaphoreGive(mutex);
            dataSaved = true;
          }else{
            //if mutex fails try again in 10ms
            delay(10);
            delayTime -= 10;
          }
        }        
      } else{
           Serial.println("Mutex null");
        } 
    } 
    parasite.clearScanResults(); // clear results from BLEScan buffer to release memory
    vTaskDelay(delayTime / portTICK_PERIOD_MS);
  }
}

void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    if(bot.messages[i].chat_id == CHAT_ID_USER || bot.messages[i].chat_id == CHAT_ID_MAJA){
      String message = bot.messages[i].text;
      for(int j = 0; j < NUMBER_OF_PLANTS; j++){
        message += "\n\n*";
        message += plantNames[j].c_str();
        message += "*\nsoil moisture: ";
        message += parasite.data[j].soil_moisture/100.0;
        message += "%\ntemperature: ";
        message += parasite.data[j].temperature/100.0;
        message += "°C\nhumidity (air): ";
        message += parasite.data[j].humidity/100.0;
        message += " %rH\nmeasured: ";
        message += (time(nullptr) - lastTimeDataReceived[j]) / 60;
        message += " minutes ago";
      }
      bot.sendMessage(bot.messages[i].chat_id, message, "Markdown");
    }
  }
}
