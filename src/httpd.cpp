#include <FS.h>
#include <Servo.h>
#include "httpd.h"
#include "eeprom_util.h"
#include "properties.h"

extern ESP8266WebServer server;
extern EEPROM_struct eeprom;

extern Servo servo;
void moveServo(void){
    servo.write(SERVO_ON);
    delay(500);
    servo.write(SERVO_DEFAULT);
    server.send(200, F("text/plain"), "succeeded!");
}

/**
 * HTTPDの初期設定
 */
void setupHttpd(void) {
  
  server.on("/", setupHttpdRoot);

  server.on("/index.html", setupHttpdRoot);

  // http://192.168.5.1/eeprom アクセス時に setupHttpdEeprom() を実行
  server.on("/eeprom", setupHttpdEeprom);

  server.on("/servo-on", moveServo);

  // 上記以外アクセス時のハンドラ指定
  server.onNotFound(handleConfirmFile);

  // Webサーバ起動
  server.begin();
  
  Serial.println("HTTP server started");
}
/**
 * ルートディレクトリアクセス時のハンドラ
 */
void setupHttpdRoot(void) {
  
  // 「設定する」ボタン押下時
  if(server.hasArg("connectionInfoSet")) {

    // トップページで選択した性別を取得
    if(server.hasArg("connectionInfo-ssid")) {
      strncpy( eeprom.connectionInfo.ssid, server.arg("connectionInfo-ssid").c_str(), EEPROM_SSID_SIZE);
    }

    // トップページで入力した名前を取得
    if(server.hasArg("connectionInfo-password")) {
      strncpy( eeprom.connectionInfo.password, server.arg("connectionInfo-password").c_str(), EEPROM_PASS_SIZE);
    }

    // EEPROMに保存
    setEEPROM();

  }

  // /index.html を読み込みブラウザ側に返す
  handleFileRead(INDEX_HTML_FILE);
}

/**
 *  EEPROM取得Ajaxのハンドラ
 */
void setupHttpdEeprom(void) {
  
  // JSON形式の性別・名前をブラウザに返します。
  server.send(200, "application/json", getEepromJson());
  
}

/**
 * 未登録のURI（css,jsファイルも含める)アクセス時のハンドラ
 * Fileの存在確認を行う
 */
void handleConfirmFile(void) {
  
  // URIを取得
  String path = server.uri();
  
  // SPIFFSにファイルが存在しているか確認
  if (SPIFFS.exists(path)) {

    // css/site.css、js/jquery.min.js、js/eeprom.js を読み込みブラウザ側に返す
    handleFileRead(path);

  } else {

    // File Not Found を返す
    String message = F("File Not Found\n\n");
    message += F("URI: ");
    message += server.uri();
    message += F("\nMethod: ");
    message += (server.method() == HTTP_GET)?F("GET"):F("POST");
    message += F("\nArguments: ");
    message += server.args();
    message += F("\n");

    for (uint8_t i=0; i<server.args(); i++){
      message += " " + server.argName(i) + F(": ") + server.arg(i) + F("\n");
    }

    server.send(404, F("text/plain"), message);

  }

}

/**
 * mime type 取得
 */
String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

/**
 * SPIFFSからファイルを返す
 */
void handleFileRead(String path) {
  
  // ファイルのMIMEタイプを取得
  String contentType = getContentType(path);

  // SPIFFSにファイルが存在するか確認
  if(SPIFFS.exists(path)){

    // ファイルを読み込む
    File file = SPIFFS.open(path, "r");

    // 読み込んだファイルをブラウザ側に返す
    size_t sent = server.streamFile(file, contentType);

    // ファイルクローズ
    file.close();

  }else
  {
      Serial.println("can not read file");
  }
  
  
}