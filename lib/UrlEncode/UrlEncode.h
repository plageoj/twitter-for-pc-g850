#ifndef _URLENCODE_H_INCL
#define _URLENCODE_H_INCL

#include <Arduino.h>

/**
 * 与えられた文字列をパーセントエンコードする
 * @param msg UTF-8文字列
 * @returns エンコード後の ASCII 文字列
 */
inline String urlEncode(const char *msg)
{
    const char *hex = "0123456789ABCDEF";
    String encodedMsg = "";

    while (*msg != '\0')
    {
        if (('a' <= *msg && *msg <= 'z') || ('A' <= *msg && *msg <= 'Z') || ('0' <= *msg && *msg <= '9') || *msg == '-' || *msg == '_' || *msg == '.' || *msg == '~')
        {
            encodedMsg += *msg;
        }
        else
        {
            encodedMsg += '%';
            encodedMsg += hex[*msg >> 4];
            encodedMsg += hex[*msg & 0xf];
        }
        msg++;
    }
    return encodedMsg;
}
inline String urlEncode(String msg)
{
    return urlEncode(msg.c_str());
}

#endif