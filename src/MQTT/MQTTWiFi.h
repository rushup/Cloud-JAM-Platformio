
#if !defined(MQTTWIFI_H)
#define MQTTWIFI_H

#include "MQTTmbed.h"
#include "WiFiInterface.h"
#include "MQTTSocket.h"

class MQTTWiFi : public MQTTSocket
{
public:    
    MQTTWiFi(SpwfSAInterface &WiFiIntf, const char *ssid, const char *pass, nsapi_security_t security = NSAPI_SECURITY_NONE) : WiFi(WiFiIntf)
    {
//        eth.init();                          // Use DHCP
        WiFi.connect(ssid, pass,  security);
    }
    
    SpwfSAInterface& getWiFi()
    {
        return WiFi;
    }
    
/*    void reconnect()
    {
        WiFi.connect();  // nothing I've tried actually works to reconnect 
    }
*/    
private:

SpwfSAInterface& WiFi;
    
};


#endif
