#ifndef ESP32_TWITTER_LIB
#define ESP32_TWITTER_LIB

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include "Request.h"
class Twitter
{
public:
  struct Tweet
  {
    String tweetId;
    String userScreenName;
    String name;
    String text;
  };

  Twitter();
  void begin(const char *, const char *);
  void getUserTimeline(const char *, Tweet *);
  void search(const char *, Tweet *);
  void post(const char *, Tweet *);
  void post(const char *status)
  {
    return post(status, NULL);
  }

private:
  void TwttrApiRequest(Request *, Tweet *);
  /**
   * TCP/IP 通信で取得した JSON データを解析し、Tweet 構造体に格納する
   * @param *client TCP/IP クライアント
   * @param *timeline Tweet 構造体の配列
   * @returns 取得したツイート数
   */
  uint16_t parseTimeline(WiFiClient *, Tweet *);
  void skipUntil(WiFiClient *, const char *);
  String readJsonStringValue(WiFiClient *);
  String UTF16toUTF8(String str);
  std::string utf16_to_utf8(std::u16string const &);
};
#endif