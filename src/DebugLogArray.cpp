#include <Arduino.h>
#include "DebugLogArray.h"

void DebugLogArray::add(String str){
    debug_logs[this->current_idx++]=str;
    if(this->current_idx>=DEBUGLOG_SIZE){
        this->current_idx=0;
    }
}

String DebugLogArray::to_string(){
    String str;
    int j=this->current_idx;
    for(int i=0;i<DEBUGLOG_SIZE;i++){
        str.concat(debug_logs[j++]);
        str.concat("<br/>");
        if(j>=DEBUGLOG_SIZE){
            j=0;
        }
    }
    return str;
}