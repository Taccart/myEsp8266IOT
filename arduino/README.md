# Arduino IDE #
You have to configure your arduino IDE with additional libs and plugins:
## plugin ##
 * [Arduino ESP8266 filesystem uploader](https://github.com/esp8266/arduino-esp8266fs-plugin
)
## libraries ##
* [ArduinoJson](https://github.com/bblanchon/ArduinoJson) to deal data stored as JSON objects.
* [ESP8266WiFi](https://github.com/esp8266/Arduino) contains all libraries for ESP8266 (wifi client, webserver, ...)
* [WiFiManager](https://github.com/tzapu/WiFiManager) is the core library for online configuration of wifi and settings.
* [NTPClient](https://github.com/arduino-libraries/NTPClient) to have a synchronized clock.

# Custom sensors #
I'm using a BMP280 sensor and a SDS011 air quality sensor in my project:
## libraries ##

* [SdsDustSensor](https://github.com/lewapek/sds-dust-sensors-arduino-library) for SDS011 dust sensor.
* [BMx280I2C](https://bitbucket.org/christandlg/bmx280mi) for BMP280 temperature and pressure sensor.
