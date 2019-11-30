#ifndef URLENCODE_H_INCL
#define URLENCODE_H_INCL

#include <Arduino.h>

/**
 * 与えられた文字列をパーセントエンコードする
 * @param txt UTF-8文字列
 * @returns エンコード後の ASCII 文字列
 */
String urlEncode(const char *txt);
String urlEncode(String msg)
{
    return urlEncode(msg.c_str());
}

#endif