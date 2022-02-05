
#ifndef __DEBUG_LOG_ARRAY_H__
#define __DEBUG_LOG_ARRAY_H__ 
#define DEBUGLOG_SIZE 10
#include <Arduino.h>

class DebugLogArray{
    private:
        String debug_logs[DEBUGLOG_SIZE];
        int current_idx=0;
    public:
        void add(String str);
        String to_string();
};
#endif