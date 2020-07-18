#include <Arduino.h>
#include <WiFiClientSecure.h>

#include <base64.h>
#include "mbedtls/sha1.h"

#include <codecvt>
#include <string>
#include <cassert>
#include <locale>

#include "UrlEncode.cpp"
#include "TwitterLib.h"

#define SHA1_SIZE 20
#define Display_MaxData 15
String unicode_str[Display_MaxData];

//Twitter api root CA
const char *root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
    "d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n"
    "ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n"
    "MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n"
    "LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n"
    "RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n"
    "+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n"
    "PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n"
    "xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n"
    "Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n"
    "hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n"
    "EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n"
    "MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n"
    "FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n"
    "nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n"
    "eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n"
    "hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n"
    "Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n"
    "vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n"
    "+OkuE6N36B9K\n"
    "-----END CERTIFICATE-----\n";

char *consumer_key, *consumer_secret, *access_token, *access_secret;

Twitter::Twitter()
{
  consumer_key = (char *)malloc(26);
  consumer_secret = (char *)malloc(51);
  access_token = (char *)malloc(51);
  access_secret = (char *)malloc(51);
}

void Twitter::begin(const char *ckey, const char *csecret, const char *atoken, const char *asecret)
{
  strcpy(consumer_key, ckey);
  strcpy(consumer_secret, csecret);
  strcpy(access_token, atoken);
  strcpy(access_secret, asecret);
}

void Twitter::getUserTimeline(uint32_t timestamp, const char *screen_name, Tweet *timeline)
{
  Request *rq = new Request("GET", "api.twitter.com", "/1.1/statuses/user_timeline.json");
  rq->queryParam.add("screen_name", screen_name);
  rq->queryParam.add("include_rts", "false");
  TwitterAPI_HTTP_Request(timestamp, rq, timeline);
  delete rq;
}

void Twitter::search(uint32_t timestamp, const char *query, Tweet *timeline)
{
  Request *rq = new Request("GET", "api.twitter.com", "/1.1/search/tweets.json");
  rq->queryParam.add("include_entities", "false");
  rq->queryParam.add("q", query);
  TwitterAPI_HTTP_Request(timestamp, rq, timeline);
  delete rq;
}

void Twitter::post(uint32_t timestamp, const char *status, Tweet *replyTo)
{
  Request *rq = new Request("POST", "api.twitter.com", "/1.1/statuses/update.json");
  if (replyTo != NULL)
  {
    rq->queryParam.add("in_reply_to_status_id", replyTo->tweetId);
  }
  rq->bodyParam.add("status", status);

  TwitterAPI_HTTP_Request(timestamp, rq, NULL);
  delete rq;
}

void Twitter::TwitterAPI_HTTP_Request(uint32_t value_timestamp, Request *req, Tweet *timeline)
{
  WiFiClientSecure client;

  uint32_t value_nonce = 1111111111 + value_timestamp;

  String status_all = "";
  String parameter_str = make_parameter_str(req, value_nonce, value_timestamp);
  String sign_base_str = make_sign_base_str(req, parameter_str);
  String oauth_signature = make_signature(sign_base_str);
  String OAuth_header = make_OAuth_header(req, oauth_signature);

  client.stop();
  client.setCACert(root_ca);

  if (client.connect(req->host.c_str(), 443, 2000))
  {
#ifdef DEBUG
    Serial.print(F("Building an HTTP request."));
#endif

    String content = req->bodyParam.get(), header;
    header = req->getRequestString();
    header += "Accept-Language: ja,en\r\n";
    header += "Authorization: " + OAuth_header + "\r\n";
    header += "Connection: close\r\n";
    header += "Content-Length: " + String(content.length()) + "\r\n";
    header += "Content-Type: application/x-www-form-urlencoded\r\n";
    header += "Accept: application/json\r\n";
    header += "Host: " + req->host + "\r\n\r\n";

    Serial.println(F("-------------------- HTTP Request Sent"));
    Serial.print(header);
    Serial.print(content);
#ifdef DEBUG
#endif

    client.print(header);
    if (req->method != "GET")
    {
      client.print(content);
      client.print("\r\n");
    }

    String res_str = "";

    uint16_t iter = 0, dataset;

    if (req->method.equals("POST"))
    {
      return;
    }

#ifdef DEBUG
    Serial.println(F("--------------------HTTP Response"));
#endif

    while (client.connected())
    {
      while (client.available())
      {
        // Header
        res_str = client.readStringUntil('\n');
#ifdef DEBUG
        Serial.println(res_str);
#endif
        if (res_str.indexOf("\r") <= 2)
        {
          res_str = "";
          dataset = 0;
          while (client.connected())
          {
            // body
            res_str = client.readStringUntil(',');
#ifdef DEBUG
            Serial.println(res_str);
#endif
            if (iter >= Display_MaxData)
              break;
            if (res_str.startsWith("\"name\":\""))
            {
              while (res_str[res_str.length() - 1] != '"')
              {
                res_str += ',';
                res_str += client.readStringUntil(',');
              }
              res_str.remove(0, 8);
              res_str.remove(res_str.length() - 1);
              timeline[iter].name = UTF16toUTF8(res_str);
              dataset |= 1;
            }
            else if ((dataset & 2) == 0 && res_str.startsWith("\"text\":\""))
            {
              while (res_str[res_str.length() - 1] != '"')
              {
                res_str += ',';
                res_str += client.readStringUntil(',');
              }
              res_str = UTF16toUTF8(res_str);
              res_str.remove(0, 8);
              res_str.remove(res_str.length() - 1);
              timeline[iter].text = res_str;
              dataset |= 2;
            }
            else if ((dataset & 4) == 0 && res_str.startsWith("\"id_str\":"))
            {
              res_str.remove(0, 10);
              res_str.remove(res_str.length() - 1);
              timeline[iter].tweetId = res_str;
              dataset |= 4;
            }
            else if (res_str.startsWith("\"screen_name\":"))
            {
              res_str.remove(0, 15);
              res_str.remove(res_str.length() - 1);
              timeline[iter].userScreenName = res_str;
              dataset |= 8;
            }
            else if (res_str.startsWith("\"favorite_count\":"))
            {
              if (dataset == 15)
              {
                iter++;
                dataset = 0;
              }
            }
            // place name を user name と誤認しないよう、読み飛ばす
            else if (res_str.startsWith("\"place\":{"))
            {
              client.readStringUntil('}');
            }
          }
        }
      }
    }
    client.flush();
    delay(10);
    client.stop();
  }
  else
  {
    // if you didn't get a connection to the server:
    Serial.println(F("connection failed"));
  }
}

String Twitter::make_parameter_str(Request *req, uint32_t value_nonce, uint32_t value_timestamp)
{
  req->authParam.add("oauth_consumer_key", consumer_key);
  req->authParam.add("oauth_nonce", String(value_nonce));
  req->authParam.add("oauth_signature_method", "HMAC-SHA1");
  req->authParam.add("oauth_timestamp", String(value_timestamp));
  req->authParam.add("oauth_token", access_token);
  req->authParam.add("oauth_version", "1.0");

#ifdef DEBUG
  Serial.print(F("parameter_str:  "));
  Serial.println(req->getParameterString());
#endif
  return req->getParameterString();
}
//*************************************************
String Twitter::make_sign_base_str(Request *req, String parameter_str)
{
  String sign_base_str = req->method + "&" + urlEncode(req->getUrl("https").c_str());
  sign_base_str += "&" + urlEncode(parameter_str.c_str());
#ifdef DEBUG
  Serial.print(F("sign_base_str = "));
  Serial.println(sign_base_str);
#endif
  return sign_base_str;
}
//*************************************************
String Twitter::make_signature(String sign_base_str)
{
  String signing_key = urlEncode(consumer_secret);
  signing_key += "&";
  signing_key += urlEncode(access_secret);
#ifdef DEBUG
  Serial.print(F("signing_key = "));
  Serial.println(signing_key);
#endif

  unsigned char digestkey[32];
  mbedtls_sha1_context context;

  mbedtls_sha1_starts(&context);
  mbedtls_sha1_update(&context, (uint8_t *)signing_key.c_str(), (int)signing_key.length());
  mbedtls_sha1_finish(&context, digestkey);

  uint8_t digest[32];
  ssl_hmac_sha1((uint8_t *)sign_base_str.c_str(), (int)sign_base_str.length(), digestkey, SHA1_SIZE, digest);

  String oauth_signature = base64::encode(digest, SHA1_SIZE).c_str();
#ifdef DEBUG
  Serial.print(F("oauth_signature = "));
  Serial.println(oauth_signature);
#endif
  return oauth_signature;
}
//*************************************************
String Twitter::make_OAuth_header(Request *req, String oauth_signature)
{
  Parameter temp;

  temp = req->authParam;
  temp.concat(req->queryParam);
  temp.sort();
  temp.add("oauth_signature", oauth_signature);

  return "OAuth " + temp.get(",", "=");
}
//*************************************************
//Reference: https://github.com/igrr/axtls-8266/blob/master/crypto/hmac.c
//License axTLS 1.4.9 Copyright (c) 2007-2016, Cameron Rich
void Twitter::ssl_hmac_sha1(uint8_t *msg, int length, const uint8_t *key, int key_len, unsigned char *digest)
{
  mbedtls_sha1_context context;
  uint8_t k_ipad[64];
  uint8_t k_opad[64];
  int i;

  memset(k_ipad, 0, sizeof k_ipad);
  memset(k_opad, 0, sizeof k_opad);
  memcpy(k_ipad, key, key_len);
  memcpy(k_opad, key, key_len);

  for (i = 0; i < 64; i++)
  {
    k_ipad[i] ^= 0x36;
    k_opad[i] ^= 0x5c;
  }

  mbedtls_sha1_starts(&context);
  mbedtls_sha1_update(&context, k_ipad, 64);
  mbedtls_sha1_update(&context, msg, length);
  mbedtls_sha1_finish(&context, digest);
  mbedtls_sha1_starts(&context);
  mbedtls_sha1_update(&context, k_opad, 64);
  mbedtls_sha1_update(&context, digest, SHA1_SIZE);
  mbedtls_sha1_finish(&context, digest);
}
//********** Unicode ( UTF16 ) to UTF-8 convert ********************************
String Twitter::UTF16toUTF8(String str)
{
  str.replace("\\n", "\n");
  str.replace("\\u", "\\");
  str.replace("\\/", "/");
  str += '\0';
  uint16_t len = str.length();
  char16_t utf16code[len], code;

  int i = 0;
  String str4 = "";
  for (int j = 0; j < len; j++)
  {
    if (str[j] == 0x5C)
    { //'\'を消去
      j++;
      if ((str[j + 1] >= '0' && str[j + 1] <= '9') ||
          (str[j + 1] >= 'a' && str[j + 1] <= 'f'))
      {
        for (int k = 0; k < 4; k++)
        {
          str4 += str[j + k];
        }
        code = strtol(str4.c_str(), NULL, 16); //16進文字列を16進数値に変換
        // サロゲートペアをとばす
        if (code >= 0xd800 && code <= 0xdbff)
        {
          j += 5;
          utf16code[i] = 32;
        }
        else
        {
          utf16code[i] = code;
        }
        i++;
        str4 = "";
        j = j + 3;
      }
      else
      {
        utf16code[i] = (char16_t)str[j];
        i++;
      }
    }
    else
    {
      utf16code[i] = (char16_t)str[j];
      i++;
    }
  }

  std::u16string u16str(utf16code);
  std::string u8str = utf16_to_utf8(u16str);

  return String(u8str.c_str());
}
//***********************************************************************
std::string Twitter::utf16_to_utf8(std::u16string const &src)
{
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
  return converter.to_bytes(src);
}
