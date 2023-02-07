#include <Arduino.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp32fota.h>
#include "FS.h"
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include <LITTLEFS.h>
#include <WiFiClientSecure.h>

#ifdef DEBUG
#define DEBUG(fmt) Serial.print(fmt)
#define DEBUGLN(fmt) Serial.println(fmt)
#else 
#define DEBUG(fmt)
#define DEBUGLN(fmt)
#endif

#define FORMAT_LITTLEFS_IF_FAILED true
#define FW_VERSION "1.0.36"
#define FW_TITLE "Tank_Level"
#define LED 2
#define DHTTYPE DHT22          // DHT 22  (AM2302)
#define DHTPIN 22 // what pin we're connected to
#define LEVELPIN 4
#define uS_TO_S_FACTOR 1000000
#define FW_STATE_ATTR "fw_state"
#define FW_TITLE_ATTR "fw_title"
#define FW_VERSION_ATTR "fw_version"
#define FW_CYCLE 5
#define THINGSBOARD_SERVER "192.168.67.58"
#define PROVISION_KEY "s6ymhzoxhan5l7k9kwwq"
#define PROVISION_SECRET "ayrubtxsal0l09mg9jra"
#define EDGE_HOST "192.168.67.58"
#define EDGE_SEND_PORT 9999
#define EDGE_SECURE_PORT 9988
#define EDGE_PORT "8080"


typedef struct {
  int deepsleep = 40;
  int fw_cycle = 5;
  int checkFirmware = 0;
} Params;

RTC_DATA_ATTR Params params;

StaticJsonDocument<256> doc;
StaticJsonDocument<200> firmware_info;
WiFiClient client;
HTTPClient http;
char ssid[] = "WanpenResort";  //  your network SSID (name)
char pass[] = "wanpenpromson"; // your network password
esp32FOTA esp32FOTA1("esp32-fota-http", 1, false, true);
String token;
const long interval = 10000; // interval to wait for Wi-Fi connection (milliseconds)
String provisionUrl = "http://" THINGSBOARD_SERVER ":" EDGE_PORT "/api/v1/provision";
String clientAttrUrl = "http://" THINGSBOARD_SERVER ":" EDGE_PORT "/api/v1/";
IPAddress local_IP(192, 168, 70, 95);
IPAddress gateway(192, 168, 70, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(192, 168, 70, 1);
IPAddress dns2(8, 8, 8, 8);
/*
const char* root_ca = "-----BEGIN CERTIFICATE-----\n"
"MIIB7DCCAXOgAwIBAgIUCaZs5ZYTsop78FTAwIHyUU5Zu+YwCgYIKoZIzj0EAwQw\n"
"ETEPMA0GA1UEAwwGaW90X2NhMB4XDTIyMDgyNTA4MzU1MloXDTMyMDgyMjA4MzU1\n"
"MlowETEPMA0GA1UEAwwGaW90X2NhMHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEDxku\n"
"CJAthCvLjAXrI0UXUTRu+tDaBT4q7fe82BA7L+KguWivaOg3geskw+wgLcQWZTmZ\n"
"9TVVr0eZTBIzolsb4e1IDzmzrAS5Bf5WblsXLj5GX0GO5iYF/vISuzBdWvuVo4GL\n"
"MIGIMAwGA1UdEwQFMAMBAf8wHQYDVR0OBBYEFN2GM1zNSs57aDC9fSN3LiUwH8py\n"
"MEwGA1UdIwRFMEOAFN2GM1zNSs57aDC9fSN3LiUwH8pyoRWkEzARMQ8wDQYDVQQD\n"
"DAZpb3RfY2GCFAmmbOWWE7KKe/BUwMCB8lFOWbvmMAsGA1UdDwQEAwIBBjAKBggq\n"
"hkjOPQQDBANnADBkAjAXqJuSQUPWn9iz7JlhkablEEaqRrRNna2DoiBoWlErj4Um\n"
"HXCHtAQVt/sDEJ+hyIQCMC29amp5mhYlv+Qat5HqMzVclFXp3AzxMjG+qx2eLInb\n"
"WxKyFfq1Jx2QD+UIeNSMNg==\n"
"-----END CERTIFICATE-----\n";
*/
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIBwzCCAWkCFEMeqs2FD8k+VsS5e0pBjxPCgjDHMAoGCCqGSM49BAMCMGQxCzAJ\n" \
"BgNVBAYTAkFVMQwwCgYDVQQIDANOU1cxEDAOBgNVBAcMB1dvdyBXb3kxDzANBgNV\n" \
"BAoMBkZJTlBFTjEMMAoGA1UECwwDZXNwMRYwFAYDVQQDDA0xOTIuMTY4LjY3LjU4\n" \
"MB4XDTIzMDIwNTAyNTIxNloXDTMzMDIwMjAyNTIxNlowZDELMAkGA1UEBhMCQVUx\n" \
"DDAKBgNVBAgMA05TVzEQMA4GA1UEBwwHV293IFdveTEPMA0GA1UECgwGRklOUEVO\n" \
"MQwwCgYDVQQLDANlc3AxFjAUBgNVBAMMDTE5Mi4xNjguNjcuNTgwWTATBgcqhkjO\n" \
"PQIBBggqhkjOPQMBBwNCAASVTQU/Y575g36A4+8NIw372eCfVUjdXEKszAipbcwZ\n" \
"rSTMVgpI0wlsP5szoAofQObB9qmmP2SF7tzHcRCb6+9hMAoGCCqGSM49BAMCA0gA\n" \
"MEUCIBR+k64MiZ3vUOWbfba/XzHjeAzM9oiE7Z1BihzdlMs6AiEArzFAZzFjggch\n" \
"AEfzm+fwyxKVzwLiFXV03ZAbMaeNEng=\n" \
"-----END CERTIFICATE-----\n" \
"";

WiFiClientSecure secureclient;

void writeFile(fs::FS &fs, const char *path, String value)
{
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }

  file.print(value.c_str());
  file.close();
  Serial.println("Finished writing");
}

bool readTokenFile(fs::FS &fs, const char *path, String *data)
{
  // Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return false;
  }

  while (file.available())
  {
    *data = file.readStringUntil('\n');
  }
  file.close();
  // Serial.println("Read token.txt"+token);
  return true;
}
void connect_wifi()
{
  
  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet, dns1, dns2);
  // Serial.println(ssid);
  // Serial.println(pass);
  WiFi.begin(ssid, pass);
  // Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }
  Serial.println();
}
void sendSecureDataTcp() {
    secureclient.setCACert(root_ca);
    //secureclient.setInsecure();
    Serial.println("Checking securetcp");
  if (!secureclient.connect(EDGE_HOST, EDGE_SECURE_PORT))
    { 
      Serial.println("Connection failed!");
    } else {
      String data = "Secured !!";
      secureclient.print(data);
      secureclient.stop();
    }
}
void sendDataTcp(float t, float h, int l)
{
  if (!client.connect(EDGE_HOST, EDGE_SEND_PORT))
      {
        Serial.println("connection failed");
        return;
  }
  String tcpreply;
  int deepsleep;
  int fw_cycle;
  String md5 = ESP.getSketchMD5();
  String mac = WiFi.macAddress();
  int rssi = WiFi.RSSI();
  unsigned long uptime = millis();
  int ota;
  String data = "{\"current_fw_title\":\"" FW_TITLE "\",\"current_fw_version\":\"" FW_VERSION 
                "\",\"uptime\":" + String(uptime) + 
                ",\"rssi\":" + String(rssi) + 
                ",\"mac\":\"" + mac + 
                "\",\"md5\":\"" + md5 + 
                "\",\"level\":" + l + 
                ",\"temperature\":" + String(t, 2) + 
                ",\"humidity\":" + String(h, 0) + "}";
  client.print(data);
  tcpreply = client.readStringUntil('\r');
  //Serial.println(tcpreply);

  deserializeJson(doc, tcpreply);
  ota = doc["ota"];
  deepsleep = doc["deepsleep"];
  fw_cycle = doc["fw_cycle"];

  if (deepsleep)
  {
    DEBUG("Deepsleep : ");
    //Serial.print("Deep sleep value : ");
    DEBUGLN(deepsleep);
    //Serial.println(deepsleep);
    esp_sleep_enable_timer_wakeup(deepsleep * uS_TO_S_FACTOR);
    params.deepsleep = deepsleep;
  }
  if (fw_cycle) {
    DEBUG("Firmware Cycle :");
    //Serial.print("Firmware Cycle : ");
    DEBUGLN(fw_cycle);
    //Serial.println(fw_cycle);
    params.fw_cycle = fw_cycle;
  }
  client.stop();
  DEBUG("OTA : ");
  //Serial.print("OTA : ");
  DEBUGLN(ota);
  //Serial.println(ota);
  if (ota)
  {
    //Serial.println("OTA received : true");
    //we should initiate ota
  }
  else
  {
    Serial.println("OTA received : False");
    delay(10);
    //esp_deep_sleep_start();
  }
}

void sensorDhtData(float *t, float *h) {
  DHT dht(DHTPIN, DHTTYPE);
  dht.begin();
  *t = dht.readTemperature();
  *h = dht.readHumidity();
}

void sensorLevel(int *data)
{
  pinMode(LEVELPIN, INPUT);
  int totallevel = 0;
      
  for (int i = 0; i <= 2; i++) {
    delay(10);  
    if (digitalRead(LEVELPIN))
      {
        //Serial.println("Liquid Detected!");
        totallevel+=1;
      }    
  }
  *data = totallevel;
}

void get_firmware_info()
{
  delay(200);
  String firmware_url = "http://" THINGSBOARD_SERVER ":" EDGE_PORT "/api/v1/" + token + "/attributes?sharedKeys=fw_title,fw_version,fw_size";
  //Serial.println(servername);
  http.begin(firmware_url);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0)
  {
    // Serial.print("HTTP Response code: ");
    // Serial.println(httpResponseCode);
    DeserializationError error = deserializeJson(doc, http.getStream());
    // Test if parsing succeeds.
    if (error)
    {
      Serial.print(F("1. deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }

    String fw_title = doc["shared"]["fw_title"];
    String fw_version = doc["shared"]["fw_version"];
    // int fw_size = doc["shared"]["fw_size"];
    Serial.println("Firmware Title : " + fw_title);
    Serial.println("Firmware Version : " + fw_version);
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void send_telemetry(String telemetry)
{
  // post(f"http://{config['host']}:{config['port']}/api/v1/{config['token']}/telemetry",json=telemetry)
  // Serial.println("Goding to send "+telemetry);
  String servername2 = "http://" THINGSBOARD_SERVER ":" EDGE_PORT "/api/v1/" + token + "/telemetry";
  http.begin(servername2);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(telemetry);
  if (httpResponseCode != 200)
  {
    Serial.println("Response not expected " + httpResponseCode);
  }
  //Serial.println("Finished sending " + telemetry);
  http.end();
}

bool get_firmware()
{
  // response = get(f"http{'s' if config['port'] == 443 else ''}://{config['host']}:{config['port']}/api/v1/{config['token']}/firmware", params=params)
  String fw_title = doc["shared"]["fw_title"];
  String fw_version = doc["shared"]["fw_version"];
  // int fw_size = doc["shared"]["fw_size"];
  String url = "http://" THINGSBOARD_SERVER ":" EDGE_PORT "/api/v1/" + token + "/firmware?title=" + fw_title + "&version=" + fw_version + "&size=0&chunk=1";
  Serial.println("Starting firmware update : " + url);
  bool result = esp32FOTA1.forceUpdate(url, false);
  return result;
}

boolean check_firmware()
{
  if ((doc["shared"]["fw_version"] != "" && doc["shared"]["fw_version"] != NULL && doc["shared"]["fw_version"] != firmware_info["current_" FW_VERSION_ATTR]) || (doc["shared"]["fw_title"] != "" && doc["shared"]["fw_title"] != NULL && doc["shared"]["fw_title"] != firmware_info["current_" FW_TITLE_ATTR]))
  {
    // new firmware
    return true;
  }
  else
  {
    return false;
  }
}

void mountFileSystem()
{
  if (!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED))
  {
    Serial.println("LITTLEFS Mount Failed");
    return;
  }
}

void provisionMe()
{
  String deviceName;
  String provisionData;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  bool haveName = readTokenFile(LITTLEFS, "/name.txt", &deviceName);
  if (haveName)
  {
    provisionData = "{\"deviceName\":" + deviceName + ",\"provisionDeviceKey\":" PROVISION_KEY ", \"provisionDeviceSecret\":" PROVISION_SECRET "}";
  }
  else
  {
    provisionData = "{\"provisionDeviceKey\":" PROVISION_KEY ", \"provisionDeviceSecret\":" PROVISION_SECRET "}";
  }
  Serial.print("Connecting to WiFi ..");
  unsigned long currentMillis = millis();
  unsigned long previousMillisInit = currentMillis;
  while (WiFi.status() != WL_CONNECTED)
  {
    currentMillis = millis();
    if (currentMillis - previousMillisInit >= interval)
    {
      Serial.println("Failed to connect.");
      return;
    }
  }
  Serial.println(WiFi.localIP());
  http.begin(provisionUrl);
  int httpResponseCode = http.POST(provisionData);
  //Serial.println(httpResponseCode);
  DeserializationError error = deserializeJson(doc, http.getStream());
  if (error)
  {
    Serial.print(F("1. deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  http.end();
  String status = doc["status"];
  String credentialsType = doc["credentialsType"];
  String credentialsValue = doc["credentialsValue"];
  const char *filePath = "/token.txt";
  Serial.println("Status : " + status);
  Serial.println("Type : " + credentialsType);
  Serial.println("Value : " + credentialsValue);
  if (status == "SUCCESS")
  {
    writeFile(LITTLEFS, filePath, credentialsValue);
    String mac_address = WiFi.macAddress();
    clientAttrUrl = clientAttrUrl + credentialsValue + "/attributes";
    String clientAttr = "{\"mac_address\":"+mac_address+"}"; 
    http.begin(clientAttrUrl);
    http.addHeader("Content-Type","application/json");
    httpResponseCode = http.POST(clientAttr);
    Serial.println(httpResponseCode);
  }
  delay(1000);
  ESP.restart();
}

void handle_firmware() {
  params.checkFirmware = 0;
  String cf_string;
  get_firmware_info();
  serializeJson(firmware_info, cf_string);
  send_telemetry(cf_string);
  if (check_firmware())
      {
        firmware_info[FW_STATE_ATTR] = "DOWNLOADING";
        cf_string = "";
        serializeJson(firmware_info, cf_string);
        send_telemetry(cf_string);
        // Serial.println(millis());
        bool firmware_updated = get_firmware();
        delay(1);
        // Serial.print("Waiting for update complete ");
        while (!esp32FOTA1.isCompleted())
        {
          delay(100);
          Serial.print(".");
        }
        Serial.println("Update Done.");
        delay(1);
        if (firmware_updated)
        {
          // Serial.println("Going to send new info..");
          firmware_info[FW_STATE_ATTR] = "UPDATED";
          firmware_info["current_" FW_TITLE_ATTR] = doc["shared"]["fw_title"];
          firmware_info["current_" FW_VERSION_ATTR] = doc["shared"]["fw_version"];
          cf_string = "";
          serializeJson(firmware_info, cf_string);
          send_telemetry(cf_string);
          delay(1000);
          ESP.restart();
        }
        else
        {
          delay(1);
          Serial.println("xxSomething went wrong during upgrade");
        }
      }
      else
      {
        delay(1000);
        if (doc["shared"]["fw_version"] == NULL || doc["shared"]["fw_title"] == NULL) {
          Serial.println("Firmware data empty");
        } else {
        Serial.println("No new firmware available");
        }
      }

}
void setup()
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  float temperature,humidity;
  int level;
  mountFileSystem();
  bool isprovisioned = readTokenFile(LITTLEFS, "/token.txt", &token);
  if (!isprovisioned)
  {
    // token not exist
    provisionMe();
  }
  else
  {
  
    // token exist
    DEBUG("Token is :");
    //Serial.print("Token is :");
    DEBUGLN(token);
    //Serial.println(token);
    esp_sleep_enable_timer_wakeup(params.deepsleep * uS_TO_S_FACTOR);
    firmware_info["current_" FW_VERSION_ATTR] = FW_VERSION;
    firmware_info["current_" FW_TITLE_ATTR] = FW_TITLE;
    params.checkFirmware += 1;
    delay(1);
    DEBUG("Firmare counter : ");
    //Serial.print("Firmware counter : ");
    DEBUGLN(params.checkFirmware);
    //Serial.println(params.checkFirmware);
    DEBUGLN("Current Firmware : " FW_VERSION);
    //Serial.println("Current Firmware : " FW_VERSION);

    connect_wifi();

    if (params.checkFirmware >= params.fw_cycle)
    {
      handle_firmware();
    }
    sensorDhtData(&temperature,&humidity);
    sensorLevel(&level);
    delay(20);
    //sendSecureDataTcp();
    if (isnan(temperature)) { temperature = 999; };
    if (isnan(humidity)) { humidity = 0;}; 
    sendDataTcp(temperature,humidity,level);
    DEBUG("Temperature :");
    //Serial.print("Temperature : ");
    DEBUGLN(temperature);
    //Serial.println(temperature);
    DEBUG("Humidity : ");
    //Serial.print("Humidity : ");
    DEBUGLN(humidity);
    //Serial.println(humidity);
    DEBUG("Level : ");
    //Serial.print("Level: ");
    DEBUGLN(level);
    //Serial.println(level);
    DEBUG("Deepsleep : ");
    //Serial.print("Deepsleep: ");
    DEBUGLN(params.deepsleep);
    //Serial.println(params.deepsleep);
    digitalWrite(LED, LOW);
    DEBUGLN("Going to sleep ... ");
    //Serial.println("Going to sleep...");
    esp_deep_sleep_start();
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
}