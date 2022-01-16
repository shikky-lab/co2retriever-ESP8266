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
#include "common.h"

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

//pin assign
// https://qiita-user-contents.imgix.net/https%3A%2F%2Fqiita-image-store.s3.amazonaws.com%2F0%2F55103%2F56eec04e-f231-8f4f-d792-840d36d791d7.png?ixlib=rb-4.0.0&auto=format&gif-q=60&q=75&w=1400&fit=max&s=c463cc4daec76e1a62fd74636fba93f6

const int servo_pin = 12;
const int boot_switch_pin = 13; //+でSTモード，-でAPモード
const int ST_mode_led = 5;
const int AP_mode_led = 4;
const int co2_pwm_in = 14;

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

volatile unsigned int co2ppm=0;
IRAM_ATTR void pwmInChangeHandler() {
  static unsigned long previousHighTiming = 0;
  static unsigned long ellapsedTime;
  if(digitalRead(co2_pwm_in)){//when rising
    unsigned long currentTIme = millis();
    unsigned long T = currentTIme - previousHighTiming;

    /* caluclate co2 ppm. It needs the time between previous high and next high. So caluclation should be done when the signal rise up(after 2nd times).*/
    if(previousHighTiming!=0){
      co2ppm=2000*(ellapsedTime-2)/(T-4);
      Serial.println(co2ppm);
    }

    previousHighTiming=currentTIme;
  }else{//when falling
    ellapsedTime = millis() - previousHighTiming;
  }
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
  pinMode(co2_pwm_in, INPUT_PULLUP);
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

  attachInterrupt(digitalPinToInterrupt(co2_pwm_in),pwmInChangeHandler,CHANGE);
  Serial.println("Ready");
}


void loop()
{
  ArduinoOTA.handle();
  server.handleClient();
}
