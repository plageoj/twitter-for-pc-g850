#ifndef _URLENCODE_H_INCL
#define _URLENCODE_H_INCL

#include <Arduino.h>

/**
 * 与えられた文字列をパーセントエンコードする
 * @param txt UTF-8文字列
 * @returns エンコード後の ASCII 文字列
 */
inline String urlEncode(const char *txt);
inline String urlEncode(String msg)
{
    return urlEncode(msg.c_str());
}

#endif