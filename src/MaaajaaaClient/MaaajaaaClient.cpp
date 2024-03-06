#include "MaaajaaaClient.h"

MaaajaaaClient::MaaajaaaClient(std::vector<std::string> localPlantNames)
{
    secured_client.setCACert(howmysslkey);
}

int MaaajaaaClient::connectToServer(){
    /*return*/
    Serial.println("connecting to Maaajaaa");
    //secured_client.connect(dbServer, 443);
    if (!secured_client.connect(dbServer, 443))
        Serial.println("Connection failed!");
    else
    {
        Serial.println("Connected to maaajaaa!");
        // Make a HTTP request:
        secured_client.println("GET https://maaajaaa.de/index.html HTTP/1.0");
        secured_client.println("Host: maaajaaa.de");
        secured_client.println("Connection: close");
        secured_client.println();

        while (secured_client.connected())
        {
            String line = secured_client.readStringUntil('\n');
            if (line == "\r")
            {
                Serial.println("headers received");
                break;
            }
        }
        // if there are incoming bytes available
        // from the server, read them and print them:
        while (secured_client.available())
        {
            char c = secured_client.read();
            Serial.write(c);
        }

        //secured_client.stop();
    }
    return 1;

    WiFiClientSecure client;

    const char *server = "www.howsmyssl.com"; // Server URL
    client.setCACert(dbServerCert);
    Serial.println("\nStarting connection to server...");
    if (!client.connect(server, 443))
        Serial.println("Connection failed!");
    else
    {
        Serial.println("Connected to server!");
        // Make a HTTP request:
        client.println("GET https://www.howsmyssl.com/a/check HTTP/1.0");
        client.println("Host: www.howsmyssl.com");
        client.println("Connection: close");
        client.println();

        while (client.connected())
        {
            String line = client.readStringUntil('\n');
            if (line == "\r")
            {
                Serial.println("headers received");
                break;
            }
        }
        // if there are incoming bytes available
        // from the server, read them and print them:
        while (client.available())
        {
            char c = client.read();
            Serial.write(c);
        }

        client.stop();
    }
    return 1;
}

int MaaajaaaClient::sendRequestGetRespCode(String reqUrl){
    /*return*/
    Serial.println("connecting to Maaajaaa for request");
    //secured_client.connect(dbServer, 443);
    if (!secured_client.connect(dbServer, 443))
        Serial.println("Connection failed!");
    else
    {
        Serial.println("Connected to maaajaaa!");
        secured_client.print("POST https://");
        secured_client.print(reqUrl);
        secured_client.println(" HTTP/1.0");
        secured_client.print("Host: ");
        secured_client.println(dbServer);
        secured_client.println("Content-Type: application/x-www-form-urlencoded");
        secured_client.println(F("Connection: close"));
        if(secured_client.println() == 0){
            log_e("failed to send request %s", reqUrl);
            return -1;
        }

        String responseHeader;
        while(secured_client.connected() && responseHeader.length() <= MAX_HEADER_LENGTH){
            responseHeader = secured_client.readStringUntil('\n');
            Serial.print("got response");
            Serial.println(responseHeader);
            long responseCode = responseHeader.substring(0,2).toInt();
            if(responseCode != 0){
                log_i("responseCode: %i", responseCode);
            }else{
                log_e("failed to read response code in %s", responseHeader);
            }
            return responseCode;
        }

        // if there are incoming bytes available
        // from the server, read them and print them:
        while (secured_client.available()) {
            char c = secured_client.read();
            Serial.write(c);
        }
    }
}

int MaaajaaaClient::createSensor(String MAC, String plantName){
    String url = dbServer;
    url += "/createSensor.php?";
    url += "name=" + plantName;
    url += "&MAC=" + MAC;
    Serial.print("CREATESENSOR ");
    Serial.println(url);
    return sendRequestGetRespCode(url);
}