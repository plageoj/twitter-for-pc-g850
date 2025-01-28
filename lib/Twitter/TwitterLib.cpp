#include <Arduino.h>
#include <WiFiClientSecure.h>

#include <codecvt>
#include <string>
#include <cassert>
#include <locale>

#include "UrlEncode.h"
#include "TwitterLib.h"
#include "cJSON.h"

#define Q(root, path) cJSON_GetObjectItemCaseSensitive(root, path)

#define Display_MaxData 3
String unicode_str[Display_MaxData];

// Twitter api root CA:
// Amazon Root CA 1
const char *root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
    "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
    "-----END CERTIFICATE-----";

char *rapid_api_key, *twttr_session;

Twitter::Twitter()
{
  rapid_api_key = (char *)malloc(64);
  twttr_session = (char *)malloc(1024);
}

void Twitter::begin(const char *apiKey, const char *session)
{
  strcpy(rapid_api_key, apiKey);
  strcpy(twttr_session, session);
}

void Twitter::getUserTimeline(const char *screen_name, Tweet *timeline)
{
  Request *rq = new Request("GET", "twttrapi.p.rapidapi.com", "/user-tweets");
  rq->queryParam.add("username", screen_name);
  TwttrApiRequest(rq, timeline);
  delete rq;
}

void Twitter::search(const char *query, Tweet *timeline)
{
  Request *rq = new Request("GET", "twttrapi.p.rapidapi.com", "/search-latest");
  rq->queryParam.add("query", query);
  TwttrApiRequest(rq, timeline);
  delete rq;
}

void Twitter::post(const char *status, Tweet *replyTo)
{
  Request *rq = new Request("POST", "twttrapi.p.rapidapi.com", "/create-tweet");
  if (replyTo != NULL)
  {
    rq->bodyParam.add("in_reply_to_tweet_id", replyTo->tweetId);
  }
  rq->bodyParam.add("tweet_text", status);

  TwttrApiRequest(rq, NULL);
  delete rq;
}

void Twitter::TwttrApiRequest(Request *req, Tweet *timeline)
{
  WiFiClientSecure client;

  client.stop();
  client.setCACert(root_ca);

  if (client.connect(req->host.c_str(), 443, 2000))
  {
    String content = req->bodyParam.get(), header;
    header = req->getRequestString();
    header += "X-Rapidapi-key: " + String(rapid_api_key) + "\r\n";
    header += "X-Rapidapi-host: " + req->host + "\r\n";
    header += "Content-Length: " + String(content.length()) + "\r\n";
    header += "Twttr-Session: " + String(twttr_session) + "\r\n";
    header += "Content-type: application/x-www-form-urlencoded\r\n";
    header += "Host: " + req->host + "\r\n\r\n";

    Serial.println(F("-------------------- HTTP Request Sent"));
    Serial.print(header);
    Serial.print(content);

    client.print(header);
    if (req->method != "GET")
    {
      client.print(content);
      client.print("\r\n");
    }

    if (req->method.equals("POST"))
    {
      return;
    }

    // レスポンスを待つ
    while (!client.available())
      ;
    parseTimeline(&client, timeline);

    Serial.println(F("Response parsed"));
    delay(10);
    client.stop();
    Serial.println(F("Client disconnected"));
  }
  else
  {
    // if you didn't get a connection to the server:
    Serial.println(F("connection failed"));
  }
}

void Twitter::skipUntil(WiFiClient *client, const char *str)
{
  char *buf = (char *)malloc(strlen(str));
  char c;

  Serial.print(F("\nSkip until: "));
  Serial.println(str);

  while (1)
  {
  seek:
    do
    {
      c = client->read();
      Serial.print(c);
    } while (c != *str);

    for (int i = 1; i < strlen(str); i++)
    {
      c = client->read();
      Serial.print(c);

      if (c != str[i])
      {
        goto seek;
      }
    }
    free(buf);
    Serial.println();
    delay(10);
    return;
  }
}

String Twitter::readJsonStringValue(WiFiClient *client)
{
  String value = "";
  char c;
  while (1)
  {
    c = client->read();
    if (c == '\\')
    {
      value += c;
      c = client->read();
    }
    else if (c == '"')
    {
      break;
    }
    value += c;
  }
  return value;
}

uint16_t Twitter::parseTimeline(WiFiClient *client, Tweet *timeline)
{
  uint16_t iter = 0;

  // Header を読み飛ばす
  String header = "";
  do
  {
    header = client->readStringUntil('\n');
    Serial.println(header);
  } while (header != "\r");

  skipUntil(client, "\"TimelineAddEntries\"");

  for (iter = 0; iter < Display_MaxData; iter++ && client->connected() && client->available())
  {
    skipUntil(client, "\"__typename\":\"TimelineTweet\"");
    skipUntil(client, "\"__typename\":\"User\"");
    skipUntil(client, "\"description\":\"");
    // String userDescription = readJsonStringValue(client);

    skipUntil(client, "\"name\":\"");
    String userName = readJsonStringValue(client);
    timeline[iter].name = UTF16toUTF8(userName);
    Serial.println(timeline[iter].name);

    skipUntil(client, "\"profile_link_color\":\"");
    skipUntil(client, "\"screen_name\":\"");
    String userScreenName = readJsonStringValue(client);
    timeline[iter].userScreenName = UTF16toUTF8(userScreenName);
    Serial.println(timeline[iter].userScreenName);

    skipUntil(client, "\"full_text\":\"");
    String tweetText = readJsonStringValue(client);
    timeline[iter].text = UTF16toUTF8(tweetText);
    Serial.println(timeline[iter].text);

    skipUntil(client, "\"rest_id\":\"");
    timeline[iter].tweetId = readJsonStringValue(client);
    Serial.println(timeline[iter].tweetId);
  }

  return iter;
}

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
        code = strtol(str4.c_str(), NULL, 16); // 16進文字列を16進数値に変換
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

std::string Twitter::utf16_to_utf8(std::u16string const &src)
{
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
  return converter.to_bytes(src);
}
