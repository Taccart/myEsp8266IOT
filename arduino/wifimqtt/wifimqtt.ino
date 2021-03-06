#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include <float.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <NTPClient.h>            //https://github.com/arduino-libraries/NTPClient
#include <ESP8266WebServer.h>     //https://github.com/esp8266/Arduino
#include <PubSubClient.h>
#include <Thread.h>
#include <Arduino.h>
#include <Wire.h>
#include <SdsDustSensor.h>
#include <BMx280I2C.h>

/*
   HARDWARE SETTINGS
*/

#define IS_LCD                 true
#define IS_LOG_LCD             false

#define IS_LOG_SERIAL          true

#define IS_DEBUG               false

#define SDS_PIN_RX             D4
#define SDS_PIN_TX             D3
#define SDS_COLLECT_INTERVAL   3600000 // 1 hour

#define BMP_I2C_ADDRESS        0x76
#define BMP_MAX_WAIT           1000 // 1000=one second 
#define BMP_COLLECT_INTERVAL   5000 // 5 seconds

#define MQTT_CONN_RETRY_DELAY  1000 // delay to retry mqtt connection.

#define NTP_UPDATE_MS  900000
#define NTP_SERVER "europe.pool.ntp.org"


/*
   SENSORS NAMES KEYS AND UNITS
*/
<<<<<<< HEAD
#define UNIT_MILLISECONDS "second"
#define UNIT_PM           "μg_per_m³"
#define UNIT_PASCAL       "Pascal"
#define UNIT_CELCIUS      "Celcius"

#define KEY_UPTIME        "uptime"
#define KEY_PM25          "pm2.5"
#define KEY_PM10          "pm10"
#define KEY_PRESS         "press"
#define KEY_TEMP          "temp"

#define SENSOR_SDS        "sds011"
#define SENSOR_BMP280     "bmp280"
#define SENSOR_ESP        "esp"

#define AP_MODE_SSID      "thing-setup"
#define AP_MODE_PASSWORD  "thing-setup"

#define CONFIGURATION_FILE "/config.json"
=======
#define UNIT_MILLISECONDS "s"
#define UNIT_PM "μg_per_m³"
#define UNIT_PASCAL "Pa"
#define UNIT_CELCIUS "C"
#define KEY_UPTIME "uptime"
#define KEY_PM25 "pm2.5"
#define KEY_PM10 "pm10"
#define KEY_PASCAL "press"
#define KEY_CELCIUS "temp"
#define SENSOR_SDS "sds011"
#define SENSOR_BMP280 "bmp280"
#define SENSOR_ESP "esp"
/*
   WIFI
*/
#define AP_MODE_SSID "thing-setup"
#define AP_MODE_PASSWORD "thing-setup"
>>>>>>> 874380c71384ff1c8d6d9f388559caf6328249a6

#define WEB_SERVER_PORT 8888
#define CONTENT_TYPE_JSON "application/json"
#define CONTENT_TYPE_HTML "text/html"
#define CONTENT_TYPE_TEXT "text/plain"

//WebServer exposes REST services and a minimalist UI.
ESP8266WebServer server(WEB_SERVER_PORT );

#if IS_DEBUG
#define DEBUG_NTPClient true
#endif



#if IS_LCD
#include <LiquidCrystal_I2C.h>
#define LCD_I2C_ADDRESS        0x27
#define LCD_UPDATE_INTERVAL    5000 // 5 seconds
#define LCD_LIGHTOFF_INTERVAL  10000 //  10 seconds
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display
Thread lightOffThread = Thread();
Thread lcdUpdateThread = Thread();
#endif







//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_client[16] = "ESP";
char mqtt_server[40] =  "192.168.0.30";
char mqtt_port[6] = "1883";
char mqtt_user[128] = "";
char mqtt_password[128] = "";
char mqtt_sleep[6] = "6000";
//char collect_sleep[6] = "6000";
char mqtt_pub_prefix[32] = "metrics";
char mqtt_sub_prefix[32] = "commands";
char board_id[32] = SENSOR_ESP;
long unsigned startedEpoch = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

//ntp server for time syncronization.
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER , 0, NTP_UPDATE_MS);


// Extra parameters
WiFiManagerParameter custom_mqtt_server("mqtt_server", "mqtt server", mqtt_server, 40 );
WiFiManagerParameter custom_mqtt_port("mqtt_port", "mqtt port", (char*) mqtt_port, 6 );
WiFiManagerParameter custom_mqtt_user("mqtt_user", "mqtt user", mqtt_user, 128 );
WiFiManagerParameter custom_mqtt_password("mqtt password", "mqtt_password", mqtt_password, 128 );
WiFiManagerParameter custom_mqtt_sleep("mqtt sleep", "mqtt_sleep", (char*) mqtt_sleep, 6 );
//WiFiManagerParameter custom_collect_sleep("collect sleep", "collect_sleep", (char*) collect_sleep, 6 );
WiFiManagerParameter custom_mqtt_pub_prefix("mqtt publish prefix", "mqtt_pub_prefix", mqtt_pub_prefix, 32 );
WiFiManagerParameter custom_mqtt_sub_prefix("mqtt subscribe prefix", "mqtt_sub_prefix", mqtt_sub_prefix, 32 );

// wifi events
WiFiEventHandler stationModeConnectedHandler ;
WiFiEventHandler stationModeDisconnectedHandler ;
WiFiEventHandler stationModeAuthModeChangedHandler ;
WiFiEventHandler stationModeGotIPHandler ;
WiFiEventHandler softAPModeStationConnectedHandler ;
WiFiEventHandler softAPModeStationDisconnectedHandler ;


//used for REST /status
DynamicJsonBuffer jsonStatusBuffer;
DynamicJsonBuffer jsonMetricsBuffer;
JsonObject& jsonStatus = jsonStatusBuffer.createObject();
JsonObject& jsonMetrics = jsonMetricsBuffer.createObject();



// SENSORS AND EXTENSIONS
// Air Sensor SDS011 for pm2.5 and pm10
SdsDustSensor sds(SDS_PIN_RX, SDS_PIN_TX);

// Temperature and Pressure
BMx280I2C bmp280(BMP_I2C_ADDRESS);

// Dust sensor has a short life of 8000 hours: we will collect values on a low frequency.
Thread collectSDSThread = Thread();
Thread collectBMPThread = Thread();

//publish thread will push last values of sensors to  mqtt server
Thread publishThread = Thread();



//flag for saving data
bool shouldSaveConfig = true;





/* ==========  WEB SERVER  ==========
*/

void startWebServer() {
#if IS_DEBUG
  log("Web server", "preparing");
#endif
  server.on("/status", handleStatus);
  server.on("/metrics", handleMetrics);
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  jsonStatus["webserverStart"] = timeClient.getEpochTime();
  log("Web server", "Started.");
}

bool loadFromFS(String path) {
  String dataType = "text/plain";
  if (path.endsWith(".html")) {
    dataType = "text/html";
  } else if (path.endsWith(".css")) {
    dataType = "text/css";
  } else if (path.endsWith(".js")) {
    dataType = "application/javascript";
  }
  File dataFile = SPIFFS.open(path.c_str(), "r" );

  if (!dataFile) {
    server.send(404, CONTENT_TYPE_TEXT, "File not found.");
    return false;
  }

  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    Serial.printf("webserver sent less data than expected for %s", path.c_str());
  }

  dataFile.close();
  return true;
}
void handleRoot() {
  char response[512] = "";
  sprintf(response, "<html><body><p>See:<ul><li><a href=\"/metrics\">/metrics</a><li><a href=\"/status\">/status</a></ul></p><p>epoch (s):<glow>%lu</glow></p></body></html>", timeClient.getEpochTime());
  strncat(response, "", sizeof(response));
  server.send(200, CONTENT_TYPE_HTML, response);
}

void handleStatus() {
  jsonStatus[KEY_UPTIME] = upTime();
  jsonStatus["espHeapFragmentation"] = ESP.getHeapFragmentation();
  jsonStatus["espFreeHeap"] = ESP.getFreeHeap();
  jsonStatus["espResetReason"] = ESP.getResetReason();
  String output;
  jsonStatus.printTo(output);
  server.send(200, CONTENT_TYPE_JSON, output);
}

void handleMetrics() {
#if IS_DEBUG
  jsonStatus["millis"] = millis();
#endif
  String output;
  jsonMetrics.printTo(output);
  server.send(200, CONTENT_TYPE_JSON, output);
}
void handleNotFound() {
  loadFromFS(server.uri());
}

/*==========  MQTT - PUBLISH  ==========
*/

void startPubSub() {
#if IS_DEBUG
  log("MQTT pub/sub", "preparing");
#endif
  jsonStatus["mqttSubTotal"] = 0;
  jsonStatus["mqttPubTotal"] = 0;
  jsonStatus["mqttStart"] = timeClient.getEpochTime();
  mqttClient.setServer(mqtt_server,  str_to_uint16(mqtt_port));
  mqttClient.setCallback(consumeCallback);
  int tentatives = 0;
  while (! (mqttClient.connected() ||  tentatives > 10) ) {
    log("MQTT pub/sub", "Connecting");

    if (mqttClient.connect(board_id, mqtt_user, mqtt_password )) {
      log("MQTT pub/sub", "Connected.");
    } else {
      log("MQTT pub/sub", "Connect FAILED.");
      jsonStatus["mqttConnectionTentatives"] = tentatives;
      tentatives++;
      Serial.println(tentatives);
      delay(MQTT_CONN_RETRY_DELAY);
    }

  }
  log(mqtt_pub_prefix, mqtt_sub_prefix);
  mqttClient.subscribe(mqtt_sub_prefix);
  publishThread.setInterval(str_to_uint16(mqtt_sleep));
  publishThread.onRun(publishCallback);

}
int pub(char* key, float value) {
  char v[128];
  snprintf(v, sizeof v, "%g", value); // assume 128 is sufficient to store

  return pub(key, v);
}
int pub(char* key, int value) {
  char v[128];
  snprintf(v, sizeof v, "%i", value); // assume 128 is sufficient to store

  return pub(key, v);
}
int pub(char* key, long unsigned int value) {
  char v[128];
  snprintf(v, sizeof v, "%lu", value); // assume 128 is sufficient to store

  pub(key, v);
}
int pub(char* key, char* value) {

  return mqttClient.publish(key, value);
}
// publish callback (for scheduler)
void publishCallback() {
  
#if IS_DEBUG
  log("MQTT: Publishing ");
  jsonStatus["mqttPubLastStart"] = timeClient.getEpochTime();
#endif

  // JsonObject has unfortunately no list of keys: we need to search for know ones.
  char k[384] ;
  genKey(board_id , "board", KEY_UPTIME, UNIT_MILLISECONDS, k);
  pub ( k, upTime());

  genKey(board_id , SENSOR_SDS, KEY_PM25, UNIT_PM, k);
  if (jsonMetrics.containsKey(k))
    pub (k, jsonMetrics[k].as<float>());
#if IS_DEBUG
  else
    pub (k, 0);
#endif

  genKey(board_id , SENSOR_SDS, KEY_PM10, UNIT_PM, k);
  if (jsonMetrics.containsKey(k))
    pub (k, jsonMetrics[k].as<float>());
#if IS_DEBUG
  else
    pub (k, 0);
#endif

  genKey(board_id , SENSOR_BMP280, KEY_PRESS, UNIT_PASCAL, k);
  if (jsonMetrics.containsKey(k))
    pub (k, jsonMetrics[k].as<float>());
#if IS_DEBUG
  else
    pub (k, 0);
#endif

  genKey(board_id , SENSOR_BMP280, KEY_TEMP, UNIT_CELCIUS, k);
  if (jsonMetrics.containsKey(k))
    pub (k, jsonMetrics[k].as<float>());
#if IS_DEBUG
  else
    pub (k, 0);
#endif

#if IS_DEBUG
  jsonStatus["mqttPubLastEnd"] = timeClient.getEpochTime();
#endif
}




/*==========  MQTT - SUBSCRIBE  ==========
*/

void consumeCallback(char* topic, byte* payload, unsigned int length) {
  char* cpayload = reinterpret_cast<char*>(payload);

#if IS_DEBUG
  jsonStatus["mqttLastSubStart"] = timeClient.getEpochTime();
  log("SUB" , cpayload);
#endif


#if IS_LCD
  if (cpayload == "lcd/print test") {
    lcd.print("test");
    log("LCD", "test");
  }
  else if (cpayload == "lcd/cursor 0") {
    lcd.setCursor(0, 0);
    log("LCD", "0,0");
  }
  else if (cpayload == "lcd/cursor 1") {
    lcd.setCursor(0, 1);
    log("LCD", "0,1");
  }
  else if (cpayload == "lcd/noBacklight") {
    lcd.noBacklight();
    log("LCD", "no backlight");
  }
  else if (cpayload == "lcd/backlight") {
    lcd.backlight();
    log("LCD", "backlight");
  }
#endif

  jsonStatus["mqttSubTotal"] = jsonStatus["mqttSubTotal"].as<long>() + 1;

#if IS_DEBUG
  jsonStatus["mqttLastSubEnd"] = timeClient.getEpochTime();
#endif
}


/* ==========  SENSORS  ==========
*/
long unsigned upTime() {
  return  (timeClient.getEpochTime() - startedEpoch);
}

String getMetric(char* board, char* sensor, char* key, char* unit){
  char k[384] ;
  genKey(board, sensor, key, unit, k);
  return (jsonMetrics[k]) ;
}


void setMetric(char* board, char* sensor, char* key, char* unit, float value) {
  char k[384] ;
  genKey(board, sensor, key, unit, k);
  char v[128];
  snprintf(v, sizeof v, "%g", value); // assume 128 is sufficient to store
  jsonMetrics[k] = v;
  //  free(v);
  //  free(k);
}
void setMetric(char* board, char* sensor, char* key, char* unit, long unsigned value) {
  char k[384] ;
  genKey(board, sensor, key, unit, k);
  jsonMetrics[k] = value;
}
void collectBMPCallback() {
  // uptime
  setMetric(board_id, "board", KEY_UPTIME, UNIT_MILLISECONDS,   upTime());
  //BMP280

  if (!bmp280.measure())
  {
    jsonStatus["bmpStatus"] = "Can't get measure.";
    setMetric(board_id, SENSOR_BMP280, KEY_PRESS, UNIT_PASCAL, FLT_MIN);
    setMetric(board_id, SENSOR_BMP280, KEY_TEMP, UNIT_CELCIUS, FLT_MIN);
  }
  else
  { int start = millis();
    do {
      delay(100);
    } while (!(bmp280.hasValue() || millis() - start < BMP_MAX_WAIT));

    if (bmp280.hasValue()) {
      setMetric(board_id, SENSOR_BMP280, KEY_PRESS, UNIT_PASCAL, bmp280.getPressure());
      setMetric(board_id, SENSOR_BMP280, KEY_TEMP, UNIT_CELCIUS, bmp280.getTemperature());
    }
#if IS_DEBUG
    else {
      setMetric(board_id, SENSOR_BMP280, KEY_PRESS, UNIT_PASCAL, FLT_MIN);
      setMetric(board_id, SENSOR_BMP280, KEY_TEMP, UNIT_CELCIUS, FLT_MIN);
    }
#endif
  }



  
}
void collectSDSCallback() {
  if (startedEpoch == 0 && timeClient.getEpochTime() > 1582165220 )
  {
    startedEpoch = timeClient.getEpochTime() ;
  }
  // uptime
  setMetric(board_id, "board", KEY_UPTIME, UNIT_MILLISECONDS,   upTime());



  //SDS011
#if IS_DEBUG
  info(SENSOR_SDS, "Waking up.");
  jsonStatus["sdsStartLast"] = timeClient.getEpochTime();
#endif
  jsonStatus["sdsStartTotal"] = jsonStatus["sds_start_total"].as<long>() + 1;
  sds.wakeup();
  delay(1000); // delay should never be placed in a callback.

  PmResult pm = sds.queryPm();

  if (pm.isOk()) {
    //log( pm.statusToString() );
    jsonStatus["sdsLastQueryPM"] = pm.statusToString();
    setMetric(board_id, SENSOR_SDS, KEY_PM25, UNIT_PM, pm.pm25);
    setMetric(board_id, SENSOR_SDS, KEY_PM10, UNIT_PM, pm.pm10);
  }
  else {
    log("SDS011", "FAILED");
    jsonStatus["sdsLastQueryPM"] = "FAILED";
#if IS_DEBUG
    setMetric(board_id, SENSOR_SDS, KEY_PM25, UNIT_PM, FLT_MIN);
    setMetric(board_id, SENSOR_SDS, KEY_PM10, UNIT_PM, FLT_MIN);
#endif

    log(SENSOR_SDS, "Not OK.");
  }

  WorkingStateResult state = sds.sleep();
  if (state.isWorking()) {
    jsonStatus["sdsLastStatus"] = "Can't sleep";

    log(SENSOR_SDS, "Can't sleep.");

  } else {
    jsonStatus["sdsLastStatus"] = "Sleeping";
#if IS_DEBUG
    log(SENSOR_SDS, "Sleeping");
#endif
  }
#if IS_DEBUG
  jsonStatus["sdsEndLast"] = timeClient.getEpochTime();
#endif
}

void initBMP280() {
  Wire.begin();
  long start = millis();
  if (!bmp280.begin())
  {
    jsonStatus["bmpStatus"] = "Init failed - check Interface and I2C Address.";
    delay(10);
    while (millis() - start < BMP_MAX_WAIT);
  }
  bmp280.resetToDefaults();
  bmp280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
  bmp280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);


}
void startCollect() {
#if IS_DEBUG
  log("Init sensor", "SDS");
#endif
  sds.begin();
  WorkingStateResult state = sds.sleep();
  if (state.isWorking()) {
    jsonStatus["sdsLastStatus"] = "Can't sleep";

    log(SENSOR_SDS, "Can't sleep.");

  }
#if IS_DEBUG
  log("Init sensor", "BPM280");
#endif
  initBMP280();
  collectBMPThread.setInterval(BMP_COLLECT_INTERVAL);
  collectBMPThread.onRun(collectBMPCallback);
  collectSDSThread.setInterval(SDS_COLLECT_INTERVAL);
  collectSDSThread.onRun(collectSDSCallback);
  jsonStatus["collectStart"] = timeClient.getEpochTime();
}



/* ==========  TOOLING  ==========
*/
void genKey (  char* board, char* sensor, char* metric, char* unit, char k[384] ) {
  strcpy(k, mqtt_pub_prefix);
  strcat(k, "/board/");
  strcat(k, board);
  strcat(k, "/sensor/");
  strcat(k, sensor);
  strcat(k, "/");
  strcat(k, metric);
  strcat(k, "_");
  strcat(k, unit);



}
// defaults to 65535 if error
static uint16_t str_to_uint16(const char *str) {
  char *end;
  errno = 0;
  long  val = strtol(str, &end, 10);
  if (errno == ERANGE || val < 0 || val > UINT16_MAX || end == str || *end != '\0')
    return  (uint16_t)  65535;
  return (uint16_t) val;

}
// logs lines to Serial, and/or to LCD16x02
void log(const char* line ) {
#if IS_LOG_SERIAL
  Serial.println(line);
#endif

#if IS_LOG_LCD && IS_LCD
  lcd.setCursor(0, 0);
  lcd.print(line);
#endif
}
void log(const char* line1, const char* line2) {
#if IS_LOG_SERIAL
  Serial.println(line1);
  Serial.println(line2);
#endif

#if IS_LOG_LCD && IS_LCD
  lcd.setCursor(0, 0);
  lcd.print(line1)
  lcd.setCursor(0, 2);
  lcd.print(line2);
#endif

}
void info(String l) {
#if IS_LOG_SERIAL
  Serial.println(l);
#endif
#if IS_LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(l);
#endif
}
void info (String l1, String l2) {
#if IS_LOG_SERIAL
  Serial.print(l1);
  Serial.print("\n");
  Serial.println(l2);
#endif

#if IS_LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
#endif


}


/* ==========  WIFI AND CONFIGURATION  ==========
*/
//callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig = true;
}
void onStationModeConnected          (const WiFiEventStationModeConnected& evt)          {
  jsonStatus["wifiSSID"] = evt.ssid;
  jsonStatus["wifiStatus"] = "Connected";

  info("Wifi", "Connected");

}
void onStationModeDisconnected       (const WiFiEventStationModeDisconnected& evt)       {
  jsonStatus["wifiSSID"] = evt.ssid;
  jsonStatus["wifiStatus"] = "Disonnected";

  info("Wifi", "Disconnected");

}

void onStationModeGotIP              (const WiFiEventStationModeGotIP& evt)              {
  jsonStatus["wifiIP"] = evt.ip.toString();
  info("IP", evt.ip.toString().c_str());
  startNTP();
}
/*
  void onStationModeAuthModeChanged    (const WiFiEventStationModeAuthModeChanged& evt)    {
  log("WifiEvent: StationModeAuthModeChanged");
  }
  void onSoftAPModeStationConnected    (const WiFiEventSoftAPModeStationConnected& evt)    {
  log("WifiEvent: SoftAPModeStationConnected");
  }
  void onSoftAPModeStationDisconnected (const WiFiEventSoftAPModeStationDisconnected& evt) {
  log("WifiEvent: SoftAPModeStationDisconnected");
  }
*/
void setWifiEventHandlers() {
  stationModeConnectedHandler = WiFi.onStationModeConnected( &onStationModeConnected);
  stationModeDisconnectedHandler = WiFi.onStationModeDisconnected( &onStationModeDisconnected);
  stationModeGotIPHandler = WiFi.onStationModeGotIP( &onStationModeGotIP);
  //  stationModeAuthModeChangedHandler = WiFi.onStationModeAuthModeChanged( &onStationModeAuthModeChanged);
  //  softAPModeStationConnectedHandler = WiFi.onSoftAPModeStationConnected( &onSoftAPModeStationConnected);
  //  softAPModeStationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected( &onSoftAPModeStationDisconnected);
}
void wifiSetup(bool isReset) {
  //Local intialization. Once its business is done, there is no need to keep it around

  WiFiManager wifiManager;
#if IS_DEBUG
  wifiManager.setDebugOutput(true);
#else
  wifiManager.setDebugOutput(false);
#endif
  if (isReset) {
    info(AP_MODE_SSID, AP_MODE_PASSWORD);
    wifiManager.resetSettings();
  }
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);



  //Custom parameters
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_sleep);
//  wifiManager.addParameter(&custom_collect_sleep);
  wifiManager.addParameter(&custom_mqtt_pub_prefix);
  wifiManager.addParameter(&custom_mqtt_sub_prefix);

  //reset settings - for testing
  //  wifiManager.resetSettings();


  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  info("Mode", "Autoconnect");

  if (!wifiManager.autoConnect(AP_MODE_SSID, AP_MODE_PASSWORD)) {
    info("Autoconnect", "FAILED");

    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
#if IS_DEBUG
     info("Autoconnect", "Succeeded");

#endif
  //read updated parameters
  strcpy( mqtt_server, custom_mqtt_server.getValue());
  strcpy( mqtt_port, custom_mqtt_port.getValue());
  strcpy( mqtt_user, custom_mqtt_user.getValue());
  strcpy( mqtt_password, custom_mqtt_password.getValue());
  strcpy( mqtt_sleep, custom_mqtt_sleep.getValue());
//  strcpy( collect_sleep, custom_collect_sleep.getValue());
  strcpy( mqtt_pub_prefix, custom_mqtt_pub_prefix.getValue());
  strcpy( mqtt_sub_prefix, custom_mqtt_sub_prefix.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_password"] = mqtt_password;
    json["mqtt_sleep"] = mqtt_sleep;
//    json["collect_sleep"] = collect_sleep;
    json["mqtt_pub_prefix"] = mqtt_pub_prefix;
    json["mqtt_sub_prefix"] = mqtt_sub_prefix;

#if IS_DEBUG
    json.printTo(Serial);
#endif

    File configFile = SPIFFS.open(CONFIGURATION_FILE, "w");
    if (!configFile) {
      jsonStatus["readConfigStatus"] = "Failed to save config.";
    }
    else {
      json.printTo(configFile);
    }
    configFile.close();
    jsonStatus["readConfigStatus"] = "File closed.";

  }

  jsonStatus["mqttConnServer"] = mqtt_server;
  jsonStatus["mqttConnPort"] = mqtt_port;
  jsonStatus["mqttPubPrefix"] = mqtt_pub_prefix;
  jsonStatus["mqttPubSleep"] = mqtt_sleep;
  jsonStatus["mqttSubPrefix"] = mqtt_sub_prefix;
//  jsonStatus["sensorCollectSleep"] = collect_sleep;

}

void readConf() {

  //read configuration from
  if (SPIFFS.begin()) {
    jsonStatus["readConfigStatus"] = "File sytem started";
#if IS_DEBUG
    log("FS mounted.");
#endif
    if (SPIFFS.exists(CONFIGURATION_FILE)) {
      File configFile = SPIFFS.open(CONFIGURATION_FILE, "r");
      if (configFile) {
#if IS_DEBUG
        log("Read config.");
#endif

        jsonStatus["readConfigStatus"] = "File opened";

        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
#if IS_DEBUG
        json.printTo(Serial);
        log("Read JSON.");
#endif
        if (json.success()) {

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_password, json["mqtt_password"]);
          strcpy(mqtt_sleep, json["mqtt_sleep"]);
//          strcpy(collect_sleep, json["collect_sleep"]);
          strcpy(mqtt_pub_prefix, json["mqtt_pub_prefix"]);
          strcpy(mqtt_sub_prefix, json["mqtt_sub_prefix"]);

        } else {
          jsonStatus["readConfigStatus"] = "Failed to read json.";
          log("Read Config", "FAILED" );
        }
        configFile.close();
      }
    }
    else {
      jsonStatus["readConfigStatus"] = "Config file not found.";
    }
  } else {
    jsonStatus["readConfigStatus"] = "FS mount failed.";
#if IS_DEBUG
    log("FS mount failed.");
#endif
  }
}

void startAPMode() {

#if IS_LCD
  lcd.backlight();
  info(AP_MODE_SSID, AP_MODE_PASSWORD);
#endif
  wifiSetup(true);
#if IS_LCD
  lcd.noBacklight();
#endif
}

void startNTP() {
  NTPClient timeClient(ntpUDP);
}
/* ==========  ARDUINO - SETUP  ==========
*/
void updateBoardInfos() {
  // board ID enrichet with chipID
  char chipid[10] = "";
  sprintf(chipid, "_%08X", ESP.getChipId());
  strncat( board_id, chipid, sizeof(board_id));

  // board infos added to /status
  jsonStatus["boardId"] = board_id;
  jsonStatus["espChipId"] = ESP.getChipId();
  jsonStatus["espFreeSketchSpace"] = ESP.getFreeSketchSpace();
  jsonStatus["espSketchSize"] = ESP.getSketchSize();
  jsonStatus["espCpuFreqMHz"] = ESP.getCpuFreqMHz();
  jsonStatus["espSdkVersion"] = ESP.getSdkVersion();
  jsonStatus["espCoreVersion"] = ESP.getCoreVersion();
  jsonStatus["espMaxFreeBlockSize"] = ESP.getMaxFreeBlockSize();


}

#if IS_LCD
// switch off lcd light regularly.
void lightOffCallback() {
  lcd.noBacklight();
}

void lcdUpdateCallback() {
  lcd.backlight();
  float p=getMetric(board_id, SENSOR_BMP280, KEY_PRESS, UNIT_PASCAL).toFloat()/100;
  float t=getMetric(board_id, SENSOR_BMP280, KEY_TEMP, UNIT_CELCIUS).toFloat();
  float pm2=getMetric(board_id, SENSOR_SDS, KEY_PM25, UNIT_PM).toFloat();
  float pm10=getMetric(board_id, SENSOR_SDS, KEY_PM10, UNIT_PM).toFloat();

  char line1[20];
  char line2[20];

  sprintf(line1,"%5.1f" "\xDF" "C %3.0f p2.5 ",t,pm2);
  sprintf(line2,"%4.0fhPa %3.0f p10  ",p,pm10);

  info(line1,line2);
}

void startLCD() {
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lightOffThread.onRun(lightOffCallback);
  lightOffThread.setInterval(LCD_LIGHTOFF_INTERVAL);
  lcdUpdateThread.onRun(lcdUpdateCallback);
  lcdUpdateThread.setInterval(LCD_UPDATE_INTERVAL);
}
#endif


// Setup mainly deals with initialization of Wifi connection and setup of scheduled action.
void setup() {
  Serial.begin(115200);
#if IS_LCD
  startLCD();
#endif
  updateBoardInfos();
  setWifiEventHandlers();
  readConf();
  collectSDSThread.run();
  collectBMPThread.run();
  lcdUpdateThread.run();
  wifiSetup(false);
  startWebServer();
  startCollect();

  info("Running");
  
  startPubSub();
}

void loop() {
#if IS_LCD
 // if (lightOffThread.shouldRun()) lightOffThread.run();
  if (lcdUpdateThread.shouldRun()) lcdUpdateThread.run();
#endif

  if (publishThread.shouldRun())  publishThread.run();
  if (collectSDSThread.shouldRun()) collectSDSThread.run();
  if (collectBMPThread.shouldRun()) collectBMPThread.run();
  mqttClient.loop();
  timeClient.update();
  server.handleClient();
}
