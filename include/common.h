
#ifndef __COMMON_H__
#define __COMMON_H__ 

#include "DebugLogArray.h"

const int DEBUG_LOG_SIZE=1000 ;

volatile extern unsigned int co2ppm;
// volatile extern char debugLog[DEBUG_LOG_SIZE];
extern DebugLogArray debugLog;

#endif