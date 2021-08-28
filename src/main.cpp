#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include "httpd.h"
#include "eeprom_util.h"
#include <EEPROM.h>
#include <Servo.h>
#include "properties.h"
#include <Ticker.h>

const char *host = "LNLD-esp";

/* AP mode setting*/
IPAddress apmode_ip(192, 168, 10, 1);
IPAddress apmode_subnet(255, 255, 255, 0);
const char *apmode_ssid = "ESP_AP";
const char *apmode_password = "password";

ESP8266WebServer server(HTTPD_PORT);
EEPROM_struct eeprom;

Servo servo;
Ticker flicker;

const int servo_pin = 12;
const int boot_switch_pin = 13; //+でSTモード，-でAPモード
const int ST_mode_led = 5;
const int AP_mode_led = 4;

void brinkForError()
{
  digitalWrite(ST_mode_led, LOW);
  digitalWrite(AP_mode_led, LOW);
  for (int i = 0; i < 5; i++)
  {
    digitalWrite(AP_mode_led, HIGH);
    delay(300);
    digitalWrite(AP_mode_led, LOW);
    delay(300);
  }
}

void flipST_mode_led()
{
  digitalWrite(ST_mode_led, !digitalRead(ST_mode_led));
}

void setup()
{
  /* set up serials */
  Serial.begin(74880); //速度変更すると不調のため，espのデフォルト速度を指定．
  Serial.println("");
  Serial.println("Booting");

  /* set up digital io */
  pinMode(ST_mode_led, OUTPUT);
  pinMode(AP_mode_led, OUTPUT);
  pinMode(boot_switch_pin, INPUT_PULLUP);
  // first, all leds are ON
  digitalWrite(ST_mode_led, HIGH);
  digitalWrite(AP_mode_led, HIGH);

  /* setup the OTA server */
  ArduinoOTA.setHostname(host);
  ArduinoOTA.onError([](ota_error_t error) {
    brinkForError();
    ESP.restart();
  });
  ArduinoOTA.begin();

  /*set up SPIFFS*/
  SPIFFS.begin();

  /*set up eeprom*/
  EEPROM.begin(sizeof(eeprom));

  // EEPROMに保存されたSSID/PASSWORDを読み込む
  getEEPROM();

  if (digitalRead(boot_switch_pin) == HIGH)
  { //Behave as ST
    // set indicator
    digitalWrite(AP_mode_led, LOW);
    flicker.attach(1, flipST_mode_led);

    //set ssid/password
    const char *ssid = eeprom.connectionInfo.ssid;
    const char *password = eeprom.connectionInfo.password;

    // set up WiFi station.
    WiFi.mode(WIFI_STA);
    WiFi.hostname(host);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
      WiFi.begin(ssid, password);
      Serial.println("Retrying connection...");
    }

    // start mDNS to resolve myself by host name.
    MDNS.begin(host);

    // reset indicator
    flicker.detach();
    digitalWrite(ST_mode_led, HIGH);
  }
  else
  { //Behave as AP
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apmode_ip, IPAddress(0, 0, 0, 0), apmode_subnet);
    WiFi.softAP(apmode_ssid, apmode_password);

    /* set indicator */
    digitalWrite(ST_mode_led, LOW);
    digitalWrite(AP_mode_led, HIGH);
  }

  /* set up HTTP Server */
  setupHttpd();

  /* set up servos */
  servo.attach(servo_pin);
  servo.write(SERVO_DEFAULT);

  Serial.println("Ready");
}

void loop()
{
  ArduinoOTA.handle();
  server.handleClient();
}
