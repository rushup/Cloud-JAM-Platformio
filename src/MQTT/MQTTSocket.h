#if !defined(MQTTSOCKET_H)
#define MQTTSOCKET_H

#include "MQTTmbed.h"
#include "TCPSocket.h"

class MQTTSocket
{
public:

    int open(NetworkStack *ipstack)
    {
        return mysock.open(ipstack);        
    }
  
    int connect(char* hostname, int port, int timeout=1000)
    {
        int err;
        mysock.set_timeout(timeout);
        err = mysock.connect(hostname, port);  
//    t.start();             
        return err; 
    }

    int read(unsigned char* buffer, int len, int timeout)
    {
        mysock.set_timeout(timeout);        
//t.reset();
// int start = t.read_ms();
        int rc = mysock.recv((char*)buffer, len);
// int stop = t.read_ms();       
//        if (rc>0) printf ("recv File: %s, Line: %d Read nB: %d rc: %d timeout: %d elaps: %d\n\r",__FILE__,__LINE__, len, rc, timeout, stop-start);        
        return rc;
    }
    
    int write(unsigned char* buffer, int len, int timeout)
    {
        mysock.set_timeout(timeout);                
//        mysock.set_blocking(false, timeout);  
//        mysock.set_blocking(false);  
        int rc = mysock.send((char*)buffer, len);
//         printf ("send File: %s, Line: %d Write nB: %d rc: %d\n\r",__FILE__,__LINE__, len, rc);
        return rc;
    }
    
    int disconnect()
    {
//        t.stop();
        return mysock.close();
    }
     
private:
    TCPSocket mysock; 
 //   Timer t;
    
};
#endif
