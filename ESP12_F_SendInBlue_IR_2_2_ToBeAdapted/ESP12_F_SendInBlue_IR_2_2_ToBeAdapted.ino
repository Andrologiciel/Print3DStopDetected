/******************************************************************************************
   ESP-12-F Detect collision with Optocoupleur connected to GPIO5 / D04
            Send an email with GMAIL if collision detected
            Push button reboot
   -- v 2.2
   -- When use with KingRoon KP3S, the start position is the board covering IR
   -- So if the collision is detected less than 3' after init it is not the printing end
   -- SendMail inspired by Rui Santos
     - ESP8266: https://RandomNerdTutorials.com/esp8266-nodemcu-send-email-smtp-server-arduino/

******************************************************************************************/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP_Mail_Client.h>

# define LRED   15
# define LGREEN 12
# define LBLUE  13
///////////////////////////////////////////
//========================================
//----------------------
//--- Enter SSID and WIFI PASSWORD
#define WIFI_SSID "xxx"
#define WIFI_PASSWORD "yyy"

/////////////////
#define SMTP_HOST "smtp-relay.sendinblue.com"
#define SMTP_PORT 587
//----------------------
//--- Enter the mail sign in credentials */
#define AUTHOR_EMAIL "xxx@xxx"
#define AUTHOR_PASSWORD "xxxxx"
//----------------------
//--- Enter the Recipient's email*/
#define RECIPIENT_EMAIL "xxx@xxxx"
//========================================
////////////////////////////////////////////

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

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

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
  WiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

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
  Serial.println("<GMAIL_IR ver=v2.2>");
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
  SendPrinterMail("Start printing");
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
      SendPrinterMail("Printing stopped");
      analogWrite( LRED, 0);
      analogWrite( LBLUE, 1023);
    }
  }
  if (start > nTimer && !IsStart)  {
    IsStart = true;
    analogWrite( LGREEN, 0);
    analogWrite( LRED, 1023);
  }
}

void SendPrinterMail(String sMsg) {

  /** Enable the debug via Serial port
     none debug or 0
     basic debug or 1
  */
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the session config data */
  ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = "ESP";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "ESP Test Email New";
  message.addRecipient("Andrologiciels", RECIPIENT_EMAIL);

  /*Send HTML message*/
  String htmlMsg = "<div style=\"color:#2f4468;\"><h1>" + sMsg + "</h1><p>- Sent from ESP board</p></div>";
  message.html.content = htmlMsg.c_str();
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  /*
    //Send raw text message
    String textMsg = "Hello World! - Sent from ESP board";
    message.text.content = textMsg.c_str();
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
    message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;*/

  /* Set the custom message header */
  //message.addHeader("Message-ID: <abcde.fghij@gmail.com>");

  /* Connect to server with the session config */
  if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status) {
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()) {
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
    analogWrite( LBLUE, 0);
    analogWrite( LGREEN, 1023);
  }
}
