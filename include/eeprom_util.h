#ifndef __EEPROM_UTIL_H__
#define __EEPROM_UTIL_H__

#include <EEPROM.h>
#include <Arduino.h>

#define EEPROM_SSID_SIZE 20
#define EEPROM_PASS_SIZE 20
#define DEFAULT_SSID "ssid"
#define DEFAULT_PASS "password"

typedef struct {
  char ssid[EEPROM_SSID_SIZE + 1];
  char password[EEPROM_PASS_SIZE + 1];
} EEPROM_Connection;

typedef struct {
  EEPROM_Connection connectionInfo;
} EEPROM_struct;

/* EEPROMから一括して読み出す */
void getEEPROM(void);
/* EEPROMに一括して書き出す */
void setEEPROM(void);
/* EEPROMデータをJSON形式で返す */
String getEepromJson(void);

#endif
