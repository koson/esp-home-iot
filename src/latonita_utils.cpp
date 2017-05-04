#ifndef _LATONITA_UTILS_C_
#define _LATONITA_UTILS_C_

#include "latonita_utils.h"

char * secondsToString(unsigned long t) {
    static char str[12];
    long h = t / 3600;
    t = t % 3600;
    int m = t / 60;
    int s = t % 60;
    sprintf(str, "%04ld:%02d:%02d", h, m, s);
    return str;
}

char * getUpTime() {
    return secondsToString(millis() / 1000);
}

void delayMicros(uint32_t us){
    uint32_t start = micros();
    while(micros() - start < us) {}
}

void delayMs(uint32_t ms){
    uint32_t start = millis();
    while(millis() - start < ms) {
        yield();
    }
}

char * formatDouble41(double d) {
    return dtostrf(d, 4, 1, utils_buff32);
}

#endif
