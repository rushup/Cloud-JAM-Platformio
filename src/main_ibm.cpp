/* SpwfInterface NetworkSocketAPI Example Program
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "SpwfInterface.h"
#include "TCPSocket.h"
#include "MQTTClient.h"
#include "MQTTWiFi.h"
#include <ctype.h>
#include "XNucleoIKS01A2.h"
#include "XNucleoNFC01A1.h"
#include "NDefLib/NDefNfcTag.h"
#include "NDefLib/RecordType/RecordURI.h"
#include "main.h"

#if MAIN_ACTIVE == TEST_IBM

//------------------------------------
// Hyperterminal configuration
// 9600 bauds, 8-bit data, no parity
//------------------------------------
Serial pc(SERIAL_TX, SERIAL_RX); 
DigitalOut myled(LED1);
bool quickstartMode = true;    

#define ORG_QUICKSTART           // comment to connect to play.internetofthings.ibmcloud.com
//#define SUBSCRIBE              // uncomment to subscribe to broker msgs (not to be used with IBM broker) 
#define X_NUCLEO_NFC01A1_PRESENT // uncomment to add NFC support
    
#define MQTT_MAX_PACKET_SIZE 250   
#define MQTT_MAX_PAYLOAD_SIZE 300 

 // Configuration values needed to connect to IBM IoT Cloud
#define BROKER_URL ".messaging.internetofthings.ibmcloud.com";     
#ifdef ORG_QUICKSTART
#define ORG "quickstart"     // connect to quickstart.internetofthings.ibmcloud.com/ For a registered connection, replace with your org 
#define ID ""
#define AUTH_TOKEN ""
#define DEFAULT_TYPE_NAME "iotsample-mbed-Nucleo"
#else   // not def ORG_QUICKSTART
#define ORG "play"             // connect to play.internetofthings.ibmcloud.com/ For a registered connection, replace with your org
#define ID ""       // For a registered connection, replace with your id
#define AUTH_TOKEN ""// For a registered connection, replace with your auth-token
#define DEFAULT_TYPE_NAME "sensor"
#endif
#define TOPIC  "iot-2/evt/status/fmt/json" 

#define TYPE DEFAULT_TYPE_NAME       // For a registered connection, replace with your type
#define MQTT_PORT 1883
#define MQTT_TLS_PORT 8883
#define IBM_IOT_PORT MQTT_PORT
// WiFi network credential
#define SSID   "ST"  // Network must be visible otherwise it can't connect
#define PASSW  "DEMO"
#warning "Wifi SSID & password empty"
    
char id[30] = ID;                 // mac without colons  
char org[12] = ORG;        
int connack_rc = 0; // MQTT connack return code
const char* ip_addr = "";
char* host_addr = "";
char type[30] = TYPE;
char auth_token[30] = AUTH_TOKEN; // Auth_token is only used in non-quickstart mode
bool netConnecting = false;
int connectTimeout = 1000;
bool mqttConnecting = false;
bool netConnected = false;
bool connected = false;
int retryAttempt = 0;
char subscription_url[MQTT_MAX_PAYLOAD_SIZE];

LPS22HBSensor *pressure_sensor;
HTS221Sensor *humidity_sensor;
HTS221Sensor *temp_sensor1;

MQTT::Message message;
MQTTString TopicName={TOPIC};
MQTT::MessageData MsgData(TopicName, message);

void subscribe_cb(MQTT::MessageData & msgMQTT) {
    char msg[MQTT_MAX_PAYLOAD_SIZE];
    msg[0]='\0';
    strncat (msg, (char*)msgMQTT.message.payload, msgMQTT.message.payloadlen);
    printf ("--->>> subscribe_cb msg: %s\n\r", msg);
}

int subscribe(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
    char* pubTopic = TOPIC;    
    return client->subscribe(pubTopic, MQTT::QOS1, subscribe_cb);
}

int connect(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{ 
    const char* iot_ibm = BROKER_URL; 

    
    char hostname[strlen(org) + strlen(iot_ibm) + 1];
    sprintf(hostname, "%s%s", org, iot_ibm);
    SpwfSAInterface& WiFi = ipstack->getWiFi();
//    ip_addr = WiFi.get_ip_address();
    // Construct clientId - d:org:type:id
    char clientId[strlen(org) + strlen(type) + strlen(id) + 5];  
    sprintf(clientId, "d:%s:%s:%s", org, type, id);  
    sprintf(subscription_url, "%s.%s/#/device/%s/sensor/", org, "internetofthings.ibmcloud.com",id);

    // Network debug statements 
    LOG("=====================================\n\r");
    LOG("Connecting WiFi.\n\r");
    LOG("Nucleo IP ADDRESS: %s\n\r", WiFi.get_ip_address());
    LOG("Nucleo MAC ADDRESS: %s\n\r", WiFi.get_mac_address());
    LOG("Server Hostname: %s port: %d\n\r", hostname, IBM_IOT_PORT);
//    for(int i = 0; clientId[i]; i++){  // set lowercase mac
//       clientId[i] = tolower(clientId[i]); 
//    }    
    LOG("Client ID: %s\n\r", clientId);
    LOG("Topic: %s\n\r",TOPIC);
    LOG("Subscription URL: %s\n\r", subscription_url);
    LOG("=====================================\n\r");
    
    netConnecting = true;
    ipstack->open(&ipstack->getWiFi());
    int rc = ipstack->connect(hostname, IBM_IOT_PORT, connectTimeout);    
    if (rc != 0)
    {
        WARN("IP Stack connect returned: %d\n", rc);    
        return rc;
    }
    printf ("--->TCP Connected\n\r");
    netConnected = true;
    netConnecting = false;

    // MQTT Connect
    mqttConnecting = true;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 4;
    data.struct_version=0;
    data.clientID.cstring = clientId;
 
    if (!quickstartMode) 
    {        
        data.username.cstring = "use-token-auth";
        data.password.cstring = auth_token;
    }   
    if ((rc = client->connect(data)) == 0) 
    {       
        connected = true;
        printf ("--->MQTT Connected\n\r");
#ifdef SUBSCRIBE
        if (!subscribe(client, ipstack)) printf ("--->>>MQTT subscribed to: %s\n\r",TOPIC);
#endif           
    }
    else {
        WARN("MQTT connect returned %d\n", rc);        
    }
    if (rc >= 0)
        connack_rc = rc;
    mqttConnecting = false;
    return rc;
}

int getConnTimeout(int attemptNumber)
{  // First 10 attempts try within 3 seconds, next 10 attempts retry after every 1 minute
   // after 20 attempts, retry every 10 minutes
    return (attemptNumber < 10) ? 3 : (attemptNumber < 20) ? 60 : 600;
}

void attemptConnect(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
    connected = false;
           
    while (connect(client, ipstack) != MQTT_CONNECTION_ACCEPTED) 
    {    
        if (connack_rc == MQTT_NOT_AUTHORIZED || connack_rc == MQTT_BAD_USERNAME_OR_PASSWORD) {
            printf ("File: %s, Line: %d Error: %d\n\r",__FILE__,__LINE__, connack_rc);        
            return; // don't reattempt to connect if credentials are wrong
        } 
        int timeout = getConnTimeout(++retryAttempt);
        WARN("Retry attempt number %d waiting %d\n", retryAttempt, timeout);
        
        // if ipstack and client were on the heap we could deconstruct and goto a label where they are constructed
        //  or maybe just add the proper members to do this disconnect and call attemptConnect(...)        
        // this works - reset the system when the retry count gets to a threshold
        if (retryAttempt == 5)
            NVIC_SystemReset();
        else
            wait(timeout);
    }
}

int publish(MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTTWiFi* ipstack)
{
    MQTT::Message message;
    char* pubTopic = TOPIC;
            
    char buf[MQTT_MAX_PAYLOAD_SIZE];
    float temp, press, hum;
    temp_sensor1->get_temperature(&temp);
    pressure_sensor->get_pressure(&press);
    humidity_sensor->get_humidity(&hum);

    sprintf(buf,
     "{\"d\":{\"ST\":\"Nucleo-IoT-mbed\",\"Temp\":%0.4f,\"Pressure\":%0.4f,\"Humidity\":%0.4f}}",
              temp, press, hum);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf);
    
//    LOG("Publishing %s\n\r", buf);
    printf("Publishing %s\n\r", buf);
    return client->publish(pubTopic, message);
} 
    
int main()
{
    const char * ssid = SSID; // Network must be visible otherwise it can't connect
    const char * seckey = PASSW;
    SpwfSAInterface spwf(D8, D2, false);
    
//    Timer tyeld;
    myled=0;
    DevI2C *i2c = new DevI2C(I2C_SDA, I2C_SCL);
    i2c->frequency(400000);    
    
    XNucleoIKS01A2 *mems_expansion_board = XNucleoIKS01A2::instance(i2c);   
    pressure_sensor = mems_expansion_board->pt_sensor;
    temp_sensor1 = mems_expansion_board->ht_sensor;  
    humidity_sensor = mems_expansion_board->ht_sensor;  
    pressure_sensor->enable();
    humidity_sensor->enable();
    
    pc.printf("\r\nX-NUCLEO-IDW01M1 mbed Application\r\n");     
    pc.printf("\r\nconnecting to AP\r\n");            

   quickstartMode=false;
   if (strcmp(org, "quickstart") == 0){quickstartMode = true;}
   MQTTWiFi ipstack(spwf, ssid, seckey, NSAPI_SECURITY_WPA2);
   MQTT::Client<MQTTWiFi, Countdown, MQTT_MAX_PACKET_SIZE> client(ipstack);
   if (quickstartMode){
        char mac[50];  // remove all : from mac
        char *digit=NULL;
        sprintf (id,"%s", "");                
        sprintf (mac,"%s",ipstack.getWiFi().get_mac_address()); 
        digit = strtok (mac,":");
        while (digit != NULL)
        {
            strcat (id, digit);
            digit = strtok (NULL, ":");
        }     
   }
   attemptConnect(&client, &ipstack);
   if (connack_rc == MQTT_NOT_AUTHORIZED || connack_rc == MQTT_BAD_USERNAME_OR_PASSWORD)    
   {
      while (true)
      wait(1.0); // Permanent failures - don't retry
   }
#ifdef X_NUCLEO_NFC01A1_PRESENT      
   // program NFC with broker URL        
   XNucleoNFC01A1 *nfcNucleo = XNucleoNFC01A1::instance(*i2c, NULL, XNucleoNFC01A1::DEFAULT_GPO_PIN, XNucleoNFC01A1::DEFAULT_RF_DISABLE_PIN, NC,NC,NC);  
   NDefLib::NDefNfcTag& tag = nfcNucleo->get_M24SR().get_NDef_tag();
   printf("NFC Init done: !\r\n");
   //open the i2c session with the nfc chip
   if(tag.open_session()){
       //create the NDef message and record
       NDefLib::Message msg;
       NDefLib::RecordURI rUri(NDefLib::RecordURI::HTTPS, subscription_url);
       msg.add_record(&rUri);
        //write the tag
        if(tag.write(msg)){
            printf("Tag writed \r\n");
        }
        //close the i2c session
        if(!tag.close_session()){
            printf("Error Closing the session\r\n");
        }
    }else printf("Error open Session\r\n");             
#endif    
   myled=1;         
   int count = 0;    
//    tyeld.start();    
    while (true)
    {
        if (++count == 100)
        {               // Publish a message every second
            if (publish(&client, &ipstack) != 0) { 
                myled=0;
                attemptConnect(&client, &ipstack);   // if we have lost the connection                
            } else myled=1;
            count = 0;
        }        
//        int start = tyeld.read_ms();
        client.yield(10);  // allow the MQTT client to receive messages
//        printf ("tyeld: %d\n\r",tyeld.read_ms()-start);
    }
}


#endif