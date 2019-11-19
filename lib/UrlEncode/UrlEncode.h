#ifndef URLENCODE_H_INCL
#define URLENCODE_H_INCL

#include <Arduino.h>

String urlEncode(const char *);
String urlEncode(String msg)
{
    return urlEncode(msg.c_str());
}

#endif