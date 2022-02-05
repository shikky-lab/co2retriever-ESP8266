#include "debug_util.h"
#include <FS.h>
#include <ArduinoOTA.h>

void showAllFilesInSPIFFS(){
  Serial.println("Show all files in spiffs");
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
      Serial.print(dir.fileName());
      Serial.print("\t");
      Serial.println(dir.fileSize());
  }
  Serial.println("finished listing up");
}