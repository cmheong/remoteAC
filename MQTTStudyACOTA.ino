/*
 * Based on MQTTAutogateOTA.ino and
 * 
 * IRremote: IRsendRawDemo - demonstrates sending IR codes with sendRaw
 * An IR LED must be connected to Arduino PWM pin 3. (Defualt configuration for UNO class Arduinos)
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 *
 * IRsendRawFlash - added by AnalysIR (via www.AnalysIR.com), 11 April 2016
 *
 * This example shows how to send a RAW signal, from flash instead of SRAM, using the IRremote library.
 *
 * It is more efficient to use the sendNEC style function to send NEC signals.
 * Use of sendRaw_Flash here, serves only as an example of using the function to save SRAM with RAW signals.
 * 
 * Typical Uses: Allows users to store a significant number of signals in flash for sending, without using up SRAM.
 *               This is particularly important for sending RAW AC signal. Typically only one or 2 AC signals could be stored in SRAM using the traditional sendRAW function.
 *               With this function a large number of RAW AC signals can be stored & sent from Flash on a standard Arduino, with no material SRAM overhead.
 *               For even more Flash & signal storage, consider using a Mega1280/2560 with 128K/256K of available Flash vs the standard of 32k bytes.
 *
 * SRAM usage: On the Author's dev system the Arduino IDE 1.6.8 reported only 220 SRAM usage & 8,052 bytes of Flash storage used for this sketch.
 *             In contrast, only one of the signals below will fit on a standard UNO using the example sendRAW function of IRremote (using SRAM to store the signal)
 *
 * 2021-11-13 cmheong created
 * pinout:
 *   VPP = baseboard VIN 9V6 (Max 12V)
 *   GND
 *   led TX = D6  led is salvaged Panasonic remote, possibly IR333 
 *   
 *   D5/GPIO12 led RX led is TSOP4838
 *   D4 (LED_BUILTIN) set to output. Note logic is reversed
 *
 * Aircon remote codes are for Hitachi RAC-EJ10CKM 27degC, fan on, Cooling mode
 * code capture use same hardware setup with AnalysIR.ino, output to serial debug port
 */
 
#include <ESP8266WiFi.h>

#include <ESP8266mDNS.h> // 2019-10-22
#include <WiFiUdp.h> // Needed for NTP as well as mDNSResolver
#include <ArduinoOTA.h>
#include <TimeLib.h>

#include <mDNSResolver.h> // Also needs WiFiUdp.h
#include <PubSubClient.h>
#include <IRremote.h>

IRsend irsend(12); // D6 cmheong 2021-11-02

using namespace mDNSResolver;
WiFiUDP mdns;             // 2019-11-04
Resolver resolver(mdns);

#define WLAN_SSID       "YourWiFiAP"             // 343 main router
#define WLAN_PASS       "password"        
extern "C" {
  #include <user_interface.h>                    // to detect cold starts
}

const char *mqtt_servername = "mqttServer.local";
IPAddress mqtt_serveraddress;
WiFiClient espClient;
PubSubClient mqtt_client(espClient);
#define MQTT_PUBLISH "aircond/messages"
#define MQTT_SUBSCRIBE "aircond/commands"

long lastMsg = 0;
char msg[80];

IPAddress dns(8, 8, 8, 8);  //Google dns
IPAddress staticIP(12,34,56,78); // Your fixed IP address here
IPAddress gateway(12,34,56,1);   // Your gateway IP addr here 
IPAddress subnet(255,255,255,0);
// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(8080);

// NTP Servers:
static const char ntpServerName[] = "pool.ntp.org"; // 2019-03-24 from us.pool.ntp.org
const int timeZone = +8;     // Malaysia Time Time
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();
void digitalClockDisplay();
void digitalClockStr();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

const unsigned int HitachiAC_On[] PROGMEM = {3378, 1696, 448, 1255, 448, 398, 471, 398, 470, 398, 470, 399, 471, 397, 471, 399, 471, 406, 470, 398, 470, 398, 470, 398, 471, 397, 472, 1255, 449, 398, 471, 398, 471, 404, 471, 398, 470, 397, 471, 441, 471, 397, 472, 396, 471, 398, 470, 398, 470, 406, 471, 398, 470, 397, 471, 397, 471, 397, 471, 397, 471, 398, 471, 1256, 448, 404, 471, 1255, 448, 1255, 448, 1256, 448, 1255, 448, 1255, 448, 1256, 449, 397, 471, 1262, 448, 1254, 449, 1254, 448, 1256, 447, 1257, 447, 1255, 448, 1255, 448, 1255, 448, 1263, 448, 398, 471, 396, 471, 397, 471, 398, 471, 418, 471, 398, 471, 397, 471, 405, 470, 399, 470, 398, 470, 1255, 449, 1254, 449, 398, 471, 397, 471, 1255, 448, 1262, 448, 1256, 448, 1255, 448, 398, 470, 397, 471, 1255, 448, 1285, 448, 399, 470, 405, 470, 397, 471, 1255, 449, 397, 471, 398, 470, 1256, 449, 397, 470, 399, 471, 1262, 448, 1255, 449, 398, 470, 1255, 448, 1255, 449, 397, 471, 1256, 448, 1283, 449, 405, 471, 1254, 448, 1256, 448, 398, 470, 398, 471, 1255, 448, 398, 471, 396, 471, 406, 470, 397, 471, 397, 470, 1255, 449, 1255, 449, 397, 471, 1254, 449, 1255, 448, 1290, 449, 398, 470, 398, 471, 1254, 449, 1254, 449, 397, 470, 1255, 449, 1254, 449, 405, 471, 1254, 449, 1254, 449, 398, 470, 397, 471, 1254, 449, 399, 471, 397, 471, 1262, 449, 419, 471, 397, 472, 397, 471, 397, 471, 396, 472, 398, 470, 398, 471, 404, 471, 1277, 450, 1254, 449, 1253, 449, 1253, 451, 1252, 451, 1252, 451, 1253, 449, 1261, 450, 399, 470, 406, 470, 398, 469, 399, 470, 398, 469, 400, 469, 400, 468, 407, 468, 1235, 469, 1234, 468, 1235, 468, 1234, 469, 1235, 468, 1236, 468, 1235, 468, 1243, 468, 401, 468, 399, 469, 429, 468, 401, 468, 400, 467, 401, 468, 400, 468, 409, 468, 1254, 448, 1255, 449, 1255, 448, 1255, 449, 1254, 449, 1254, 447, 1256, 448, 1263, 447, 403, 466, 402, 466, 402, 466, 430, 466, 404, 465, 404, 465, 403, 465, 411, 465, 1259, 443, 1260, 444, 1259, 443, 1261, 441, 1261, 442, 1262, 424, 1279, 416, 1295, 415, 453, 415, 453, 414, 455, 414, 455, 414, 484, 414, 454, 414, 455, 414, 461, 414, 1290, 413, 1291, 412, 1292, 411, 1292, 411, 1314, 389, 1315, 389, 1314, 390, 1321, 389, 1314, 389, 1315, 389, 456, 413, 456, 413, 454, 413, 1315, 389, 455, 413, 464, 412, 457, 412, 456, 412, 1314, 389, 1314, 389, 1314, 389, 456, 412, 1348, 389, 1322, 389, 1314, 389, 458, 411, 480, 389, 476, 391, 1316, 388, 479, 388, 1315, 388, 1323, 387, 480, 389, 1314, 389, 1314, 388, 1316, 389, 479, 388, 1316, 388, 480, 388, 489, 388, 481, 388, 480, 388, 480, 388, 481, 388, 480, 388, 480, 389, 480, 387, 488, 387, 1317, 387, 1315, 388, 1317, 387, 1317, 387, 1316, 387, 1316, 386, 1316, 387, 1325, 386, 481, 387, 481, 388, 481, 387, 481, 388, 480, 387, 481, 388, 481, 387, 488, 387, 1340, 363, 1340, 363, 1340, 363, 1340, 363, 1341, 362, 1342, 362, 1341, 362, 1334, 362}; 
const unsigned int HitachiAC_Off[] PROGMEM = { 189, 63402, 2071, 133, 141, 791670, 3447, 1621, 512, 1189, 512, 355, 512, 355, 513, 354, 512, 355, 512, 355, 513, 356, 512, 362, 513, 354, 513, 354, 513, 355, 512, 354, 512, 1189, 513, 354, 513, 354, 511, 363, 512, 355, 510, 357, 511, 356, 510, 358, 484, 384, 484, 383, 482, 385, 482, 394, 475, 391, 476, 391, 449, 418, 450, 417, 426, 441, 426, 441, 425, 1285, 431, 446, 428, 1269, 434, 1267, 435, 1264, 463, 1237, 464, 1237, 465, 1235, 466, 404, 463, 1309, 424, 1277, 401, 1300, 400, 1257, 445, 1255, 445, 1255, 446, 1255, 444, 1256, 443, 1265, 419, 448, 419, 448, 419, 448, 419, 448, 420, 447, 420, 447, 420, 448, 419, 455, 420, 447, 420, 446, 420, 1282, 418, 1283, 418, 447, 420, 448, 419, 1283, 418, 1290, 418, 1283, 418, 1283, 418, 448, 420, 446, 420, 1282, 419, 1283, 419, 447, 419, 455, 421, 445, 420, 1284, 419, 446, 420, 446, 421, 1282, 419, 447, 421, 445, 422, 1288, 444, 1257, 445, 421, 445, 1255, 447, 1255, 446, 421, 445, 1255, 445, 1256, 444, 431, 421, 1279, 423, 1279, 423, 445, 423, 443, 423, 1277, 424, 445, 422, 444, 423, 452, 422, 445, 422, 445, 421, 1278, 422, 1279, 422, 446, 420, 1279, 421, 1278, 422, 1287, 421, 448, 419, 464, 418, 1280, 421, 1281, 421, 449, 417, 1281, 420, 1282, 420, 458, 415, 1282, 420, 1281, 419, 454, 414, 452, 413, 1283, 420, 454, 413, 453, 413, 1290, 420, 452, 415, 454, 413, 454, 413, 454, 414, 454, 413, 453, 414, 453, 413, 461, 413, 1283, 418, 1283, 418, 1283, 418, 1282, 418, 1283, 417, 1283, 418, 1282, 418, 1292, 417, 454, 413, 454, 413, 454, 412, 455, 413, 454, 413, 455, 412, 454, 413, 461, 412, 1289, 413, 1287, 413, 1288, 412, 1288, 412, 1289, 412, 1290, 412, 1288, 412, 1297, 412, 455, 413, 454, 413, 454, 413, 454, 414, 454, 413, 454, 412, 455, 413, 461, 413, 1288, 413, 1317, 413, 1288, 412, 1289, 411, 1289, 411, 1312, 389, 1311, 389, 1296, 412, 454, 412, 455, 413, 453, 413, 454, 413, 454, 413, 455, 412, 455, 412, 462, 412, 1290, 412, 1289, 411, 1311, 389, 1312, 390, 1311, 389, 1312, 389, 1311, 389, 1320, 389, 454, 412, 455, 412, 455, 412, 455, 412, 455, 411, 455, 413, 454, 412, 463, 412, 1312, 389, 1312, 388, 1314, 388, 1312, 388, 1313, 388, 1312, 389, 1313, 388, 1319, 389, 1311, 388, 1313, 388, 457, 411, 462, 405, 455, 411, 1313, 388, 479, 388, 487, 387, 457, 411, 480, 388, 1312, 388, 1313, 388, 1313, 387, 479, 387, 1313, 388, 1320, 387, 1314, 388, 479, 387, 479, 388, 480, 388, 480, 387, 479, 387, 1313, 387, 1322, 387, 479, 388, 1314, 387, 1314, 387, 1314, 387, 1314, 386, 1315, 386, 480, 387, 488, 387, 479, 387, 480, 387, 481, 386, 481, 386, 480, 387, 501, 386, 481, 386, 487, 386, 1315, 387, 1338, 362, 1338, 362, 1339, 362, 1338, 362, 1337, 363, 1367, 361, 1347, 362, 505, 361, 506, 361, 506, 361, 505, 362, 505, 361, 507, 361, 543, 361, 513, 360, 1342, 336, 1389, 311, 1389, 311, 1390, 310, 1391, 310, 1390, 310, 1390, 310, 1409, 284};

time_t LastOn, LastOff = now();
int ACon = 0;

// 2021-09-28
const int led =LED_BUILTIN; // ESP8266 Pin to which onboard LED is connected

void sendRAW_Flash(const unsigned int * signalArray, unsigned int signalLength, unsigned char carrierFreq) {

  Serial.print("Signal length "); Serial.println(signalLength); Serial.flush();
  digitalWrite(led, LOW);  // logic is reversed
  irsend.enableIROut(carrierFreq); //initialise the carrier frequency for each signal to be sent
  
  for (unsigned int i=0;i<signalLength;i++){
    //tmp=pgm_read_word_near(&signalArray[i]);
   // tmp=cleanPanasonic(tmp); //not needed
    if (i & 1) irsend.space(pgm_read_word_near(&signalArray[i]));
    else irsend.mark(pgm_read_word_near(&signalArray[i]));
  }
  irsend.space(1);//make sure IR is turned off at end of signal
  digitalWrite(led, HIGH);  // logic is reversed
}

void publish_mqtt(const char *str) {
  snprintf (msg, 80, "%s", str);
  // does not mix well with mqtt publish // Serial.print("Publish message: ");
  //Serial.println(msg);
  mqtt_client.publish(MQTT_PUBLISH, msg);
  mqtt_client.loop(); // 2019-11-06
}

// If CPU were to reset, it may pick up the last command on the mqtt server and
// process it. To fix this we alter the mqtt *command* after we have processed it
// 2021-09-08
void makesafe_mqtt() {
  const char *str = "StudyAC_commandcompleted";
  snprintf (msg, 80, "%s", str);
  // does not mix well with mqtt publish // Serial.print("Publish message: ");
  //Serial.println(msg);
  mqtt_client.publish(MQTT_SUBSCRIBE, msg);
  mqtt_client.loop(); // 2019-11-06
}

// 2019-11-04 PubSubClient
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("MQTT message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on and off the remote button on receiving command.
  /*
   *  Note messages are overwritten on top of old ones:
   *  
   */
  if (strncmp((char *)payload, "StudyAC_On", 10)==0) {
    digitalClockStr("Turning on Study Aircond ...");
    Serial.println("Turning on Study Aircond ..."); Serial.flush();
    sendRAW_Flash(HitachiAC_On, sizeof(HitachiAC_On)/sizeof(int),38); // Note 38kHz
    ACon = 1;           
    LastOn = now();
    makesafe_mqtt(); // 2021-09-08
    delay(100);
  }
  else if (strncmp((char*)payload, "StudyAC_Off", 11)==0) 
  {
    digitalClockStr("Turning off Study Aircond ...");
    Serial.println("Turning off Study Aircond ...");
    sendRAW_Flash(HitachiAC_Off, sizeof(HitachiAC_Off)/sizeof(int),38); // Note 38kHz
    ACon = 0;           
    LastOff = now();
    makesafe_mqtt(); // 2021-09-08
    delay(100);
  }
}

long lastReconnectAttempt = 0; // 2019-11-05 Non-blocking reconnect

boolean reconnect() {
  Serial.print("Attempting MQTT reconnection...");
  // Create a random client ID
  String clientId = "MQTTStudyACOTA"; 
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (mqtt_client.connect(clientId.c_str())) {
    Serial.println("connected");
    // Once connected, publish an announcement...
    // client.publish("outTopic", "hello esp8266 mqtt world");
    digitalClockStr("Study Aircond IoT reconnected.");
    mqtt_client.subscribe(MQTT_SUBSCRIBE);
    mqtt_client.loop();
  } else {
      Serial.print("failed, rc=");
      Serial.println(mqtt_client.state());
  }
  return mqtt_client.connected();
}

void setup()
{ // 2019-11-12 Try not to mqtt_publish() as it may not be set up yet
  // Serial.begin(9600);
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);  // 2021-10-31 logic is reversed
  Serial.begin(115200); // 2019-10-19

  Serial.print("Connecting to ");
  
  Serial.println(WLAN_SSID);

  WiFi.mode(WIFI_OFF); // 2018-12-11 workaround from 2018-10-09
  WiFi.mode(WIFI_STA); // 2018-11-27 remove rogue AP
  WiFi.disconnect();
  WiFi.config(staticIP, dns, gateway, subnet);
  WiFi.begin(WLAN_SSID, WLAN_PASS); // 2018-12-11 changed to after WIFI_STA
  while (WiFi.waitForConnectResult() != WL_CONNECTED) { // 2019-10-17
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  randomSeed(micros()); // 2019-11-04
  
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());

   // Start the server
  server.begin();

  rst_info *rinfo;
  rinfo = ESP.getResetInfoPtr();
  Serial.println(String("ResetInfo.reason = ") + (*rinfo).reason);
  // if ((*rinfo).reason == 0)  // power up    
  // There is a problem: 'int gateopen' needs to be saved as well. Until this is done do
  // not apply cold-start code
  // It would be nice to read the relays states from the 2nd cpu  
  delay(100);

  // OTA
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("MQTTStudyACOTA");

  // No authentication by default
  ArduinoOTA.setPassword("drachenzahne");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); 
     
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  Serial.print("Resolving ");
  Serial.println(mqtt_servername);
  resolver.setLocalIP(WiFi.localIP());

  mqtt_serveraddress = resolver.search(mqtt_servername);

  if(mqtt_serveraddress != INADDR_NONE) {
    Serial.print("Resolved: ");
    Serial.println(mqtt_serveraddress);
  } 
  else {
    Serial.println("Not resolved");
    Serial.println("Connection Failed! Rebooting in 5s ...");
    delay(5000);
    ESP.restart();
  }

  mqtt_client.setServer(mqtt_serveraddress, 1883); // Use mdns name
  mqtt_client.setCallback(callback);
  lastReconnectAttempt = 0; // 2019-11-05  
}

// 2020-12-26
// Make html reply to HTTP GET. Note the String s *must* be pre-allocated memory and not
// from the stack
void http_reply(String &s, const String msg) {
  String preamble = "HTTP/1.1 200 OK\r\nContent-Length: "; // need to add \r\n;
  
  s = "<!DOCTYPE HTML>\r\n<html>\r\n" + msg + "\r\n</html>\r\n";

  preamble += String(s.length(), DEC);
  preamble += "\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";

  s = preamble + s;  
}

// Make text reply to HTTP GET. Note the String s *must* be pre-allocated memory and not
// from the stack
void http_reply_txt(String &s, const String msg) {
  String preamble = "HTTP/1.1 200 OK\r\nContent-Length: "; // need to add \r\n;
  
  s = "\r\n" + msg + "\r\n";

  preamble += String(s.length(), DEC);
  preamble += "\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";

  s = preamble + s;  
}

static int dots = 0;
static int val = 0; 
static int commas = 0;

void loop()
{
   ArduinoOTA.handle(); // 2019-10-22
  
   // 2019-02-24 Check for broken wifi links
   if (WiFi.status() != WL_CONNECTED) {
     Serial.println("No WiFi! Initiating reset ...");      
     delay(5000);  // 2019-10-14
     ESP.restart();
   }

  // 2019-11-05 nonblocking reconnects
  if (!mqtt_client.connected()) { 
    long mqtt_now = millis();
    if (mqtt_now - lastReconnectAttempt > 5000) 
    {
      lastReconnectAttempt = mqtt_now;
      Serial.print("mqtt_client.connected() returns false ...");
      Serial.print("Retrying after 5s ...");
      if (reconnect())
      {
        lastReconnectAttempt = 0;
        Serial.println(" reconnected!");
      }
    }
  }
  else 
  {
    long mqtt_now = millis();
    if (mqtt_now - lastMsg > 3600000)
    {
      lastMsg = mqtt_now;
      

      //snprintf(publishstr, 80, "MQTT loop time is %ld\n", mqtt_now);
      //publish_mqtt(publishstr);

      //mqtt_client.publish(MQTT_PUBLISH, publishstr);
      
      //snprintf(publishstr, 80, "%02d:%02d:%02d %02d.%02d.%02d %s\n", hour(), minute(), second(), day(), month(), year(), "Woo hoo!");
      //publish_mqtt(publishstr);

      digitalClockStr("Autogate MQTT loop time");
      // publish_mqtt(publishstr);
    }  
  }
  mqtt_client.loop();

    
  // Check if a client has connected
  WiFiClient client = server.available();
  if ( ! client ) {
    return;
  }

  //Wait until the client sends some data
  while ( ! client.available () )
  {
    delay (10); // 2019-02-28 reduced from 100
    Serial.print(".");
    if (commas++ > 5) {
      commas = 0;
      Serial.println("Terminating client connecting without reading");
      delay(20);
      client.stop();
      return;
    }  
    mqtt_client.loop(); // 2019-12-22
    ArduinoOTA.handle(); // 2019-12-22
  }

  Serial.println("new client connect, waiting for data ");
  
  // Read the first line of the request
  String req = client.readStringUntil ('\r');
  client.flush ();
  
  // Match the request
  String s = "";
  s.reserve(256); // 2021-11-13 required fopr http_reply_txt()
  // Prepare the response
  if (req.indexOf ("/on") != -1)
  {
    digitalClockStr("Turning on Study Aircond from webserver ...");
    Serial.println("Turning on Study Aircond from webserver ..."); Serial.flush();
    sendRAW_Flash(HitachiAC_On, sizeof(HitachiAC_On)/sizeof(int),38); // Note 38kHz
    ACon = 1;           
    LastOn = now();
    digitalClockDisplay();
    //s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nAircond is ";
    //s += (ACon)?"on":"off";
    // s += "</html>\n";      
    String ss = "\r\nStudy Aircond is ";
    ss += (ACon)?"on":"off";   
    http_reply(s, ss);
  } else if (req.indexOf ("/off") != -1) {
      digitalClockStr("Turning off Study Aircond from webserver ...");
      Serial.println("Turning off Study Aircond from webserver ...");
      sendRAW_Flash(HitachiAC_Off, sizeof(HitachiAC_Off)/sizeof(int),38); // Note 38kHz
      ACon = 0;           
      LastOff = now();
      /*
      s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nStudy Aircond is ";
      s += (ACon)?"on":"off";
      // s += val; // 2018-10-25
      s += "</html>\n";
      */
      String ss = "Study Aircond is ";
      ss += (ACon)?"on":"off";   
      http_reply(s, ss);

  } else if (req.indexOf("/index.html") != -1) {
    /*
      s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nStatus is ";
      //s += (val)?"on":"off";
      s += val; // 2018-10-25
      s += " Firmware version: "; // 2019-10-14 
      s += ESP.getSdkVersion();
      s += "</HTML>\n";
      s += "<HTML><body><h1>It works!</h1></body></HTML>";
      */
      
      String ss = "";
      /*
      s = "HTTP/1.1 200 OK\r\n";
      s += "Content-Type: text/html\r\n";
      s += "Connection: close\r\n";
      s += "\r\n<!DOCTYPE HTML><html>";
      */
      ss += "Firmware version:  ";
      ss += ESP.getSdkVersion();
      ss += "\r\nStudy Aircond is ";
      ss += (ACon)?"on":"off";
      /*
      s += "\r\n";
      s += "</html>\r\n";
      */
      http_reply(s, ss); // note arguments cannot be swapped round

      Serial.println("responded to index.html");
  }    
  else {
    Serial.println("invalid request");
    String ss = "";
    // s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nInvalid request</html>\n";
    ss = "Invalid request";
    http_reply(s, ss); // note arguments cannot be swapped round
  }

  client.flush ();
  
  // Send the response to the client
  client.print (s);
  delay (10);
  client.stop(); // 2019-02-24
  Serial.println("Client disonnected");
  delay(10);   
}


int digitalClockStr(const char *message)
{
  char clockstr[80];

  // output digital clock time to char buffer
  snprintf(clockstr, 80, "%02d:%02d:%02d %02d.%02d.%02d %s\n", hour(), minute(), second(), year(), month(), day(), message);
  publish_mqtt(clockstr);
  mqtt_client.loop();
}

void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  // Serial.println();
  // debugV("%d:%d:%d", hour(), minute(), second());
  Serial.print(" ");
}


void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
