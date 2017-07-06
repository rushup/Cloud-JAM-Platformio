#include "mbed.h"
#include "XNucleoNFC01A1.h"
#include "XNucleoIKS01A2.h"
#include "SpwfInterface.h"
#include "NDefLib/NDefNfcTag.h"
#include "NDefLib/RecordType/RecordURI.h"
#include "TCPSocket.h"
#include "HTTPClient.h"
#include "main.h"


#if MAIN_ACTIVE == TEST_SENSORS_AND_NFC

DigitalOut myled(LED1);
Serial pc(USBTX, USBRX);

/* Instantiate the expansion board */
static XNucleoIKS01A2 *mems_expansion_board = XNucleoIKS01A2::instance(D14, D15, D4, D5);

/* Retrieve the composing elements of the expansion board */
static LSM303AGRMagSensor *magnetometer = mems_expansion_board->magnetometer;
static HTS221Sensor *hum_temp = mems_expansion_board->ht_sensor;
static LPS22HBSensor *press_temp = mems_expansion_board->pt_sensor;
static LSM6DSLSensor *acc_gyro = mems_expansion_board->acc_gyro;
static LSM303AGRAccSensor *accelerometer = mems_expansion_board->accelerometer;

/* Helper function for printing floats & doubles */
static char *print_double(char *str, double v, int decimalDigits = 2)
{
  int i = 1;
  int intPart, fractPart;
  int len;
  char *ptr;

  /* prepare decimal digits multiplicator */
  for (; decimalDigits != 0; i *= 10, decimalDigits--)
    ;

  /* calculate integer & fractinal parts */
  intPart = (int)v;
  fractPart = (int)((v - (double)(int)v) * i);

  /* fill in integer part */
  sprintf(str, "%i.", intPart);

  /* prepare fill in of fractional part */
  len = strlen(str);
  ptr = &str[len];

  /* fill in leading fractional zeros */
  for (i /= 10; i > 1; i /= 10, ptr++)
  {
    if (fractPart >= i)
    {
      break;
    }
    *ptr = '0';
  }

  /* fill in (rest of) fractional part */
  sprintf(ptr, "%i", fractPart);

  return str;
}

static void write_url(XNucleoNFC01A1 *nfcNucleo)
{
  //retrieve the NdefLib interface
  NDefLib::NDefNfcTag &tag = nfcNucleo->get_M24SR().get_NDef_tag();

  //open the i2c session with the nfc chip
  if (tag.open_session())
  {
    printf("Session opened\n\r");
    nfcNucleo->get_led1() = 1;

    //create the NDef message and record
    NDefLib::Message msg;
    NDefLib::RecordURI rUri(NDefLib::RecordURI::HTTP_WWW, "google.com");
    msg.add_record(&rUri);

    //write the tag
    if (tag.write(msg))
    {
      printf("Tag written\n\r");
      nfcNucleo->get_led2() = 1;
    }
    else
    {
      printf("Error writing \n\r");
    } //if-else

    //close the i2c session
    if (tag.close_session())
    {
      printf("Session closed\n\r");
      nfcNucleo->get_led3() = 1;
    }
    else
    {
      printf("Error closing the session\n\r");
    } //if-else
  }
  else
    printf("Error opening the session\n\r");
}

int main()
{
  uint8_t id;
  float value1, value2;
  char buffer1[32], buffer2[32];
  int32_t axes[3];
  

  I2C i2cChannel(XNucleoNFC01A1::DEFAULT_SDA_PIN, XNucleoNFC01A1::DEFAULT_SDL_PIN);
  XNucleoNFC01A1 *nfcNucleo = XNucleoNFC01A1::instance(i2cChannel, NULL,
                                                       XNucleoNFC01A1::DEFAULT_GPO_PIN, XNucleoNFC01A1::DEFAULT_RF_DISABLE_PIN,
                                                       XNucleoNFC01A1::DEFAULT_LED1_PIN, XNucleoNFC01A1::DEFAULT_LED2_PIN,
                                                       XNucleoNFC01A1::DEFAULT_LED3_PIN);

  write_url(nfcNucleo);

  /* Enable all sensors */
  hum_temp->enable();
  press_temp->enable();
  magnetometer->enable();
  accelerometer->enable();
  acc_gyro->enable_x();
  acc_gyro->enable_g();

  printf("\r\n--- Starting new run ---\r\n");

  hum_temp->read_id(&id);
  printf("HTS221  humidity & temperature    = 0x%X\r\n", id);
  press_temp->read_id(&id);
  printf("LPS22HB  pressure & temperature   = 0x%X\r\n", id);
  magnetometer->read_id(&id);
  printf("LSM303AGR magnetometer            = 0x%X\r\n", id);
  accelerometer->read_id(&id);
  printf("LSM303AGR accelerometer           = 0x%X\r\n", id);
  acc_gyro->read_id(&id);
  printf("LSM6DSL accelerometer & gyroscope = 0x%X\r\n", id);

  while (1)
  {
    printf("\r\n");

    myled = 1;

    hum_temp->get_temperature(&value1);
    hum_temp->get_humidity(&value2);
    printf("HTS221: [temp] %7s C,   [hum] %s%%\r\n", print_double(buffer1, value1), print_double(buffer2, value2));

    press_temp->get_temperature(&value1);
    press_temp->get_pressure(&value2);
    printf("LPS22HB: [temp] %7s C, [press] %s mbar\r\n", print_double(buffer1, value1), print_double(buffer2, value2));

    printf("---\r\n");

    magnetometer->get_m_axes(axes);
    printf("LSM303AGR [mag/mgauss]:  %6ld, %6ld, %6ld\r\n", axes[0], axes[1], axes[2]);

    accelerometer->get_x_axes(axes);
    printf("LSM303AGR [acc/mg]:  %6ld, %6ld, %6ld\r\n", axes[0], axes[1], axes[2]);

    acc_gyro->get_x_axes(axes);
    printf("LSM6DSL [acc/mg]:      %6ld, %6ld, %6ld\r\n", axes[0], axes[1], axes[2]);

    acc_gyro->get_g_axes(axes);
    printf("LSM6DSL [gyro/mdps]:   %6ld, %6ld, %6ld\r\n", axes[0], axes[1], axes[2]);

    myled = 1;
    wait(1.5);
  }
}

#elif MAIN_ACTIVE == TEST_HTTP

DigitalOut myled(LED1);

//www.mbed.com CA certificate in PEM format
char  CA_cert []="-----BEGIN CERTIFICATE-----\r\n"
"MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\r\n"
"A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\r\n"
"b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\r\n"
"MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\r\n"
"YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\r\n"
"aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\r\n"
"jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\r\n"
"xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\r\n"
"1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\r\n"
"snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\r\n"
"U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\r\n"
"9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\r\n"
"BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\r\n"
"AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\r\n"
"yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\r\n"
"38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\r\n"
"AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\r\n"
"DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\r\n"
"HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\r\n"
"-----END CERTIFICATE-----\r\n";

char str[512];

int main() 
{

    printf("\r\nHTTPClient_HelloWorld mbed Application\r\n");     
    printf("\r\nconnecting to AP...\r\n");
    
    SpwfSAInterface spwf(D8, D2, PC_12, PC_8, false);
    spwf.connect("<SSID>","<PASSWORD>", NSAPI_SECURITY_WPA2);

    const char *ip = spwf.get_ip_address();
    printf("\r\nIP Address is: %s\r\n", ip);

    time_t ctTime;
    ctTime = time(NULL);             
    printf ("Start Secure Socket connection with one way server autentication test\n\r");                
    if (!spwf.get_time(&ctTime)) printf ("ERROR get_time\n\r");     // read WiFi module time
    else set_time(ctTime);  // set Nucleo system time
    printf("Initial System Time is: %s\r\n", ctime(&ctTime));   
    printf("Need to adjust time? if yes enter time in seconds elapsed since Epoch (cmd: date +'%%s'), otherwise enter 0 ");                
    int t=0;
    scanf("%d",&t);
    printf ("Entered time is: %d \n\r", t);
    if (t != 0) { 
       time_t txTm=t; ctTime=t; set_time(txTm);   // set Nucleo system time
       if (!spwf.set_time(ctTime)) printf ("ERROR set_time\n\r");          
    } else {
       if (!spwf.get_time(&ctTime)) printf ("ERROR get_time\n\r");  // read WiFi module time again to compensate time spent on scanf
       else set_time(ctTime);  // set Nucleo system time again to align with Wifi
    }
    printf ("The current system time is: %s\n\r", ctime (&ctTime));  
    spwf.get_time(&ctTime);
    printf ("The current wifi   time is: %s\n\r", ctime (&ctTime));  
    
    HTTPClient http(spwf);
    int ret;       

    if (!spwf.clean_TLS_certificate(ALL)) printf ("ERROR clean_TLS_certificate\n\r");
    if (!spwf.set_TLS_certificate(CA_cert, sizeof(CA_cert), FLASH_CA_ROOT_CERT)) printf ("ERROR set_TLS_certificate\n\r");
    if (!spwf.set_TLS_SRV_domain("*.mbed.com",FLASH_DOMAIN)) printf ("ERROR set_TLS_CA_domain\n\r");
    //GET data
    printf("\nTrying to fetch page...\n\r");
    ret = http.get("https://developer.mbed.org/media/uploads/donatien/hello.txt", str, 128, HTTP_CLIENT_DEFAULT_TIMEOUT);

//ret = http.get("https://developer.mbed.org/media/uploads/donatien/hello.txt", str, 128, HTTP_CLIENT_DEFAULT_TIMEOUT);
//ret = http.get("https://httpbin.org/ip", str, 128, HTTP_CLIENT_DEFAULT_TIMEOUT);
//ret = http.get("https://www.mbed.com", str, 128, HTTP_CLIENT_DEFAULT_TIMEOUT);
//  ret = http.get("http://httpstat.us", str, 128, HTTP_CLIENT_DEFAULT_TIMEOUT);
    if (!ret)
    {
      printf("Page fetched successfully - read %d characters\n\r", strlen(str));
      printf("Result: %s\n", str);
    }
    else
    {
      printf("Error - ret = %d - HTTP return code = %d\n\r", ret, http.getHTTPResponseCode());
    }

    //POST data
    HTTPMap map;
    HTTPText inText(str, 512);
    map.put("Hello", "World");
    map.put("test", "1234");
    printf("\nTrying to post data...\n\r");
    ret = http.post("http://httpbin.org/post", map, &inText, HTTP_CLIENT_DEFAULT_TIMEOUT);
//    ret = http.post("http://posttestserver.com/post.php", map, &inText, HTTP_CLIENT_DEFAULT_TIMEOUT);
    if (!ret)
    {
      printf("Executed POST successfully - read %d characters\n\r", strlen(str));
      printf("Result: %s\n\r", str);
    }
    else
    {
      printf("Error - ret = %d - HTTP return code = %d\n\r", ret, http.getHTTPResponseCode());
    }
    
    //PUT data
    strcpy(str, "This is a PUT test!");
    HTTPText outText(str);
    //HTTPText inText(str, 512);
    printf("\nTrying to put resource...\n\r");
    ret = http.put("http://httpbin.org/put", outText, &inText);
    if (!ret)
    {
      printf("Executed PUT successfully - read %d characters\n\r", strlen(str));
      printf("Result: %s\n\r", str);
    }
    else
    {
      printf("Error - ret = %d - HTTP return code = %d\n\r", ret, http.getHTTPResponseCode());
    }
    
    //DELETE data
    //HTTPText inText(str, 512);
    printf("\nTrying to delete resource...\n\r");
    ret = http.del("http://httpbin.org/delete", &inText);
    if (!ret)
    {
      printf("Executed DELETE successfully - read %d characters\n\r", strlen(str));
      printf("Result: %s\n\r", str);
    }
    else
    {
      printf("Error - ret = %d - HTTP return code = %d\n", ret, http.getHTTPResponseCode());
    }
    
    spwf.disconnect();
    printf ("WIFI disconnected, exiting ...\n\r");

    while(1) { 
      wait(1);
      myled = !myled;
    }
}

#endif
