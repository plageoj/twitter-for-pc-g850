#include <Arduino.h>
#include <WiFiClient.h>
#include <UrlEncode.h>
#include "KINPUT.h"

String convertJapanese(String hiragana, ConversionResult *resultSet)
{
    WiFiClient client;
    String ret = "";
    uint8_t segment = 0;

    if (client.connect("www.google.com", 80))
    {
        client.print("GET /transliterate?langpair=ja-Hira|ja&text=" + urlEncode(hiragana) + " HTTP/1.1\r\n");
        client.print("Host: www.google.com\r\n\r\n");
#ifdef DEBUG
        Serial.println(hiragana);
#endif
    }
    else
    {
        Serial.println(F("Can't connect to conversion service!"));
        return hiragana;
    }

    while (!client.available())
        ;

    while (client.available())
    {
        client.readStringUntil('[');
        client.readStringUntil('"');

        String rbuf = client.readStringUntil('"');
#ifdef DEBUG
        Serial.println(rbuf);
#endif

        resultSet[segment].yomi = rbuf;
        for (uint8_t i = 0; client.read() == ','; i++)
        {
            client.readStringUntil('"');

            rbuf = client.readStringUntil('"');
#ifdef DEBUG
            Serial.println(rbuf);
#endif

            resultSet[segment].candidates[i] = rbuf;
            if (i == 0)
            {
                ret += rbuf;
            }
        }
#ifdef DEBUG
        Serial.println();
#endif
        segment++;
    }
    client.stop();
    return ret;
}