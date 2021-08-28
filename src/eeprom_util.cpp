#include "eeprom_util.h"
#include <Arduino.h>

extern EEPROM_struct eeprom;
/**
 * EEPROMから一括して読み出す
 */
void getEEPROM(void) {
  EEPROM.get(0, eeprom);
}

/**
 * EEPROMに一括して書き出す
 */
void setEEPROM(void) {
  EEPROM.put(0, eeprom);
  EEPROM.commit();
}

/**
 * EEPROMデータをJSON形式で返す
 */
String getEepromJson(void) {
  
  // EEPROMから値を読み出す
  getEEPROM();
  
  String ssid = "";
  String pass = "";

  if(eeprom.connectionInfo.ssid == 0 || eeprom.connectionInfo.password == 0) {

    // EEPROM初期状態ではデフォルト値を設定
    ssid = DEFAULT_SSID;
    pass = DEFAULT_PASS;

  } else {

    // EEPROMの値を設定
    ssid = eeprom.connectionInfo.ssid;
    pass = eeprom.connectionInfo.password;

  }

  // EEPROMの値をJSON形式にする
  String result = "{";
  result += "\"connectionInfo\":";
  result += "{";
  result += "\"ssid\":\"" + ssid + "\"";
  result += ",";
  result += "\"password\":\"" + pass + "\"";
  result += "}";
  result += "}";

  return result;
  
}
