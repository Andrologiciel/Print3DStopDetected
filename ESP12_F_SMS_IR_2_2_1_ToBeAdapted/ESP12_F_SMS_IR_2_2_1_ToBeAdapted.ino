/******************************************************************************************
   ESP-12-F Detect collision with Optocoupleur connected to GPIO5 / D04
            Send an SMS wtith Freebox if collision detected
            Push button reboot
   -- v 2.2.1
   -- When use with KingRoon KP3S, the start position is the board covering IR
   -- So if the collision is detected less than 3' after init it is not the printing end
   -- For the FreeSms service read : https://www.framboise314.fr/sms-gratuits-vers-les-portables-free/
******************************************************************************************/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

# define LRED   15
# define LGREEN 12
# define LBLUE  13
//
int valueRED = LOW, valueBLUE = LOW, valueGREEN = LOW;


ESP8266WiFiMulti WiFiMulti;

IPAddress ip;
String sIp = "";

const int BUTTON_PIN = 4;    // Define pin the button is connected to
const int ON_BOARD_LED = 2;  // Define pin the on-board LED is connected to
const int LDR_PIN = A0;      // Define the analog pin the LDR is connected to
const int TRAKING_PIN = 5;   // The tracking module attach to pin 5

bool IsStart = false;        // False, the board is on the detector and less than 3' is passed <=> Printing not started
bool SmsSent = false;        // True if an Sms has been sent

unsigned long start, nTimer = 180000; //3*60*1000millisecondes -- Start dé&tection after 3 minutes

///////////////////////////////////////////
//========================================
//----------------------
//--- Enter SSID and WIFI PASSWORD
#define WIFI_SSID "xxx"
#define WIFI_PASSWORD "yyy"
String sFreeSmsUrl ="https://smsapi.free-mobile.fr/sendmsg?user=xxx&pass=xxxx&msg=";
//========================================
///////////////////////////////////////////

//-- Setup
void setup() {

  Serial.begin(115200);

  pinMode(ON_BOARD_LED, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Initialize button pin with built-in pullup.
  digitalWrite(ON_BOARD_LED, HIGH);  // Ensure LED is off
  pinMode(TRAKING_PIN, INPUT);       // set trackingPin as INPUT;

  //////////////////////////////
 
for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
    }

    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(WIFI_SSID, WIFI_SSID);
  
  Serial.println();
  Serial.print("Connecting to AP");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  //////////////////:

  int nRetry = 0;
  while ((WiFi.status() != WL_CONNECTED) && nRetry < 5)
  {
    analogWrite( LBLUE, 1023);
    delay(500);
    analogWrite( LBLUE, 0);
    delay(500);
    nRetry++;
  }
  analogWrite( LBLUE, 0);
  Serial.println("<WiFi connected>");
  Serial.flush();
  Serial.println("<SMS_IR ver=v2.2.1>");
  Serial.print("<ip=");
  ip = WiFi.localIP();
  sIp = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." +  String(ip[3]);
  Serial.print(sIp);
  // Serial.print(WiFi.localIP());
  Serial.println(">");
  Serial.print("<MAC=");
  Serial.print(WiFi.macAddress());
  Serial.println(">");
  Serial.println("<OK>");
  for ( int x = 0 ; x < 3 ; x++ ) {
    analogWrite( LGREEN, 0);
    delay(500);
    analogWrite( LGREEN, 1023);
    delay(500);
  }
    analogWrite( LGREEN, 0);
     SendPrinterSms("Start printing");
}

// the loop function runs over and over again forever
void loop() {
  start = millis();
  int btn_Status = HIGH;
  // Check status of button
  btn_Status = digitalRead (BUTTON_PIN);
  if (btn_Status == LOW) {                // Button pushed, so do something
    analogWrite( LGREEN, 1023);
    delay(500);
    Serial.println(start);
    ESP.restart();
  }
  bool val = digitalRead(TRAKING_PIN); // read the value of tracking module
  if (val == HIGH)
  {
    if ((IsStart) && (!SmsSent)) {
      SmsSent = true;
      SendPrinterSms("Printing stopped");
      analogWrite( LRED, 0);
      analogWrite( LBLUE, 1023);
    }
  }
  if (start > nTimer && !IsStart)  {
    IsStart = true;
    //SendPrinterSms("Start detecting");
    analogWrite( LGREEN, 0);
    analogWrite( LRED, 1023);
  }
}

void SendPrinterSms(String sMsg) {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;
    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, sFreeSmsUrl + sMsg))
    { // HTTPS

      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
      analogWrite( LGREEN, 1023);
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
}
