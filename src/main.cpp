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
#include "debug_util.h"
#include "DebugLogArray.h"

const char *host = "LNLD-esp";

/* AP mode setting*/
IPAddress apmode_ip(192, 168, 10, 1);
IPAddress apmode_subnet(255, 255, 255, 0);
const char *apmode_ssid = "ESP_AP";
const char *apmode_password = "password";

ESP8266WebServer server(HTTPD_PORT);
EEPROM_struct eeprom;

// Servo servo;
Ticker flicker;
Ticker co2PollingExecutor;

// volatile char debugLog[DEBUG_LOG_SIZE]={""};
// String debugLog[DEBUG_LOG_SIZE]={""};
// String debugLog;
DebugLogArray debugLog;


//pin assign
// https://qiita-user-contents.imgix.net/https%3A%2F%2Fqiita-image-store.s3.amazonaws.com%2F0%2F55103%2F56eec04e-f231-8f4f-d792-840d36d791d7.png?ixlib=rb-4.0.0&auto=format&gif-q=60&q=75&w=1400&fit=max&s=c463cc4daec76e1a62fd74636fba93f6
const int servo_pin = 12;
const int boot_switch_pin = 13; //+でSTモード，-でAPモード
const int ST_mode_led = 5;
const int AP_mode_led = 4;
const int co2_pwm_in = 14;
//softwareSerial使うとした14,と16くらいしか余ってなさそう

const char readValueCommand=0x86;
const char toggleSelfCalibration=0x79;
const int COMMAND_SIZE=9;//8byte+checksum

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

void pollCo2Value(){
  char readCo2Commands[]={
    0xff,
    0x01,
    readValueCommand,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x79
  };
  Serial.write(readCo2Commands,sizeof readCo2Commands);

}

volatile unsigned int co2ppm=0;

void setup()
{
  /* set up serials */
  Serial.begin(9600); //co2センサとのやり取りに使うため，シリアルデバッグは不可．

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

  /*for debug. check contents of spiffs*/
  // showAllFilesInSPIFFS();

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

  co2PollingExecutor.attach_ms(1000,pollCo2Value);
  // attachInterrupt(digitalPinToInterrupt(co2_pwm_in),pwmInChangeHandler,CHANGE);

  debugLog.add("started");
}

// 取得した値をスペース区切りで表示．
String parseReceivedInfoForDebug(char *receivedRawData){
      char receivedConvertedData[200]={0};
      int curPos=0;
      for(int j=0;j<COMMAND_SIZE;j++){
        int strlen=sprintf(receivedConvertedData+curPos,"%d",receivedRawData[j]);
        // curPos++;
        *(receivedConvertedData+curPos)=' ';
        curPos+=strlen;
      }
      debugLog.add(String(receivedConvertedData));
}

void loop()
{
  ArduinoOTA.handle();
  server.handleClient();

    if(Serial.available()>=COMMAND_SIZE){
      char receivedRawData[COMMAND_SIZE]={0};
      int readSize=Serial.readBytes(receivedRawData,sizeof receivedRawData);

      // debugLog.add(parseReceivedInfoForDebug(receivedRawData));

      if(receivedRawData[1]==readValueCommand){
        co2ppm=(receivedRawData[2]<<8)+receivedRawData[3];
      }
      debugLog.add(String(co2ppm));
    }
}
