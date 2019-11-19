#include <Arduino.h>

#ifndef ESP32_TWITTER_LIB
#define ESP32_TWITTER_LIB
class Twitter
{
public:
  struct Tweet
  {
    uint32_t id;
    String name;
    String text;
  };

  Twitter();
  void begin(const char *, const char *, const char *, const char *);
  void getUserTimeline(uint32_t, const char *, Tweet *);
  void search(uint32_t, const char *, Tweet *);
  void post(uint32_t, const char*);

private:
  void TwitterAPI_HTTP_Request(uint32_t, const char *, Tweet *);
  String make_parameter_str(String, uint32_t, uint32_t);
  String make_sign_base_str(String);
  String make_signature(const char *, const char *, String);
  String make_OAuth_header(String, uint32_t, uint32_t);
  void ssl_hmac_sha1(uint8_t *, int, const uint8_t *, int, unsigned char *);
  String UTF16toUTF8(String str);
  std::string utf16_to_utf8(std::u16string const &);
};
#endif