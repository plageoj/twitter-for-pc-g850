#include <Arduino.h>
#include <WiFiClient.h>
#include <SPIFFS.h>
#include <GPRINT.h>
#include "KINPUT.h"
#include "UrlEncode.h"

WiFiClient client;
extern HardwareSerial HWSerial;
extern GPRINT gp;
File htz;
unsigned int Max_Segments;

KINPUT::KINPUT(unsigned int max_segs)
{
    Max_Segments = max_segs;
}

int KINPUT::begin(const char *htztable)
{
    htz = SPIFFS.open(htztable, "r");
    if (!htz)
    {
        Serial.println("Failed to open htz table");
        while (1)
            ;
    }
    return client.connect("www.google.com", 80);
}

String KINPUT::convertJapanese(String hiragana, ConversionResult *resultSet)
{
    String ret = "";
    uint8_t segment = 0;

    if (!client.connected())
    {
        begin();
    }
    client.print("GET /transliterate?langpair=ja-Hira|ja&text=" + urlEncode(hiragana) + " HTTP/1.1\r\n");
    client.print("Host: www.google.com\r\n\r\n");

    while (!client.available())
        ;

    while (client.available())
    {
        client.readStringUntil('[');
        client.readStringUntil('"');

        String rbuf = client.readStringUntil('"');
        String ybuf = rbuf;
#ifdef DEBUG
        Serial.println(rbuf);
#endif
        char beginning = ybuf[0];

        if (resultSet != NULL && segment < Max_Segments)
        {
            resultSet[segment].yomi = rbuf;
        }
        for (uint8_t i = 0; client.read() == ','; i++)
        {
            client.readStringUntil('"');

            rbuf = client.readStringUntil('"');
#ifdef DEBUG
            Serial.println(rbuf);
#endif

            if (resultSet != NULL && segment < Max_Segments)
            {
                resultSet[segment].candidates[i] = rbuf;
            }
            if (i == 0)
            {
                if ((beginning >= '0' && beginning <= '9') || beginning == '#' || beginning == '@')
                {
                    ret += ybuf;
                }
                else
                {
                    ret += rbuf;
                }
            }
        }
#ifdef DEBUG
        Serial.println();
#endif
        segment++;
    }
    return ret;
}

String KINPUT::readString()
{
    String sjis, ret = "";
    HWSerial.flush();
    while (1)
    {
        while (!HWSerial.available())
            ;
        sjis = HWSerial.readStringUntil('\r');
        if (sjis.isEmpty() || sjis.startsWith("-"))
        {
            Serial.println(ret);
            return ret;
        }
        if (sjis[0] == 223) // 半濁点
        {
            sjis.remove(0, 1);
            String cdd = htzConvert(sjis);
            ret += cdd;
            gp.gprint(cdd);
            HWSerial.println(GP_NL);
            gp.gprint(F("キーを押してください"));
            HWSerial.println(GP_END);
            while (!HWSerial.available())
                ;
            HWSerial.flush();
            HWSerial.println(GP_END);
            continue;
        }
        ConversionResult segment[Max_Segments];
        convertJapanese(htzConvert(sjis), segment);
        int maxCandidates = 0;
        for (int i = 0; i < Max_Segments && !segment[i].yomi.isEmpty(); i++)
        {
            HWSerial.println('-');
            for (int j = 0; j < 5 && !segment[i].candidates[j].isEmpty(); j++)
            {
                gp.gprint(String(j + 1) + segment[i].candidates[j]);
                HWSerial.println('\xe2'); // テキストモード
                HWSerial.println(GP_NL);
                maxCandidates = j + 1;
            }
            gp.gprint("#1-" + String(maxCandidates) + "? ");
            HWSerial.println(GP_END);

            while (!HWSerial.available())
                ;
            int selectedCandidate = HWSerial.read();
            HWSerial.flush();
            if (selectedCandidate == '-')
                return "";
            selectedCandidate -= '0';
            if (selectedCandidate > 0 && selectedCandidate <= maxCandidates)
            {
                ret += segment[i].candidates[selectedCandidate - 1];
            }
        }
        HWSerial.println(GP_END);
    }
}

String KINPUT::htzConvert(String text)
{
    bool hiramode = true;
    unsigned int pos = 0, len = text.length();
    char buf[4] = "";
    String ret = "";

    while (len - pos)
    {
        char r = text.charAt(pos);
        if (r == 248) // 秒記号 ′′
        {
            hiramode = !hiramode;
        }
        else if (r >= 0xa1 && r <= 0xdf) // 仮名
        {
            uint32_t adr = (r - 0xa1) * 3 + (hiramode ? 0xc1 : 1);
            htz.seek(adr);
            htz.readBytes(buf, 3);

            switch (text.charAt(pos + 1)) // 次の文字が
            {
            case 0xdf: // 半濁点
                buf[2]++;
            case 0xde: // 濁点
                buf[2]++;
                pos++;
            }
            ret.concat(buf);
        }
        else
        {
            ret.concat(r);
        }
        pos++;
    }

#ifdef DEBUG
    Serial.println();
    Serial.println(ret);
#endif
    return ret;
}